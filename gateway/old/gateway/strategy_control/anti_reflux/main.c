/*
 * 防逆流策略调控服务进程入口
 * 支持 -h 帮助、-d 调试、-c 配置根路径，并加载防逆流配置与设备模型。
 * 通过 database 的 db_device / db_node_fops 接口对测点进行读写（dblink_id）。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "anti_reflux_config.h"
#include "anti_reflux_model.h"
#include "anti_reflux_controller.h"
#include "anti_reflux_safety_guard.h"
#include "config/anti_reflux_cfg_load.h"
#include "db_device.h"
#include "db_node_fops.h"
#include "db_struct.h"

/** 默认配置根路径（设备上一般为 /userdata/config） */
#define DEFAULT_CONFIG_BASE   "/userdata/config"

/** 运行上下文：调试开关与配置路径 */
static int  g_debug;
static char g_config_base[ANTI_REFLUX_CFG_BASE_PATH_LEN];
/** -o 安全防线继电器直测：0=关闭；'t'=分闸；'c'=合闸；'b'=先分后合 */
static char g_sg_test_relay;
static int  g_sg_test_inv = -1;

/*********************************************************************************************************
** Function name:       wait_ctrl_queue_drain
** Descriptions:        等待本策略所用通道的控制队列全部下发后再执行本次周期
** input parameters:    model - 设备模型
** output parameters:   NONE
** Returned value:      1-已排空可执行本轮；0-超时仍有未下发命令
*********************************************************************************************************/
static int wait_ctrl_queue_drain(const AntiRefluxDeviceModel *model)
{
	uint32_t i;
	uint8_t ch;
	int pending;
	unsigned wait_ms = 0;
#define WAIT_CTRL_QUEUE_TIMEOUT_MS  5000
#define WAIT_CTRL_QUEUE_POLL_MS    10

	if (!model)
		return 1;
	for (;;)
	{
		pending = 0;
		ch = (uint8_t)(model->meter.ChnlId & 0xFF);
		if (db_ctrl_queue_pending_count(ch) > 0)
			pending = 1;
		for (i = 0; i < model->inverter_count && !pending; i++)
		{
			if (!model->inverters[i].persist_enabled)
				continue;
			ch = (uint8_t)(model->inverters[i].ChnlId & 0xFF);
			if (db_ctrl_queue_pending_count(ch) > 0)
				pending = 1;
		}
		if (!pending || wait_ms >= WAIT_CTRL_QUEUE_TIMEOUT_MS)
			break;
		usleep(WAIT_CTRL_QUEUE_POLL_MS * 1000);
		wait_ms += WAIT_CTRL_QUEUE_POLL_MS;
	}
	return pending ? 0 : 1;
}




/*********************************************************************************************************
** Function name:       usage
** Descriptions:        打印命令行用法说明
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void usage(void)
{
    fprintf(stderr, "\nanti_reflux [options]\n\n");
    fprintf(stderr, "  -h                  : Show this help message\n");
    fprintf(stderr, "  -d 0|1             : Debug. 0=off, 1=print config/model/safety-guard relay log (default 0)\n");
    fprintf(stderr, "  -c <config_base>   : Config root path (default: %s)\n", DEFAULT_CONFIG_BASE);
    fprintf(stderr, "  -o t|c|b           : Test safety-guard relay via flush_commands (same as runtime), then exit\n");
    fprintf(stderr, "                       t=trip(S1 on,S2 off) c=close(S1 off,S2 on) b=both\n");
    fprintf(stderr, "                       requires config.safety_guard.enable and safety_relay_bind.enabled\n");
    fprintf(stderr, "  -i <inv_idx>       : Inverter index for -o (default: first with relay bind)\n");
    fprintf(stderr, "\nExample:\n");
    fprintf(stderr, "  anti_reflux -c /userdata/config -d 1\n");
    fprintf(stderr, "  anti_reflux -c /userdata/config -o t -i 0\n");
    fprintf(stderr, "  anti_reflux -c /userdata/config -o b\n\n");
}

static int inv_sg_relay_bind_ready(const AntiRefluxInverterModel *inv)
{
    if (!inv || !inv->persist_enabled || !inv->safety_relay_enabled)
        return 0;
    if (inv->safety_trip_dblink_id == ANTI_REFLUX_DBLINK_ID_INVALID ||
        inv->safety_close_dblink_id == ANTI_REFLUX_DBLINK_ID_INVALID ||
        inv->safety_state_dblink_id == ANTI_REFLUX_DBLINK_ID_INVALID)
        return 0;
    return 1;
}

/*********************************************************************************************************
** Function name:       find_sg_test_inv_index
** Descriptions:        查找可用于安全防线 flush_commands 直测的逆变器索引
** input parameters:    model - 设备模型；req_idx - 指定索引，<0 则自动查找
** output parameters:   NONE
** Returned value:      逆变器索引；<0 表示未找到
*********************************************************************************************************/
static int find_sg_test_inv_index(const AntiRefluxDeviceModel *model, int req_idx)
{
    uint32_t i;

    if (!model)
        return -1;
    if (req_idx >= 0)
    {
        if ((uint32_t)req_idx >= model->inverter_count)
            return -1;
        if (model->inverters[req_idx].safety_trip_dblink_id == ANTI_REFLUX_DBLINK_ID_INVALID ||
            model->inverters[req_idx].safety_close_dblink_id == ANTI_REFLUX_DBLINK_ID_INVALID)
            return -1;
        if (!inv_sg_relay_bind_ready(&model->inverters[req_idx]))
            return -1;
        return req_idx;
    }
    for (i = 0; i < model->inverter_count; i++)
    {
        if (inv_sg_relay_bind_ready(&model->inverters[i]))
            return (int)i;
    }
    return -1;
}

/*********************************************************************************************************
** Function name:       parse_sg_test_relay_cmd
** Descriptions:        解析 -o 选项的继电器测试命令字符
** input parameters:    c - 命令字符 t/c/b
** output parameters:   NONE
** Returned value:      测试命令枚举；(AntiRefluxSgTestRelayCmd)-1 表示无效
*********************************************************************************************************/
static AntiRefluxSgTestRelayCmd parse_sg_test_relay_cmd(char c)
{
    switch (c)
    {
        case 't':
        case 'T':
            return AR_SG_TEST_RELAY_TRIP;
        case 'c':
        case 'C':
            return AR_SG_TEST_RELAY_CLOSE;
        case 'b':
        case 'B':
            return AR_SG_TEST_RELAY_BOTH;
        default:
            return (AntiRefluxSgTestRelayCmd)-1;
    }
}

/*********************************************************************************************************
** Function name:       run_sg_relay_output_test
** Descriptions:        通过 anti_reflux_sg_flush_commands 执行安全防线继电器直测
** input parameters:    cfg - 配置；model - 设备模型；inv_idx - 逆变器索引；cmd - 测试命令
** output parameters:   NONE
** Returned value:      0-成功；非0-失败
*********************************************************************************************************/
static int run_sg_relay_output_test(const AntiRefluxConfig *cfg,
                                    const AntiRefluxDeviceModel *model,
                                    int inv_idx,
                                    AntiRefluxSgTestRelayCmd cmd)
{
    AntiRefluxSafetyGuardCtx sg_ctx;
    AntiRefluxConfig cfg_local;
    struct timespec ts;
    uint32_t now_ms = 0;
    int ret;

    if (!cfg || !model || inv_idx < 0)
        return -1;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0)
        now_ms = (uint32_t)(ts.tv_sec * 1000U + ts.tv_nsec / 1000000U);

    memset(&sg_ctx, 0, sizeof(sg_ctx));
    anti_reflux_sg_init(&sg_ctx, model);

    cfg_local = *cfg;
    if (cfg_local.safety_guard.relay_command_interval_ms == 0U)
        cfg_local.safety_guard.relay_command_interval_ms = 200U;
    if (!cfg_local.safety_guard.enable)
    {
        fprintf(stderr,
                "[anti_reflux][sg][test] warn: config.safety_guard.enable=false, force enable for -o test\n");
        cfg_local.safety_guard.enable = 1;
    }

    fprintf(stderr,
            "[anti_reflux][sg][test] path=flush_commands inv=%d S1(trip)=0x%08X S2(close)=0x%08X interval_ms=%u\n",
            inv_idx,
            (unsigned)model->inverters[inv_idx].safety_trip_dblink_id,
            (unsigned)model->inverters[inv_idx].safety_close_dblink_id,
            (unsigned)cfg_local.safety_guard.relay_command_interval_ms);

    ret = anti_reflux_sg_test_relay_output(&sg_ctx,
                                           &cfg_local,
                                           model,
                                           DataMgmtFops.write_command_op,
                                           (uint32_t)inv_idx,
                                           cmd,
                                           now_ms);
    if (ret != 0)
    {
        fprintf(stderr, "[anti_reflux][sg][test] failed ret=%d\n", ret);
        return ret;
    }

    if (!wait_ctrl_queue_drain(model))
        fprintf(stderr, "[anti_reflux][sg][test] warn: ctrl queue not fully drained\n");

    fprintf(stderr, "[anti_reflux][sg][test] ok via flush_commands, observe relay: only one of S1/S2 should be ON\n");
    return 0;
}

/*********************************************************************************************************
** Function name:       debug_print_config
** Descriptions:        调试模式下打印已加载的防逆流 config 摘要
** input parameters:    pCfg - 已加载的 config
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void debug_print_config(const AntiRefluxConfig *pCfg)
{
    if (!pCfg || !g_debug)
        return;
    printf("[anti_reflux] config: version=%s enable=%d ctrl_interval_ms=%u algo_mode=%s\n",
           pCfg->version,
           pCfg->system.enable ? 1 : 0,
           (unsigned)pCfg->system.ctrl_interval_ms,
           pCfg->system.algo_mode);
    printf("[anti_reflux] input: target_power_w=%d reverse_threshold_w=%d filter_alpha=%.2f\n",
           (int)pCfg->input_processing.target_power_w,
           (int)pCfg->input_processing.reverse_threshold_w,
           pCfg->input_processing.filter_alpha);
    printf("[anti_reflux] safety: ramp_up=%d ramp_down=%d meter_fail_timeout_s=%u meter_fail_action=%s\n",
           (int)pCfg->output_safety.ramp_up_pc_per_sec, (int)pCfg->output_safety.ramp_down_pc_per_sec,
           (unsigned)pCfg->output_safety.meter_fail_timeout_s, pCfg->output_safety.meter_fail_action);
    printf("[anti_reflux] inverter_groups: strategy=%s min_running_pc=%d device_count=%u\n",
           pCfg->inverter_groups.strategy, (int)pCfg->inverter_groups.min_running_pc,
           (unsigned)pCfg->inverter_groups.device_count);
}

/*********************************************************************************************************
** Function name:       debug_print_model
** Descriptions:        调试模式下打印已加载的设备模型摘要
** input parameters:    pModel - 已加载的 device model
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void debug_print_model(const AntiRefluxDeviceModel *pModel)
{
    uint32_t i, j;
    const AntiRefluxStrategyPoint *pt;
    int slot;

    if (!pModel || !g_debug)
        return;
    printf("[anti_reflux] model: meter ChnlId=%d DevIndex=%d\n",
           (int)pModel->meter.ChnlId, (int)pModel->meter.DevIndex);
    for (j = 0; j < ANTI_REFLUX_METER_READ_POINTS; j++)
    {
        pt = &pModel->meter.read_points[j];
        slot = (pt->slot_index == 0xFFFF) ? -1 : (int)pt->slot_index;
        printf("[anti_reflux]   meter read[%u] %s point_index=%u slot_index=%d datattr=%u dblink_id=0x%08X\n",
               (unsigned)j, pt->point_name, (unsigned)pt->point_index, slot,
               (unsigned)pt->datattr, (unsigned)pt->dblink_id);
    }
    printf("[anti_reflux] model: inverter_count=%u\n", (unsigned)pModel->inverter_count);
    for (i = 0; i < pModel->inverter_count; i++)
    {
        printf("[anti_reflux]   inv[%u] ChnlId=%d DevIndex=%d persist_enabled=%u\n",
               (unsigned)i, (int)pModel->inverters[i].ChnlId, (int)pModel->inverters[i].DevIndex,
               (unsigned)pModel->inverters[i].persist_enabled);
        for (j = 0; j < ANTI_REFLUX_INV_READ_POINTS; j++)
        {
            pt = &pModel->inverters[i].read_points[j];
            slot = (pt->slot_index == 0xFFFF) ? -1 : (int)pt->slot_index;
            printf("[anti_reflux]     read[%u] %s point_index=%u slot_index=%d datattr=%u dblink_id=0x%08X\n",
                   (unsigned)j, pt->point_name, (unsigned)pt->point_index, slot,
                   (unsigned)pt->datattr, (unsigned)pt->dblink_id);
        }
        for (j = 0; j < ANTI_REFLUX_INV_WRITE_POINTS; j++)
        {
            pt = &pModel->inverters[i].write_points[j];
            slot = (pt->slot_index == 0xFFFF) ? -1 : (int)pt->slot_index;
            printf("[anti_reflux]     write[%u] %s point_index=%u slot_index=%d datattr=%u dblink_id=0x%08X\n",
                   (unsigned)j, pt->point_name, (unsigned)pt->point_index, slot,
                   (unsigned)pt->datattr, (unsigned)pt->dblink_id);
        }
        pt = &pModel->inverters[i].rw_dynamic_control;
        slot = (pt->slot_index == 0xFFFF) ? -1 : (int)pt->slot_index;
        printf("[anti_reflux]     read_write DynamicAntiRefluxEnable: slot_index=%d datattr=%u dblink_id=0x%08X user_fill=%u\n",
               slot, (unsigned)pt->datattr, (unsigned)pt->dblink_id, (unsigned)pt->is_user_fill);
    }
}

/*********************************************************************************************************
** Function name:       main
** Descriptions:        解析命令行，加载防逆流配置与设备模型并保持运行
** input parameters:    argc, argv - 标准 main 参数
** output parameters:   NONE
** Returned value:      0-正常退出；非0-错误或帮助退出
*********************************************************************************************************/
int main(int argc, char *argv[])
{
    int                     opt;
    int                     ret;
    AntiRefluxConfig        cfg;
    AntiRefluxDeviceModel   model;

    g_debug = 0;
    g_sg_test_relay = 0;
    snprintf(g_config_base, sizeof(g_config_base), "%s", DEFAULT_CONFIG_BASE);

    while ((opt = getopt(argc, argv, "hd:c:o:i:")) != -1)
    {
        switch (opt)
        {
            case 'h':
                usage();
                return 0;
            case 'd':
                g_debug = (optarg && atoi(optarg) != 0) ? 1 : 0;
                break;
            case 'c':
                if (optarg && optarg[0] != '\0')
                    snprintf(g_config_base, sizeof(g_config_base), "%s", optarg);
                break;
            case 'o':
                if (optarg && optarg[0] != '\0')
                    g_sg_test_relay = optarg[0];
                break;
            case 'i':
                if (optarg)
                    g_sg_test_inv = atoi(optarg);
                break;
            default:
                usage();
                return -1;
        }
    }

    if (g_sg_test_relay != 0 && parse_sg_test_relay_cmd(g_sg_test_relay) < 0)
    {
        fprintf(stderr, "[anti_reflux] invalid -o value (use t, c, or b)\n");
        usage();
        return -1;
    }

    if (g_debug)
        printf("[anti_reflux] config_base=%s\n", g_config_base);

    memset(&cfg, 0, sizeof(cfg));
    ret = anti_reflux_config_load(&cfg, g_config_base);
    if (ret != 0)
    {
        fprintf(stderr, "[anti_reflux] anti_reflux_config_load failed (ret=%d)\n", ret);
        return -1;
    }
    if (g_debug)
        debug_print_config(&cfg);

    memset(&model, 0, sizeof(model));
    ret = anti_reflux_model_load(&model, g_config_base);
    if (ret != 0)
    {
        fprintf(stderr, "[anti_reflux] anti_reflux_model_load failed (ret=%d)\n", ret);
        return -1;
    }
    if (g_debug)
        debug_print_model(&model);

    /*
     * 参照 modbus_slave：挂接实时库并初始化测点读写接口
     */
    void *vmem = rt_database_attach();
    if (vmem == NULL)
    {
        fprintf(stderr, "[anti_reflux] rt_database_attach failed, please insert /dev/vmem.ko first\n");
        return -1;
    }
    DataMgmtFops.init(vmem);

    if (g_sg_test_relay != 0)
    {
        int inv_idx = find_sg_test_inv_index(&model, g_sg_test_inv);
        AntiRefluxSgTestRelayCmd cmd = parse_sg_test_relay_cmd(g_sg_test_relay);

        if (inv_idx < 0)
        {
            fprintf(stderr,
                    "[anti_reflux][sg][test] no inverter ready for safety-guard flush_commands"
                    " (need safety_guard.enable, persist enable, safety_relay_bind.enabled, valid trip/close/state dblink;"
                    " use -i <idx> if multiple inverters)\n");
            rt_database_deattach();
            return -1;
        }
        ret = run_sg_relay_output_test(&cfg, &model, inv_idx, cmd);
        rt_database_deattach();
        return (ret == 0) ? 0 : -1;
    }

    AntiRefluxController ctrl;
    anti_reflux_controller_init(&ctrl, &cfg, &model,
        (anti_reflux_read_point_fn)DataMgmtFops.readpoint_by_id,
        (anti_reflux_write_point_fn)DataMgmtFops.write_command_op,
        (anti_reflux_read_ctrlpoint_fn)DataMgmtFops.readctrlpoint_by_id,
        (anti_reflux_write_ctrlpoint_fn)DataMgmtFops.writectrlpoint_by_id);
    ctrl.write_command_op = DataMgmtFops.write_command_op;
    ctrl.debug = g_debug;
    anti_reflux_dynamic_suppress_bootstrap_zero(&model,
        (anti_reflux_write_ctrlpoint_fn)DataMgmtFops.writectrlpoint_by_id);

    if (g_debug)
    {
        static const char *meter_read_names[] = {
            "Ua", "Ub", "Uc", "Ia", "Ib", "Ic", "Pa", "Pb", "Pc", "Ptot", "Freq"
        };
        unsigned j;

        printf("[anti_reflux] load ok, db attached, running ...\n");
        /* 示例：读电表 11 个测点（Ua/Ub/Uc, Ia/Ib/Ic, Pa/Pb/Pc, Ptot, Freq） */
        for (j = 0; j < ANTI_REFLUX_METER_READ_POINTS; j++)
        {
            uint32_t id = model.meter.read_points[j].dblink_id;
            double   val = 0.0;
            const char *name = (j < sizeof(meter_read_names) / sizeof(meter_read_names[0]))
                ? meter_read_names[j] : model.meter.read_points[j].point_name;
            if (id == ANTI_REFLUX_DBLINK_ID_INVALID)
                printf("[anti_reflux] meter %s (dblink_id=INVALID)\n", name);
            else
            {
                int r = DataMgmtFops.readpoint_by_id(id, &val);
                if (r == 0)
                    printf("[anti_reflux] meter %s (dblink_id=0x%08X) = %.2f\n", name, (unsigned)id, val);
                else
                    printf("[anti_reflux] meter %s readpoint_by_id(0x%08X) ret=%d\n", name, (unsigned)id, r);
            }
        }
    }

    if (g_debug)
        printf("[anti_reflux] control loop interval_ms=%u\n", (unsigned)cfg.system.ctrl_interval_ms);

    for (;;)
    {
        struct timespec ts;
        uint32_t now_ms;
        unsigned ms = cfg.system.ctrl_interval_ms;
        if (ms < 100)
            ms = 100;
        sleep(ms / 1000);
        if (ms % 1000 != 0)
            usleep((ms % 1000) * 1000);  /* 不足 1 秒的毫秒用 usleep 补足 */
        /* 仅当上次周期控制命令全部下发后才执行本轮，避免队列积压导致 pctrl is NULL */
        if (!wait_ctrl_queue_drain(&model))
        {
            if (g_debug)
                fprintf(stderr, "[anti_reflux] skip cycle: ctrl queue not drained (timeout)\n");
            continue;
        }
        if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0)
            now_ms = (uint32_t)(ts.tv_sec * 1000U + ts.tv_nsec / 1000000U);
        else
            now_ms = 0;
        ret = anti_reflux_controller_run(&ctrl, now_ms);
        if (g_debug && ret != 0)
            fprintf(stderr, "[anti_reflux] controller_run ret=%d\n", ret);
    }

    rt_database_deattach();
    return 0;
}
