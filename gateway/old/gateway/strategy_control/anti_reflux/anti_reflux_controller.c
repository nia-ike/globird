/**
 * @file anti_reflux_controller.c
 * @brief 防逆流总控：负载估计 + 目标总功率 + 爬坡 + 按容量分配 + 颗粒度量化 → 写各机限功率
 *
 * 控制逻辑（单一路径，无“满发/限发”分支切换）：
 *   1) 负载估计 load_est = Σ逆变器当前有功 + 电表功率(raw_ptot，负=送网)
 *   2) 期望总功率 desired_total = load_est + target_power_w，限制在 [0, sum_rated]
 *   3) 若有逆变器配置了 limit_power_step_w，则总目标与各机分配均按颗粒度量化，并采用死区避免振荡
 *   4) 经 safety 层爬坡/电表故障处理后，按容量分配到各机 limit_w，再按各机颗粒度量化
 *   5) 仅当与读回限值差异超过容差时才写入
 *   动态抑制：InverterRefs.enable 持久层 + 虚拟点剩余秒数 S（每周期末 S-=dt 写回）
 */

#include "anti_reflux_controller.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

/** 与读回限值差异超过此值(W)才写，避免浮点抖动与频繁写 */
#define LIMIT_W_WRITE_TOLERANCE_W  1.0

/*********************************************************************************************************
** Function name:       inv_dynamic_bound
** Descriptions:        判断逆变器是否绑定有效的动态防逆流虚拟点
** input parameters:    inv - 逆变器模型
** output parameters:   NONE
** Returned value:      1-已绑定；0-未绑定
*********************************************************************************************************/
static int inv_dynamic_bound(const AntiRefluxInverterModel *inv)
{
    if (!inv || inv->rw_dynamic_control.is_user_fill)
        return 0;
    return inv->rw_dynamic_control.dblink_id != ANTI_REFLUX_DBLINK_ID_INVALID;
}

/*********************************************************************************************************
** Function name:       dynamic_suppress_read_raw
** Descriptions:        读取各机动态防逆流抑制剩余秒数 S
** input parameters:    ctrl - 控制器；model - 设备模型
** output parameters:   dyn_s_raw - 各机 S 值数组
** Returned value:      NONE
*********************************************************************************************************/
static void dynamic_suppress_read_raw(AntiRefluxController *ctrl,
                                      const AntiRefluxDeviceModel *model,
                                      double *dyn_s_raw)
{
    uint32_t i;
    uint32_t id;
    int      r;

    if (!ctrl || !model || !dyn_s_raw)
        return;
    for (i = 0; i < model->inverter_count; i++)
    {
        dyn_s_raw[i] = 0.0;
        if (!model->inverters[i].persist_enabled || !inv_dynamic_bound(&model->inverters[i]))
            continue;
        id = model->inverters[i].rw_dynamic_control.dblink_id;
        /*
         * 动态点配置为虚拟 DO/AO，与 writectrlpoint_by_id 同属控制点池；
         * readpoint_by_id 走实时测点表，会失败，本周期会被误判 S=0 并在周期末写回 0。
         */
        if (ctrl->read_ctrlpoint)
            r = ctrl->read_ctrlpoint(id, &dyn_s_raw[i]);
        else if (ctrl->read_point)
            r = ctrl->read_point(id, &dyn_s_raw[i]);
        else
            r = -1;
        /* 读失败视为 S=0（允许统筹），避免通信抖动长期锁死 */
        if (r != 0)
            dyn_s_raw[i] = 0.0;
        if (dyn_s_raw[i] < 0.0)
            dyn_s_raw[i] = 0.0;
    }
}

/*********************************************************************************************************
** Function name:       dynamic_suppress_writeback
** Descriptions:        周期末写回动态抑制倒计时 max(0, S-dt)
** input parameters:    ctrl, model, dt_ms, dyn_s_raw
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void dynamic_suppress_writeback(AntiRefluxController *ctrl,
                                       const AntiRefluxDeviceModel *model,
                                       uint32_t dt_ms,
                                       const double *dyn_s_raw)
{
    uint32_t i;
    uint32_t id;
    double   dt_sec, new_s;

    if (!ctrl || !model || !dyn_s_raw || !ctrl->write_ctrlpoint)
        return;
    dt_sec = (double)dt_ms / 1000.0;
    for (i = 0; i < model->inverter_count; i++)
    {
        if (!model->inverters[i].persist_enabled || !inv_dynamic_bound(&model->inverters[i]))
            continue;
        id = model->inverters[i].rw_dynamic_control.dblink_id;
        new_s = dyn_s_raw[i] - dt_sec;
        if (new_s < 0.0)
            new_s = 0.0;
        (void)ctrl->write_ctrlpoint(id, &new_s);
    }
}

/*********************************************************************************************************
** Function name:       anti_reflux_dynamic_suppress_bootstrap_zero
** Descriptions:        进程启动时将动态防逆流虚拟点 S 初始化为 0
** input parameters:    model - 设备模型；write_ctrlpoint_fn - 控制点写回调
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void anti_reflux_dynamic_suppress_bootstrap_zero(const AntiRefluxDeviceModel *model,
                                                 anti_reflux_write_ctrlpoint_fn write_ctrlpoint_fn)
{
    uint32_t i;
    uint32_t id;
    double   z = 0.0;

    if (!model || !write_ctrlpoint_fn)
        return;
    for (i = 0; i < model->inverter_count; i++)
    {
        if (!model->inverters[i].persist_enabled || !inv_dynamic_bound(&model->inverters[i]))
            continue;
        id = model->inverters[i].rw_dynamic_control.dblink_id;
        (void)write_ctrlpoint_fn(id, &z);
    }
}

/*********************************************************************************************************
** Function name:       read_meter_ptot
** Descriptions:        读电表总有功 Ptot
** input parameters:    model - 设备模型；read_fn - 读点回调
** output parameters:   out_w - 功率 W；valid - 是否有效
** Returned value:      0-成功；-1-参数无效
*********************************************************************************************************/
static int read_meter_ptot(const AntiRefluxDeviceModel *model,
                           anti_reflux_read_point_fn read_fn,
                           double *out_w, int *valid)
{
    const AntiRefluxStrategyPoint *pt;
    uint32_t id;

    if (!model || !read_fn || !out_w || !valid)
        return -1;
    *out_w = 0.0;
    *valid = 0;
    pt = &model->meter.read_points[(int)ANTI_REFLUX_METER_READ_Ptot];
    if (pt->is_user_fill)
    {
        *out_w = pt->user_value_num;
        *valid = 1;
        return 0;
    }
    id = pt->dblink_id;
    if (id == ANTI_REFLUX_DBLINK_ID_INVALID)
        return 0;
    if (read_fn(id, out_w) == 0)
        *valid = 1;
    return 0;
}

/*********************************************************************************************************
** Function name:       read_inverter_rated_w
** Descriptions:        读逆变器额定功率 RatedPowerW (W)
** input parameters:    inv - 逆变器模型；read_fn - 读点回调
** output parameters:   out_w - 额定功率 W
** Returned value:      0-成功；-1-参数无效
*********************************************************************************************************/
static int read_inverter_rated_w(const AntiRefluxInverterModel *inv,
                                 anti_reflux_read_point_fn read_fn,
                                 double *out_w)
{
    const AntiRefluxStrategyPoint *pt;
    uint32_t id;
    int idx = (int)ANTI_REFLUX_INV_READ_RatedPowerW;

    if (!inv || !read_fn || !out_w)
        return -1;
    *out_w = 0.0;
    pt = &inv->read_points[idx];
    if (pt->is_user_fill)
    {
        *out_w = pt->user_value_num;
        return 0;
    }
    id = pt->dblink_id;
    if (id == ANTI_REFLUX_DBLINK_ID_INVALID)
        return 0;
    (void)read_fn(id, out_w);
    return 0;
}

/*********************************************************************************************************
** Function name:       read_inverter_active_power_w
** Descriptions:        读逆变器当前有功 ActivePower (W)
** input parameters:    inv - 逆变器模型；read_fn - 读点回调
** output parameters:   out_w - 有功功率 W
** Returned value:      0-成功；-1-参数无效
*********************************************************************************************************/
static int read_inverter_active_power_w(const AntiRefluxInverterModel *inv,
                                        anti_reflux_read_point_fn read_fn,
                                        double *out_w)
{
    const AntiRefluxStrategyPoint *pt;
    uint32_t id;
    double val;
    int idx = (int)ANTI_REFLUX_INV_READ_ActivePower;

    if (!inv || !read_fn || !out_w)
        return -1;
    *out_w = 0.0;
    pt = &inv->read_points[idx];
    if (pt->is_user_fill)
    {
        *out_w = pt->user_value_num;
        return 0;
    }
    id = pt->dblink_id;
    if (id == ANTI_REFLUX_DBLINK_ID_INVALID)
        return 0;
    if (read_fn(id, &val) != 0)
        return 0;
    *out_w = val;
    return 0;
}

/*********************************************************************************************************
** Function name:       read_inverter_limit_w
** Descriptions:        读逆变器当前限功率 LimitPower (W)
** input parameters:    inv - 逆变器模型；read_fn - 读点回调
** output parameters:   out_w - 限功率 W
** Returned value:      0-成功；-1-参数无效
*********************************************************************************************************/
static int read_inverter_limit_w(const AntiRefluxInverterModel *inv,
                                 anti_reflux_read_point_fn read_fn,
                                 double *out_w)
{
    const AntiRefluxStrategyPoint *pt;
    uint32_t id;
    double val;
    int idx = (int)ANTI_REFLUX_INV_READ_LimitPower;

    if (!inv || !read_fn || !out_w)
        return -1;
    *out_w = 0.0;
    pt = &inv->read_points[idx];
    if (pt->is_user_fill)
    {
        *out_w = pt->user_value_num;
        return 0;
    }
    id = pt->dblink_id;
    if (id == ANTI_REFLUX_DBLINK_ID_INVALID)
        return 0;
    if (read_fn(id, &val) != 0)
        return 0;
    *out_w = val;
    return 0;
}

/*********************************************************************************************************
** Function name:       read_inverter_comm_ok
** Descriptions:        读逆变器通信状态 CommStatus
** input parameters:    inv - 逆变器模型；read_fn - 读点回调
** output parameters:   ok - 1-通信正常；0-异常
** Returned value:      0-成功；-1-参数无效
*********************************************************************************************************/
static int read_inverter_comm_ok(const AntiRefluxInverterModel *inv,
                                 anti_reflux_read_point_fn read_fn,
                                 int *ok)
{
    const AntiRefluxStrategyPoint *pt;
    uint32_t id;
    double val = 0.0;
    int idx = (int)ANTI_REFLUX_INV_READ_CommStatus;

    if (!inv || !read_fn || !ok)
        return -1;
    *ok = 1;
    pt = &inv->read_points[idx];
    if (pt->is_user_fill)
    {
        *ok = (pt->user_value_num > 0.5) ? 1 : 0;
        return 0;
    }
    id = pt->dblink_id;
    if (id == ANTI_REFLUX_DBLINK_ID_INVALID)
        return 0;
    if (read_fn(id, &val) == 0)
        *ok = (val > 0.5) ? 1 : 0;
    return 0;
}

/*********************************************************************************************************
** Function name:       read_any_point
** Descriptions:        按 dblink_id 读测点或控制点（先测点后控制点）
** input parameters:    ctrl - 控制器；id - dblink_id
** output parameters:   val - 读回值
** Returned value:      0-成功；非0-失败
*********************************************************************************************************/
static int read_any_point(AntiRefluxController *ctrl, uint32_t id, double *val)
{
    int r = -1;
    if (!ctrl || !val || id == ANTI_REFLUX_DBLINK_ID_INVALID)
        return -1;
    if (ctrl->read_point)
        r = ctrl->read_point(id, val);
    if (r != 0 && ctrl->read_ctrlpoint)
        r = ctrl->read_ctrlpoint(id, val);
    return r;
}

/*********************************************************************************************************
** Function name:       write_inverter_limit_w
** Descriptions:        写逆变器限功率使能与限功率值 (W)
** input parameters:    inv - 逆变器模型；write_fn - 写点回调；limit_w - 限功率 W
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void write_inverter_limit_w(const AntiRefluxInverterModel *inv,
                                   anti_reflux_write_command_op_fn write_cmd_fn,
                                   double limit_w)
{
    uint32_t id_en, id_val;

    if (!inv || !write_cmd_fn)
        return;
    id_en = inv->write_points[(int)ANTI_REFLUX_INV_WRITE_PowerLimitEnable].dblink_id;
    id_val = inv->write_points[(int)ANTI_REFLUX_INV_WRITE_PowerLimitValue].dblink_id;
    if (id_en != ANTI_REFLUX_DBLINK_ID_INVALID)
        write_cmd_fn(id_en, 1.0);
    if (limit_w < 0.0)
        limit_w = 0.0;
    if (id_val != ANTI_REFLUX_DBLINK_ID_INVALID)
        write_cmd_fn(id_val, limit_w);
}

/*********************************************************************************************************
** Function name:       quantize_to_step_w
** Descriptions:        将功率值按步长向下量化到步长整数倍
** input parameters:    value_w - 功率 W；step_w - 步长 W
** output parameters:   NONE
** Returned value:      量化后的功率 W
*********************************************************************************************************/
static double quantize_to_step_w(double value_w, double step_w)
{
    long n;

    if (step_w <= 0.0)
        return value_w;
    n = (long)(value_w / step_w);
    if (n < 0)
        n = 0;
    return (double)n * step_w;
}

/*********************************************************************************************************
** Function name:       anti_reflux_controller_init
** Descriptions:        初始化防逆流控制器及子模块状态
** input parameters:    ctrl, config, model, read_fn, write_fn, read_ctrlpoint_fn, write_ctrlpoint_fn
** output parameters:   ctrl - 初始化后的控制器
** Returned value:      NONE
*********************************************************************************************************/
void anti_reflux_controller_init(
    AntiRefluxController       *ctrl,
    const AntiRefluxConfig     *config,
    const AntiRefluxDeviceModel *model,
    anti_reflux_read_point_fn   read_fn,
    anti_reflux_write_point_fn write_fn,
    anti_reflux_read_ctrlpoint_fn read_ctrlpoint_fn,
    anti_reflux_write_ctrlpoint_fn write_ctrlpoint_fn)
{
    if (!ctrl)
        return;
    memset(ctrl, 0, sizeof(*ctrl));
    ctrl->config = config;
    ctrl->model = model;
    ctrl->read_point = read_fn;
    ctrl->write_point = write_fn;
    ctrl->read_ctrlpoint = read_ctrlpoint_fn;
    ctrl->write_ctrlpoint = write_ctrlpoint_fn;
    anti_reflux_input_init(&ctrl->input_state);
    anti_reflux_safety_init(&ctrl->safety_state);
    anti_reflux_sg_init(&ctrl->sg_ctx, model);
}

/*********************************************************************************************************
** Function name:       anti_reflux_controller_run
** Descriptions:        执行一个防逆流控制周期（读表/滤波/分配/写限功率/安全防线）
** input parameters:    ctrl - 控制器；now_ms - 当前单调时钟毫秒
** output parameters:   NONE
** Returned value:      0-成功；负值-各类错误码
*********************************************************************************************************/
int anti_reflux_controller_run(AntiRefluxController *ctrl, uint32_t now_ms)
{
    const AntiRefluxConfig *cfg;
    const AntiRefluxDeviceModel *model;
    double raw_ptot, filtered_ptot;
    double active_w[ANTI_REFLUX_MAX_INVERTERS];
    double rated_w[ANTI_REFLUX_MAX_INVERTERS];
    double limit_w[ANTI_REFLUX_MAX_INVERTERS];
    double current_limit_w[ANTI_REFLUX_MAX_INVERTERS]; /* 本周期初读回限值，供启动初始化/安全顶滞环/写判用 */
    double load_est_w, sum_active_w, sum_rated_w, desired_total_w, desired_i_w;
    double total_step_w, safe_ceiling_w, sum_limit_w;
    uint32_t dt_ms;
    int meter_valid;
    uint32_t i;
    int32_t target_w;
    int32_t min_running_pc;
    uint8_t coord_on[ANTI_REFLUX_MAX_INVERTERS];
    AntiRefluxSgInvInput sg_inputs[ANTI_REFLUX_MAX_INVERTERS];
    double dyn_s_raw[ANTI_REFLUX_MAX_INVERTERS];
    int ret = 0;

    if (!ctrl || !ctrl->config || !ctrl->model || !ctrl->read_point || !ctrl->write_command_op)
        return -1;
    cfg = ctrl->config;
    model = ctrl->model;

    if (!cfg->system.enable)
        return 0;
    if (model->inverter_count == 0 || model->inverter_count > ANTI_REFLUX_MAX_INVERTERS)
        return -2;

    if (now_ms == 0 || ctrl->last_cycle_ms == 0)
        dt_ms = cfg->system.ctrl_interval_ms;
    else if (now_ms > ctrl->last_cycle_ms)
        dt_ms = now_ms - ctrl->last_cycle_ms;
    else
        dt_ms = cfg->system.ctrl_interval_ms;
    ctrl->last_cycle_ms = now_ms;

    if (ctrl->debug)
        fprintf(stderr, "[anti_reflux] controller: cycle dt_ms=%u\n", (unsigned)dt_ms);

    /* 0) 动态抑制：先读本周期 S（统筹判定用），周期末在 cycle_end 写回 max(0,S-dt) */
    dynamic_suppress_read_raw(ctrl, model, dyn_s_raw);

    /* ---------- 1) 读电表 ---------- */
    if (read_meter_ptot(model, ctrl->read_point, &raw_ptot, &meter_valid) != 0)
    {
        ret = -3;
        goto cycle_end;
    }
    if (ctrl->debug)
        fprintf(stderr, "[anti_reflux] controller: 1) meter Ptot=%.1f W valid=%d\n", raw_ptot, meter_valid);

    /* ---------- 2) 输入滤波与方向 ---------- */
    anti_reflux_input_step(
        &ctrl->input_state,
        raw_ptot,
        cfg->input_processing.filter_alpha,
        cfg->input_processing.reverse_threshold_w,
        cfg->input_processing.direction_hysteresis_ms,
        dt_ms
    );
    filtered_ptot = ctrl->input_state.filtered_power_w;
    if (ctrl->debug)
        fprintf(stderr, "[anti_reflux] controller: 2) input filtered=%.1f W direction=%s\n",
                filtered_ptot, ctrl->input_state.direction ? "REVERSE" : "FORWARD");

    /* 有功：所有逆变器均参与负载估计；额定/限值统筹仅「动态开关开启」的逆变器 */
    for (i = 0; i < model->inverter_count; i++)
    {
        read_inverter_active_power_w(&model->inverters[i], ctrl->read_point, &active_w[i]);
        sg_inputs[i].inv_active_w = active_w[i];
        sg_inputs[i].inv_limit_cmd_w = ctrl->sg_ctx.inv[i].last_limit_cmd_w;
    }

    /* effective_on = persist_enabled && (!dynamic_bound || S<=0)；未绑定动态点时仅 persist 决定 */
    for (i = 0; i < model->inverter_count; i++)
    {
        int comm_ok = 1;
        double relay_state = 0.0;
        int relay_state_ok = 0;

        if (!model->inverters[i].persist_enabled)
        {
            coord_on[i] = 0;
            current_limit_w[i] = 0.0;
            rated_w[i] = 0.0;
            sg_inputs[i].inv_comm_ok = 0;
            sg_inputs[i].inv_limit_readback_w = 0.0;
            sg_inputs[i].relay_state_valid = 0;
            sg_inputs[i].relay_state_value = 0.0;
            sg_inputs[i].reverse_direction = ctrl->input_state.direction ? 1 : 0;
            continue;
        }
        (void)read_inverter_comm_ok(&model->inverters[i], ctrl->read_point, &comm_ok);
        sg_inputs[i].inv_comm_ok = comm_ok ? 1 : 0;
        if (!inv_dynamic_bound(&model->inverters[i]))
        {
            coord_on[i] = 1;
            read_inverter_limit_w(&model->inverters[i], ctrl->read_point, &current_limit_w[i]);
            read_inverter_rated_w(&model->inverters[i], ctrl->read_point, &rated_w[i]);
            sg_inputs[i].inv_limit_readback_w = current_limit_w[i];
            if (read_any_point(ctrl, model->inverters[i].safety_state_dblink_id, &relay_state) == 0)
            {
                relay_state_ok = 1;
            }
            sg_inputs[i].relay_state_valid = relay_state_ok ? 1 : 0;
            sg_inputs[i].relay_state_value = relay_state;
            sg_inputs[i].reverse_direction = ctrl->input_state.direction ? 1 : 0;
            continue;
        }
        coord_on[i] = (dyn_s_raw[i] <= 0.0) ? 1 : 0;
        if (!coord_on[i])
        {
            current_limit_w[i] = 0.0;
            rated_w[i] = 0.0;
            sg_inputs[i].inv_limit_readback_w = 0.0;
            sg_inputs[i].relay_state_valid = 0;
            sg_inputs[i].relay_state_value = 0.0;
            sg_inputs[i].reverse_direction = ctrl->input_state.direction ? 1 : 0;
            continue;
        }
        read_inverter_limit_w(&model->inverters[i], ctrl->read_point, &current_limit_w[i]);
        read_inverter_rated_w(&model->inverters[i], ctrl->read_point, &rated_w[i]);
        sg_inputs[i].inv_limit_readback_w = current_limit_w[i];
        if (read_any_point(ctrl, model->inverters[i].safety_state_dblink_id, &relay_state) == 0)
        {
            relay_state_ok = 1;
        }
        sg_inputs[i].relay_state_valid = relay_state_ok ? 1 : 0;
        sg_inputs[i].relay_state_value = relay_state;
        sg_inputs[i].reverse_direction = ctrl->input_state.direction ? 1 : 0;
    }

    (void)anti_reflux_sg_step(&ctrl->sg_ctx, cfg, model, sg_inputs, now_ms, dt_ms, ctrl->debug);
    for (i = 0; i < model->inverter_count; i++)
    {
        if (coord_on[i] && !anti_reflux_sg_allow_coord(&ctrl->sg_ctx, i))
            coord_on[i] = 0;
    }
    if (ctrl->debug && cfg->safety_guard.enable)
    {
        fprintf(stderr, "[anti_reflux][sg] states");
        for (i = 0; i < model->inverter_count; i++)
        {
            const AntiRefluxSgInvRuntime *st = &ctrl->sg_ctx.inv[i];
            const AntiRefluxInverterModel *inv = &model->inverters[i];

            fprintf(stderr,
                    " [%u]=%s coord=%u ptrip=%u pclose=%u seq=%s step=%u relay_fb=%.0f(valid=%u)"
                    " S1=0x%08X S2=0x%08X",
                    (unsigned)i,
                    anti_reflux_sg_state_name(st->state),
                    (unsigned)st->allow_coord,
                    (unsigned)st->trip_cmd_pending,
                    (unsigned)st->close_cmd_pending,
                    anti_reflux_sg_relay_seq_name(st->relay_seq),
                    (unsigned)st->relay_seq_step,
                    sg_inputs[i].relay_state_value,
                    (unsigned)sg_inputs[i].relay_state_valid,
                    (unsigned)inv->safety_trip_dblink_id,
                    (unsigned)inv->safety_close_dblink_id);
        }
        fprintf(stderr, "\n");
    }

    /* ---------- 3) 算负载与总目标 ---------- */
    sum_active_w = 0.0;
    sum_rated_w = 0.0;
    for (i = 0; i < model->inverter_count; i++)
    {
        sum_active_w += active_w[i];
        if (rated_w[i] > 0.0)
            sum_rated_w += rated_w[i];
    }

    load_est_w = sum_active_w + raw_ptot;
    target_w = cfg->input_processing.target_power_w;
    /* 正向：爬坡目标仍为满发，便于功率恢复；分配后再用「不逆流安全顶」封顶，避免 n+1 步震荡。
     * 逆流：目标为 load_est+target，向下取整到颗粒度，优先保证不逆流。 */
    if (!ctrl->input_state.direction) /* FORWARD */
        desired_total_w = sum_rated_w;
    else
    {
        desired_total_w = load_est_w + (double)target_w;
        if (desired_total_w < 0.0)
            desired_total_w = 0.0;
        if (sum_rated_w > 0.0 && desired_total_w > sum_rated_w)
            desired_total_w = sum_rated_w;
    }

    /* 有效颗粒度：仅统筹参与机；取各机最小步长作为总目标量化步长（若有一台配置了步长） */
    total_step_w = 0.0;
    for (i = 0; i < model->inverter_count; i++)
    {
        double sw = model->inverters[i].limit_power_step_w;
        if (!coord_on[i])
            continue;
        if (sw > 0.0 && (total_step_w <= 0.0 || sw < total_step_w))
            total_step_w = sw;
    }
    /* 逆流时总目标向下取整到颗粒度；正向时爬坡后再量化 */
    if (total_step_w > 0.0 && ctrl->input_state.direction)
        desired_total_w = quantize_to_step_w(desired_total_w, total_step_w);
    /* 正向时预先算「不逆流安全顶」供分配后封顶用；若读回限值已高于 floor(load_est+target) 且未逆流，则不往下调 */
    safe_ceiling_w = sum_rated_w;
    if (!ctrl->input_state.direction && total_step_w > 0.0)
    {
        double raw = load_est_w + (double)target_w;
        double candidate_w;
        double current_total_limit_w;
        if (raw < 0.0) raw = 0.0;
        if (raw > sum_rated_w) raw = sum_rated_w;
        candidate_w = quantize_to_step_w(raw, total_step_w);
        current_total_limit_w = 0.0;
        for (i = 0; i < model->inverter_count; i++)
        {
            if (!coord_on[i])
                continue;
            current_total_limit_w += current_limit_w[i];
        }
        /* 当前总限值已高于本档且电表明显正向（无逆流）时，才保持当前档不往下调；逆流时必须允许下调 */
        if (candidate_w < current_total_limit_w && raw >= candidate_w && filtered_ptot > 0.0)
            safe_ceiling_w = current_total_limit_w;
        else
            safe_ceiling_w = candidate_w;
    }

    if (ctrl->debug)
        fprintf(stderr, "[anti_reflux] controller: 3) load_est=%.0f W desired_total=%.0f W (target=%d W)\n",
                load_est_w, desired_total_w, (int)target_w);

    /* ---------- 4) 爬坡与电表故障 ---------- */
    /* 进程刚启动时爬坡状态为 0，若设备读回限值已非 0 且正向，则用读回值初始化爬坡，避免首周期误把限值拉低 */
    if (!ctrl->input_state.direction && sum_rated_w >= 1e-6 && ctrl->safety_state.current_limit_pc <= 0.0)
    {
        sum_limit_w = 0.0;
        for (i = 0; i < model->inverter_count; i++)
        {
            if (!coord_on[i])
                continue;
            sum_limit_w += current_limit_w[i];
        }
        if (sum_limit_w > 0.0)
            ctrl->safety_state.current_limit_pc = 100.0 * sum_limit_w / sum_rated_w;
    }
    if (sum_rated_w >= 1e-6)
    {
        double desired_pc = 100.0 * desired_total_w / sum_rated_w;
        double ramped_pc;

        if (desired_pc > (double)cfg->output_safety.max_total_limit_pc)
            desired_pc = (double)cfg->output_safety.max_total_limit_pc;
        if (desired_pc < 0.0)
            desired_pc = 0.0;

        ramped_pc = anti_reflux_safety_step(
            &ctrl->safety_state,
            desired_pc,
            (bool)(!ctrl->input_state.direction),
            cfg->output_safety.ramp_up_pc_per_sec,
            cfg->output_safety.ramp_down_pc_per_sec,
            (bool)meter_valid,
            cfg->output_safety.meter_fail_timeout_s,
            cfg->output_safety.meter_fail_action,
            cfg->output_safety.max_total_limit_pc,
            dt_ms / 1000.0
        );

        if (!meter_valid && cfg->output_safety.meter_fail_timeout_s > 0 &&
            ctrl->safety_state.meter_invalid_sec >= (double)cfg->output_safety.meter_fail_timeout_s)
        {
            for (i = 0; i < model->inverter_count; i++)
            {
                if (!coord_on[i])
                    continue;
                write_inverter_limit_w(&model->inverters[i], ctrl->write_command_op, 0.0);
            }
            if (ctrl->debug)
                fprintf(stderr, "[anti_reflux] controller: meter_fail -> participating inv limit 0 W\n");
            ret = 0;
            goto cycle_end;
        }
        desired_total_w = (ramped_pc / 100.0) * sum_rated_w;
        /* 爬坡后的总目标按颗粒度量化，与各机 limit_power_step_w 一致，避免分配后再量化产生抖动；
         * 爬坡系数 ramp_*_pc_per_sec 为 %/s，实际功率斜率 = ramp * sum_rated_w/100 (W/s)。
         * 若希望每周期至少能增加一个步长，宜满足 ramp_up_pc_per_sec >= 100*total_step_w/(sum_rated_w*周期秒数)。 */
        if (total_step_w > 0.0)
            desired_total_w = quantize_to_step_w(desired_total_w, total_step_w);
    }
    else
    {
        (void)anti_reflux_safety_step(
            &ctrl->safety_state,
            0.0,
            (bool)(!ctrl->input_state.direction),
            cfg->output_safety.ramp_up_pc_per_sec,
            cfg->output_safety.ramp_down_pc_per_sec,
            (bool)meter_valid,
            cfg->output_safety.meter_fail_timeout_s,
            cfg->output_safety.meter_fail_action,
            cfg->output_safety.max_total_limit_pc,
            dt_ms / 1000.0
        );
        if (!meter_valid && cfg->output_safety.meter_fail_timeout_s > 0 &&
            ctrl->safety_state.meter_invalid_sec >= (double)cfg->output_safety.meter_fail_timeout_s)
        {
            for (i = 0; i < model->inverter_count; i++)
            {
                if (!coord_on[i])
                    continue;
                write_inverter_limit_w(&model->inverters[i], ctrl->write_command_op, 0.0);
            }
            if (ctrl->debug)
                fprintf(stderr, "[anti_reflux] controller: meter_fail -> participating inv limit 0 W\n");
            ret = 0;
            goto cycle_end;
        }
        desired_total_w = 0.0;
    }

    if (ctrl->debug)
        fprintf(stderr, "[anti_reflux] controller: 4) desired_total(ramped)=%.0f W\n", desired_total_w);

    /* ---------- 5) 按容量分配并按各机颗粒度量化 ---------- */
    min_running_pc = cfg->inverter_groups.min_running_pc;
    if (sum_rated_w < 1e-6)
    {
        for (i = 0; i < model->inverter_count; i++)
            limit_w[i] = 0.0;
    }
    else if (desired_total_w >= sum_rated_w - 1.0)
    {
        for (i = 0; i < model->inverter_count; i++)
        {
            if (!coord_on[i])
                limit_w[i] = 0.0;
            else
                limit_w[i] = rated_w[i];
        }
    }
    else
    {
        for (i = 0; i < model->inverter_count; i++)
        {
            double step_w = model->inverters[i].limit_power_step_w;

            if (rated_w[i] <= 0.0)
            {
                limit_w[i] = 0.0;
                continue;
            }
            desired_i_w = desired_total_w * (rated_w[i] / sum_rated_w);
            if (desired_i_w < 0.0)
                desired_i_w = 0.0;
            if (desired_i_w > rated_w[i])
                desired_i_w = rated_w[i];

            desired_i_w = quantize_to_step_w(desired_i_w, step_w > 0.0 ? step_w : 0.0);

            if (min_running_pc > 0 && desired_i_w > 0.0 &&
                desired_i_w < (double)min_running_pc * 0.01 * rated_w[i] &&
                desired_total_w >= sum_rated_w * 0.5)
                desired_i_w = 0.0;

            limit_w[i] = desired_i_w;
        }
    }

    /* 正向时：若分配后总限值超过不逆流安全顶，按比例缩到安全顶并重新量化，避免 n+1 步逆流震荡 */
    if (!ctrl->input_state.direction && total_step_w > 0.0)
    {
        sum_limit_w = 0.0;
        for (i = 0; i < model->inverter_count; i++)
            sum_limit_w += limit_w[i];
        if (sum_limit_w > safe_ceiling_w && sum_limit_w >= 1.0)
        {
            for (i = 0; i < model->inverter_count; i++)
            {
                double step_w = model->inverters[i].limit_power_step_w;
                if (!coord_on[i])
                    continue;
                desired_i_w = limit_w[i] * (safe_ceiling_w / sum_limit_w);
                if (desired_i_w > rated_w[i])
                    desired_i_w = rated_w[i];
                limit_w[i] = quantize_to_step_w(desired_i_w, step_w > 0.0 ? step_w : 0.0);
            }
        }
    }

    if (ctrl->debug)
    {
        fprintf(stderr, "[anti_reflux] controller: 5) active_w");
        for (i = 0; i < model->inverter_count; i++)
            fprintf(stderr, " [%u]=%.0f", (unsigned)i, active_w[i]);
        fprintf(stderr, "\n[anti_reflux] controller: 6) rated_w");
        for (i = 0; i < model->inverter_count; i++)
            fprintf(stderr, " [%u]=%.0f", (unsigned)i, rated_w[i]);
        fprintf(stderr, "\n[anti_reflux] controller: 7) limit_w(W)");
        for (i = 0; i < model->inverter_count; i++)
            fprintf(stderr, " [%u]=%.0f", (unsigned)i, limit_w[i]);
        fprintf(stderr, "\n");
    }

    /* ---------- 6) 判写（current_limit_w 已在周期初读回） ---------- */
    {
        double diff;

        if (ctrl->debug)
        {
            fprintf(stderr, "[anti_reflux] controller: 8) current_limit_w(readback)");
            for (i = 0; i < model->inverter_count; i++)
                fprintf(stderr, " [%u]=%.0f", (unsigned)i, current_limit_w[i]);
            fprintf(stderr, "\n");
        }
        for (i = 0; i < model->inverter_count; i++)
        {
            if (!coord_on[i])
                continue;
            if (limit_w[i] > rated_w[i])
                limit_w[i] = rated_w[i];
            diff = limit_w[i] - current_limit_w[i];
            if (diff < 0.0)
                diff = -diff;
            if (current_limit_w[i] > rated_w[i] && limit_w[i] <= rated_w[i])
            {
                write_inverter_limit_w(&model->inverters[i], ctrl->write_command_op, limit_w[i]);
                continue;
            }
            /* 逆流很大时算法会限到 0：仅当读回值非 0 时才写 0，与下面一致都用读到的值判写 */
            if (limit_w[i] <= LIMIT_W_WRITE_TOLERANCE_W)
            {
                if (current_limit_w[i] > LIMIT_W_WRITE_TOLERANCE_W)
                    write_inverter_limit_w(&model->inverters[i], ctrl->write_command_op, 0.0);
                continue;
            }
            if (diff > LIMIT_W_WRITE_TOLERANCE_W)
                write_inverter_limit_w(&model->inverters[i], ctrl->write_command_op, limit_w[i]);
        }
    }
    if (ctrl->debug)
        fprintf(stderr, "[anti_reflux] controller: 9) write done, inv_count=%u\n", (unsigned)model->inverter_count);

    ret = 0;

cycle_end:
    (void)anti_reflux_sg_flush_commands(&ctrl->sg_ctx, cfg, model, ctrl->write_command_op, now_ms,
                                       ctrl->debug);
    dynamic_suppress_writeback(ctrl, model, dt_ms, dyn_s_raw);
    return ret;
}
