/**
 * @file anti_reflux_cfg_load.c
 * @brief 防逆流策略 JSON 配置加载实现
 *
 * 解析 strategy_control.json、channel、设备 cfg_file，将结果写入调用方传入的
 * AntiRefluxConfig / AntiRefluxDeviceModel。以下层级与 config/ 目录下实测 JSON 一致：
 *
 * 1) strategy/strategy_control.json
 *    - 根为数组，取 [0]
 *    - [0].StrategyId, [0].DisplayName, [0].config, [0].MeterRef, [0].InverterRefs
 *    - [0].config: version, system{}, input_processing{}, output_safety{}, inverter_groups{}（新方案无 pid_algorithm）
 *    - [0].MeterRef / [0].InverterRefs[]: ChnlId, DevIndex, IsEmpty；可选 enable（缺省 true，持久化是否参与防逆流）
 *    - DevIndex 表示通道内设备号，与 chnl_N.json 中 attr.devices.attr[].logic_id 匹配（非数组下标）
 *
 * 2) sample/channels/chnl_N.json
 *    - 根: Name, info, attr
 *    - attr.devices.attr[] 中 logic_id 与策略中的 DevIndex 匹配，取该项的 cfg_file 为设备 cfg 路径
 *
 * 3) 设备 cfg（如 sample/devices/C1_Dx_modbusRTU.json）
 *    - 根: desc, devName, info, active_strategy, attr, ...
 *    - strategy_points 在 attr 下：attr.strategy_points.group[]
 *    - group[].strategy_id, group[].read[], group[].write[], group[].read_write[]（可选）
 *    - 每项含 key, datattr, slot_index；以及 value_source("slot"|"user_fill"), user_value, IsUserFill
 *    - read_write[] 与 read/write 字段格式相同；key=DynamicAntiRefluxEnable 为动态防逆流开关（虚拟 DO/AO）
 *
 * 编译时需提供 cJSON 头文件路径（如 -I path/to/cJSON）。
 */

#include "anti_reflux_cfg_load.h"
#include "db_cfg_json.h"
#include "cJSON.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/** slot_index 未绑定时在 JSON 中为 -1，存储为 0xFFFF */
#define ANTI_REFLUX_SLOT_UNBOUND  0xFFFF

/** 策略 ID 字符串 */
#define STRATEGY_ID_ANTI_REFLUX   "anti_reflux"

/*********************************************************************************************************
** Function name:       parse_hex_u16_str
** Descriptions:        从 JSON 对象解析 4 位十六进制字符串为 uint16
** input parameters:    obj - JSON 对象；key - 字段名
** output parameters:   out - 解析结果
** Returned value:      0-成功；-1-失败
*********************************************************************************************************/
static int parse_hex_u16_str(const cJSON *obj, const char *key, uint16_t *out)
{
    cJSON *item;
    const char *s;
    char *endptr;
    unsigned long v;
    size_t len;
    size_t i;

    if (!obj || !key || !out)
        return -1;

    item = cJSON_GetObjectItem((cJSON *)obj, key);
    if (!item || !cJSON_IsString(item) || !item->valuestring)
        return -1;

    s = item->valuestring;
    len = strlen(s);
    if (len != 4)
        return -1;
    for (i = 0; i < len; i++)
    {
        if (!isxdigit((unsigned char)s[i]))
            return -1;
    }

    v = strtoul(s, &endptr, 16);
    if (!endptr || *endptr != '\0' || v > 0xFFFFUL)
        return -1;

    *out = (uint16_t)v;
    return 0;
}

/*********************************************************************************************************
** Function name:       parse_config_obj
** Descriptions:        从 strategy_control 的 config 对象解析到 pCfg
** input parameters:    config - cJSON config 对象
** output parameters:   pCfg - 输出结构体
** Returned value:      0-成功；-1-参数无效或失败
*********************************************************************************************************/
static int parse_config_obj(cJSON *config, AntiRefluxConfig *pCfg)
{
    cJSON *item;
    cJSON *sub;

    if (!config || !pCfg)
        return -1;

    memset(pCfg, 0, sizeof(*pCfg));

    item = cJSON_GetObjectItem(config, "version");
    if (item && item->valuestring)
        snprintf(pCfg->version, sizeof(pCfg->version), "%s", item->valuestring);

    sub = cJSON_GetObjectItem(config, "system");
    if (sub)
    {
        item = cJSON_GetObjectItem(sub, "ctrl_interval_ms");
        if (item)
            pCfg->system.ctrl_interval_ms = (uint32_t)item->valueint;
        item = cJSON_GetObjectItem(sub, "algo_mode");
        if (item && item->valuestring)
            snprintf(pCfg->system.algo_mode, sizeof(pCfg->system.algo_mode), "%s", item->valuestring);
        item = cJSON_GetObjectItem(sub, "enable");
        if (item)
            pCfg->system.enable = (item->type == cJSON_True || item->valueint) ? 1 : 0;
    }

    sub = cJSON_GetObjectItem(config, "input_processing");
    if (sub)
    {
        item = cJSON_GetObjectItem(sub, "filter_alpha");
        if (item)
            pCfg->input_processing.filter_alpha = item->valuedouble;
        item = cJSON_GetObjectItem(sub, "target_power_w");
        if (item)
            pCfg->input_processing.target_power_w = (int32_t)item->valueint;
        item = cJSON_GetObjectItem(sub, "reverse_threshold_w");
        if (item)
            pCfg->input_processing.reverse_threshold_w = (int32_t)item->valueint;
        item = cJSON_GetObjectItem(sub, "direction_hysteresis_ms");
        if (item)
            pCfg->input_processing.direction_hysteresis_ms = (uint32_t)item->valueint;
    }

    sub = cJSON_GetObjectItem(config, "output_safety");
    if (sub)
    {
        item = cJSON_GetObjectItem(sub, "ramp_up_pc_per_sec");
        if (item)
            pCfg->output_safety.ramp_up_pc_per_sec = (int32_t)item->valueint;
        item = cJSON_GetObjectItem(sub, "ramp_down_pc_per_sec");
        if (item)
            pCfg->output_safety.ramp_down_pc_per_sec = (int32_t)item->valueint;
        item = cJSON_GetObjectItem(sub, "meter_fail_timeout_s");
        if (item)
            pCfg->output_safety.meter_fail_timeout_s = (uint32_t)item->valueint;
        item = cJSON_GetObjectItem(sub, "meter_fail_action");
        if (item && item->valuestring)
            snprintf(pCfg->output_safety.meter_fail_action, sizeof(pCfg->output_safety.meter_fail_action), "%s", item->valuestring);
        item = cJSON_GetObjectItem(sub, "max_total_limit_pc");
        if (item)
            pCfg->output_safety.max_total_limit_pc = (int32_t)item->valueint;
    }

    sub = cJSON_GetObjectItem(config, "inverter_groups");
    if (sub)
    {
        item = cJSON_GetObjectItem(sub, "strategy");
        if (item && item->valuestring)
            snprintf(pCfg->inverter_groups.strategy, sizeof(pCfg->inverter_groups.strategy), "%s", item->valuestring);
        item = cJSON_GetObjectItem(sub, "min_running_pc");
        if (item)
            pCfg->inverter_groups.min_running_pc = (int32_t)item->valueint;
        item = cJSON_GetObjectItem(sub, "devices");
        if (item && cJSON_IsArray(item))
        {
            int n = cJSON_GetArraySize(item);
            uint32_t i;
            if (n > (int)ANTI_REFLUX_MAX_DEVICES)
                n = (int)ANTI_REFLUX_MAX_DEVICES;
            pCfg->inverter_groups.device_count = (uint32_t)n;
            for (i = 0; i < (uint32_t)n; i++)
            {
                cJSON *dev = cJSON_GetArrayItem(item, (int)i);
                cJSON *cid = dev ? cJSON_GetObjectItem(dev, "ChnlId") : NULL;
                cJSON *did = dev ? cJSON_GetObjectItem(dev, "DevIndex") : NULL;
                cJSON *empty = dev ? cJSON_GetObjectItem(dev, "IsEmpty") : NULL;
                if (cid)
                    pCfg->inverter_groups.devices[i].ChnlId = (int32_t)cid->valueint;
                if (did)
                    pCfg->inverter_groups.devices[i].DevIndex = (int32_t)did->valueint;
                if (empty)
                    pCfg->inverter_groups.devices[i].IsEmpty = (empty->type == cJSON_True || empty->valueint) ? 1 : 0;
            }
        }
    }

    sub = cJSON_GetObjectItem(config, "safety_guard");
    if (sub)
    {
        item = cJSON_GetObjectItem(sub, "enable");
        if (item)
            pCfg->safety_guard.enable = (item->type == cJSON_True || item->valueint) ? 1 : 0;
        item = cJSON_GetObjectItem(sub, "comm_fail_cycles_light");
        if (item)
            pCfg->safety_guard.comm_fail_cycles_light = (uint32_t)item->valueint;
        item = cJSON_GetObjectItem(sub, "comm_fail_cycles_hard");
        if (item)
            pCfg->safety_guard.comm_fail_cycles_hard = (uint32_t)item->valueint;
        item = cJSON_GetObjectItem(sub, "limit_mismatch_w");
        if (item)
            pCfg->safety_guard.limit_mismatch_w = (int32_t)item->valueint;
        item = cJSON_GetObjectItem(sub, "limit_mismatch_cycles");
        if (item)
            pCfg->safety_guard.limit_mismatch_cycles = (uint32_t)item->valueint;
        item = cJSON_GetObjectItem(sub, "effect_timeout_s");
        if (item)
            pCfg->safety_guard.effect_timeout_s = (uint32_t)item->valueint;
        item = cJSON_GetObjectItem(sub, "reverse_risk_confirm_s");
        if (item)
            pCfg->safety_guard.reverse_risk_confirm_s = (uint32_t)item->valueint;
        item = cJSON_GetObjectItem(sub, "trip_confirm_s");
        if (item)
            pCfg->safety_guard.trip_confirm_s = (uint32_t)item->valueint;
        item = cJSON_GetObjectItem(sub, "trip_hold_min_s");
        if (item)
            pCfg->safety_guard.trip_hold_min_s = (uint32_t)item->valueint;
        item = cJSON_GetObjectItem(sub, "reclose_cooldown_s");
        if (item)
            pCfg->safety_guard.reclose_cooldown_s = (uint32_t)item->valueint;
        item = cJSON_GetObjectItem(sub, "max_trips_per_hour");
        if (item)
            pCfg->safety_guard.max_trips_per_hour = (uint32_t)item->valueint;
        item = cJSON_GetObjectItem(sub, "manual_reclose_only");
        if (item)
            pCfg->safety_guard.manual_reclose_only = (item->type == cJSON_True || item->valueint) ? 1 : 0;
        item = cJSON_GetObjectItem(sub, "relay_command_interval_ms");
        if (item)
            pCfg->safety_guard.relay_command_interval_ms = (uint32_t)item->valueint;
        item = cJSON_GetObjectItem(sub, "notes");
        if (item && item->valuestring)
            snprintf(pCfg->safety_guard.notes, sizeof(pCfg->safety_guard.notes), "%s", item->valuestring);
    }

    return 0;
}

/*********************************************************************************************************
** Function name:       path_chnl_subdir
** Descriptions:        根据配置根路径、子目录与通道号生成 channel JSON 路径
** input parameters:    buf, buf_size, config_base, subdir, chnl_id
** output parameters:   buf - 输出路径
** Returned value:      NONE
*********************************************************************************************************/
static void path_chnl_subdir(char *buf, size_t buf_size, const char *config_base, const char *subdir, int chnl_id)
{
    snprintf(buf, buf_size, "%s/%s/chnl_%d.json", config_base, subdir, chnl_id);
}

/*********************************************************************************************************
** Function name:       get_cfg_file_from_channel
** Descriptions:        从通道 JSON 中按 DevIndex 取设备 cfg_file 路径
** input parameters:    channel_path, dev_index, out_size
** output parameters:   cfg_file_out - 输出 cfg 路径
** Returned value:      0-成功；-1-失败
*********************************************************************************************************/
static int get_cfg_file_from_channel(const char *channel_path, int dev_index, char *cfg_file_out, size_t out_size)
{
    char *p;
    int   size, ret = -1;
    int   i, n;
    cJSON *root, *attr, *devices, *attr_arr, *dev_obj, *cfg, *logic_id_js;

    p = db_cfg_jsonfile_alloc((char *)channel_path, &size);
    if (!p)
    {
        fprintf(stderr, "[anti_reflux] channel file not found: %s\n", channel_path);
        return -1;
    }
    p = db_cfg_jsonfile_read((char *)channel_path, p, size);
    if (!p)
    {
        fprintf(stderr, "[anti_reflux] channel file read failed: %s\n", channel_path);
        goto __free;
    }

    root = cJSON_Parse(p);
    if (!root)
    {
        fprintf(stderr, "[anti_reflux] channel JSON parse failed: %s\n", channel_path);
        goto __free;
    }

    attr = cJSON_GetObjectItem(root, "attr");
    if (!attr)
    {
        fprintf(stderr, "[anti_reflux] channel JSON missing attr: %s\n", channel_path);
        goto __del;
    }
    devices = cJSON_GetObjectItem(attr, "devices");
    if (!devices)
    {
        fprintf(stderr, "[anti_reflux] channel JSON missing attr.devices: %s\n", channel_path);
        goto __del;
    }
    attr_arr = cJSON_GetObjectItem(devices, "attr");
    if (!attr_arr || !cJSON_IsArray(attr_arr))
    {
        fprintf(stderr, "[anti_reflux] channel JSON missing or invalid attr.devices.attr: %s\n", channel_path);
        goto __del;
    }

    n = cJSON_GetArraySize(attr_arr);
    dev_obj = NULL;
    for (i = 0; i < n; i++)
    {
        cJSON *item = cJSON_GetArrayItem(attr_arr, i);
        if (!item)
            continue;
        logic_id_js = cJSON_GetObjectItem(item, "logic_id");
        if (!logic_id_js)
            continue;
        if (cJSON_IsNumber(logic_id_js))
        {
            if ((int)logic_id_js->valueint == dev_index)
            {
                dev_obj = item;
                break;
            }
        }
        else if (cJSON_IsString(logic_id_js) && logic_id_js->valuestring)
        {
            if (atoi(logic_id_js->valuestring) == dev_index)
            {
                dev_obj = item;
                break;
            }
        }
    }

    if (!dev_obj)
    {
        fprintf(stderr, "[anti_reflux] channel has no device with logic_id matching DevIndex=%d: %s\n",
                dev_index, channel_path);
        fprintf(stderr, "[anti_reflux] check strategy_control MeterRef/InverterRefs; channel devices use logic_id in chnl_*.json\n");
        goto __del;
    }
    cfg = cJSON_GetObjectItem(dev_obj, "cfg_file");
    if (!cfg || !cfg->valuestring)
    {
        fprintf(stderr, "[anti_reflux] channel device (logic_id=%d) cfg_file missing: %s\n", dev_index, channel_path);
        goto __del;
    }

    snprintf(cfg_file_out, out_size, "%s", cfg->valuestring);
    ret = 0;

__del:
    cJSON_Delete(root);
__free:
    db_cfg_jsonfile_delete(p);
    return ret;
}

/*********************************************************************************************************
** Function name:       fill_point
** Descriptions:        根据 key/datattr/slot_index 填充策略测点并生成 dblink_id
** input parameters:    pt, point_index, key, datattr, slot_index, chnl_id, dev_index
** output parameters:   pt - 填充后的测点
** Returned value:      NONE
*********************************************************************************************************/
static void fill_point(AntiRefluxStrategyPoint *pt, int point_index, const char *key, int datattr, int slot_index,
                       int chnl_id, int dev_index)
{
    if (!pt || !key)
        return;
    snprintf(pt->point_name, sizeof(pt->point_name), "%s", key);
    pt->point_index = (uint16_t)point_index;
    pt->datattr = (uint16_t)(datattr & 0xFF);
    pt->is_user_fill = 0;
    pt->user_value_num = 0.0;
    if (slot_index < 0)
    {
        pt->slot_index = (uint16_t)ANTI_REFLUX_SLOT_UNBOUND;
        pt->dblink_id = ANTI_REFLUX_DBLINK_ID_INVALID;
    }
    else
    {
        pt->slot_index = (uint16_t)(slot_index & 0xFF);
        pt->dblink_id = ANTI_REFLUX_DBLINK_ID_BUILD(chnl_id, dev_index, datattr, slot_index);
    }
}

/*********************************************************************************************************
** Function name:       meter_read_index_by_key
** Descriptions:        按 key 匹配电表防逆流读点序号
** input parameters:    key - 测点 key 字符串
** output parameters:   NONE
** Returned value:      0~10-对应下标；-1-未匹配
*********************************************************************************************************/
static int meter_read_index_by_key(const char *key)
{
    static const char *keys[] = { "Ua", "Ub", "Uc", "Ia", "Ib", "Ic", "Pa", "Pb", "Pc", "Ptot", "Freq" };
    int i;
    for (i = 0; i < ANTI_REFLUX_METER_READ_POINTS; i++)
        if (key && strcmp(keys[i], key) == 0)
            return i;
    return -1;
}

/*********************************************************************************************************
** Function name:       inv_read_index_by_key
** Descriptions:        按 key 匹配逆变器防逆流读点序号
** input parameters:    key - 测点 key 字符串
** output parameters:   NONE
** Returned value:      0~3-对应下标；-1-未匹配
*********************************************************************************************************/
static int inv_read_index_by_key(const char *key)
{
    static const char *keys[] = { "CommStatus", "ActivePower", "LimitPower", "RatedPowerW" };
    int i;
    for (i = 0; i < ANTI_REFLUX_INV_READ_POINTS; i++)
        if (key && strcmp(keys[i], key) == 0)
            return i;
    return -1;
}

/*********************************************************************************************************
** Function name:       inv_write_index_by_key
** Descriptions:        按 key 匹配逆变器防逆流写点序号
** input parameters:    key - 测点 key 字符串
** output parameters:   NONE
** Returned value:      0~3-对应下标；-1-未匹配
*********************************************************************************************************/
static int inv_write_index_by_key(const char *key)
{
    static const char *keys[] = { "PowerLimitEnable", "Start", "Stop", "PowerLimitValue" };
    int i;
    for (i = 0; i < ANTI_REFLUX_INV_WRITE_POINTS; i++)
        if (key && strcmp(keys[i], key) == 0)
            return i;
    return -1;
}

/*********************************************************************************************************
** Function name:       load_meter_points_from_cfg
** Descriptions:        从设备 cfg 的 strategy_points 填充电表模型及 dblink_id
** input parameters:    cfg_file, chnl_id, dev_index
** output parameters:   pMeter - 输出电表策略模型
** Returned value:      0-成功；-1-失败
*********************************************************************************************************/
static int load_meter_points_from_cfg(const char *cfg_file, int chnl_id, int dev_index, AntiRefluxMeterModel *pMeter)
{
    char *p;
    int   size, ret = -1;
    cJSON *root, *sp, *groups, *grp, *read_arr, *item;
    int   i, idx, datattr, slot_index, effective_slot, is_user_fill;
    double user_val_num;
    const char *key;
    cJSON *j_val_src, *j_user_val, *j_is_uf;

    p = db_cfg_jsonfile_alloc((char *)cfg_file, &size);
    if (!p)
    {
        fprintf(stderr, "[anti_reflux] meter cfg file not found: %s\n", cfg_file);
        return -1;
    }
    p = db_cfg_jsonfile_read((char *)cfg_file, p, size);
    if (!p)
    {
        fprintf(stderr, "[anti_reflux] meter cfg file read failed: %s\n", cfg_file);
        goto __free;
    }

    /* 跳过 UTF-8 BOM(EF BB BF)，避免 cJSON 解析失败 */
    if (size >= 3 && (unsigned char)p[0] == 0xEF && (unsigned char)p[1] == 0xBB && (unsigned char)p[2] == 0xBF)
    {
        memmove(p, p + 3, (size_t)(size - 3));
        p[size - 3] = '\0';
    }

    root = cJSON_Parse(p);
    if (!root)
    {
        fprintf(stderr, "[anti_reflux] meter cfg JSON parse failed: %s\n", cfg_file);
        goto __free;
    }

    /* 设备 cfg 中 strategy_points 在 root.attr 下 */
    sp = cJSON_GetObjectItem(root, "attr");
    if (sp)
        sp = cJSON_GetObjectItem(sp, "strategy_points");
    if (!sp)
    {
        fprintf(stderr, "[anti_reflux] meter cfg missing attr.strategy_points: %s\n", cfg_file);
        goto __del;
    }
    groups = cJSON_GetObjectItem(sp, "group");
    if (!groups || !cJSON_IsArray(groups))
        goto __del;

    /* 找 strategy_id == "anti_reflux" 的 group */
    for (i = 0; i < cJSON_GetArraySize(groups); i++)
    {
        grp = cJSON_GetArrayItem(groups, i);
        item = cJSON_GetObjectItem(grp, "strategy_id");
        if (item && item->valuestring && strcmp(item->valuestring, STRATEGY_ID_ANTI_REFLUX) == 0)
            break;
        grp = NULL;
    }
    if (!grp)
    {
        fprintf(stderr, "[anti_reflux] meter cfg missing strategy_points.group[strategy_id=anti_reflux]: %s\n", cfg_file);
        goto __del;
    }
    read_arr = cJSON_GetObjectItem(grp, "read");
    if (!read_arr || !cJSON_IsArray(read_arr))
    {
        fprintf(stderr, "[anti_reflux] meter cfg missing or invalid strategy_points.group.read array: %s\n", cfg_file);
        goto __del;
    }
    pMeter->ChnlId = chnl_id;
    pMeter->DevIndex = dev_index;
    for (i = 0; i < ANTI_REFLUX_METER_READ_POINTS; i++)
        pMeter->read_points[i].dblink_id = ANTI_REFLUX_DBLINK_ID_INVALID;

    for (i = 0; i < cJSON_GetArraySize(read_arr); i++)
    {
        item = cJSON_GetArrayItem(read_arr, i);
        if (!item)
            continue;
        key = NULL;
        if (cJSON_GetObjectItem(item, "key") && cJSON_GetObjectItem(item, "key")->valuestring)
            key = cJSON_GetObjectItem(item, "key")->valuestring;
        if (!key)
            continue;
        idx = meter_read_index_by_key(key);
        if (idx < 0)
            continue;
        datattr = 0;
        if (cJSON_GetObjectItem(item, "datattr"))
            datattr = cJSON_GetObjectItem(item, "datattr")->valueint;
        slot_index = -1;
        if (cJSON_GetObjectItem(item, "slot_index"))
            slot_index = cJSON_GetObjectItem(item, "slot_index")->valueint;

        is_user_fill = 0;
        j_val_src = cJSON_GetObjectItem(item, "value_source");
        j_is_uf = cJSON_GetObjectItem(item, "IsUserFill");
        if (j_val_src && j_val_src->valuestring && strcmp(j_val_src->valuestring, "user_fill") == 0)
            is_user_fill = 1;
        if (j_is_uf && (j_is_uf->type == cJSON_True || (j_is_uf->type == cJSON_Number && j_is_uf->valueint)))
            is_user_fill = 1;
        effective_slot = is_user_fill ? -1 : slot_index;
        user_val_num = 0.0;
        if (is_user_fill)
        {
            j_user_val = cJSON_GetObjectItem(item, "user_value");
            if (j_user_val)
            {
                if (j_user_val->valuestring)
                    user_val_num = atof(j_user_val->valuestring);
                else if (cJSON_IsNumber(j_user_val))
                    user_val_num = j_user_val->valuedouble;
            }
        }
        fill_point(&pMeter->read_points[idx], idx, key, datattr, effective_slot, chnl_id, dev_index);
        if (is_user_fill)
        {
            pMeter->read_points[idx].is_user_fill = 1;
            pMeter->read_points[idx].user_value_num = user_val_num;
        }
    }
    ret = 0;

__del:
    cJSON_Delete(root);
__free:
    db_cfg_jsonfile_delete(p);
    return ret;
}

/*********************************************************************************************************
** Function name:       load_inverter_points_from_cfg
** Descriptions:        从设备 cfg 的 strategy_points 填充逆变器模型及 dblink_id
** input parameters:    cfg_file, chnl_id, dev_index
** output parameters:   pInv - 输出逆变器策略模型
** Returned value:      0-成功；-1-失败
*********************************************************************************************************/
static int load_inverter_points_from_cfg(const char *cfg_file, int chnl_id, int dev_index, AntiRefluxInverterModel *pInv)
{
    char *p;
    int   size, ret = -1;
    cJSON *root, *sp, *groups, *grp, *read_arr, *write_arr, *rw_arr, *item;
    int   i, idx, datattr, slot_index, effective_slot, is_user_fill;
    double user_val_num;
    const char *key;
    cJSON *j_val_src, *j_user_val, *j_is_uf;

    p = db_cfg_jsonfile_alloc((char *)cfg_file, &size);
    if (!p)
    {
        fprintf(stderr, "[anti_reflux] inverter cfg file not found: %s\n", cfg_file);
        return -1;
    }
    p = db_cfg_jsonfile_read((char *)cfg_file, p, size);
    if (!p)
    {
        fprintf(stderr, "[anti_reflux] inverter cfg file read failed: %s\n", cfg_file);
        goto __free;
    }

    /* 跳过 UTF-8 BOM，与电表 cfg 一致 */
    if (size >= 3 && (unsigned char)p[0] == 0xEF && (unsigned char)p[1] == 0xBB && (unsigned char)p[2] == 0xBF)
    {
        memmove(p, p + 3, (size_t)(size - 3));
        p[size - 3] = '\0';
    }

    root = cJSON_Parse(p);
    if (!root)
    {
        fprintf(stderr, "[anti_reflux] inverter cfg JSON parse failed: %s\n", cfg_file);
        goto __free;
    }

    /* 设备 cfg 中 strategy_points 在 root.attr 下（与 config/sample/devices 一致） */
    sp = cJSON_GetObjectItem(root, "attr");
    if (sp)
        sp = cJSON_GetObjectItem(sp, "strategy_points");
    if (!sp)
    {
        fprintf(stderr, "[anti_reflux] inverter cfg missing attr.strategy_points: %s\n", cfg_file);
        goto __del;
    }
    groups = cJSON_GetObjectItem(sp, "group");
    if (!groups || !cJSON_IsArray(groups))
        goto __del;

    for (i = 0; i < cJSON_GetArraySize(groups); i++)
    {
        grp = cJSON_GetArrayItem(groups, i);
        item = cJSON_GetObjectItem(grp, "strategy_id");
        if (item && item->valuestring && strcmp(item->valuestring, STRATEGY_ID_ANTI_REFLUX) == 0)
            break;
        grp = NULL;
    }
    if (!grp)
        goto __del;

    pInv->ChnlId = chnl_id;
    pInv->DevIndex = dev_index;
    pInv->limit_power_step_w = 0.0;
    for (i = 0; i < ANTI_REFLUX_INV_READ_POINTS; i++)
        pInv->read_points[i].dblink_id = ANTI_REFLUX_DBLINK_ID_INVALID;
    for (i = 0; i < ANTI_REFLUX_INV_WRITE_POINTS; i++)
        pInv->write_points[i].dblink_id = ANTI_REFLUX_DBLINK_ID_INVALID;
    pInv->rw_dynamic_control.dblink_id = ANTI_REFLUX_DBLINK_ID_INVALID;
    pInv->rw_dynamic_control.is_user_fill = 0;
    pInv->rw_dynamic_control.user_value_num = 0.0;

    read_arr = cJSON_GetObjectItem(grp, "read");
    if (read_arr && cJSON_IsArray(read_arr))
    {
        for (i = 0; i < cJSON_GetArraySize(read_arr); i++)
        {
            item = cJSON_GetArrayItem(read_arr, i);
            key = item ? (cJSON_GetObjectItem(item, "key") ? cJSON_GetObjectItem(item, "key")->valuestring : NULL) : NULL;
            if (!key)
                continue;
            /* 限功率颗粒度：从 read 中 key=LimitPowerStepW 且 user_fill 的项填入 pInv->limit_power_step_w */
            if (strcmp(key, "LimitPowerStepW") == 0)
            {
                is_user_fill = 0;
                j_val_src = cJSON_GetObjectItem(item, "value_source");
                j_is_uf = cJSON_GetObjectItem(item, "IsUserFill");
                if (j_val_src && j_val_src->valuestring && strcmp(j_val_src->valuestring, "user_fill") == 0)
                    is_user_fill = 1;
                if (j_is_uf && (j_is_uf->type == cJSON_True || (j_is_uf->type == cJSON_Number && j_is_uf->valueint)))
                    is_user_fill = 1;
                if (is_user_fill)
                {
                    user_val_num = 0.0;
                    j_user_val = cJSON_GetObjectItem(item, "user_value");
                    if (j_user_val)
                    {
                        if (j_user_val->valuestring)
                            user_val_num = atof(j_user_val->valuestring);
                        else if (cJSON_IsNumber(j_user_val))
                            user_val_num = j_user_val->valuedouble;
                        if (user_val_num > 0.0)
                            pInv->limit_power_step_w = user_val_num;
                    }
                }
                continue;
            }
            idx = inv_read_index_by_key(key);
            if (idx < 0)
                continue;
            datattr = 0;
            item = cJSON_GetObjectItem(cJSON_GetArrayItem(read_arr, i), "datattr");
            if (item)
                datattr = item->valueint;
            slot_index = -1;
            item = cJSON_GetObjectItem(cJSON_GetArrayItem(read_arr, i), "slot_index");
            if (item)
                slot_index = item->valueint;
            item = cJSON_GetArrayItem(read_arr, i);
            is_user_fill = 0;
            j_val_src = cJSON_GetObjectItem(item, "value_source");
            j_is_uf = cJSON_GetObjectItem(item, "IsUserFill");
            if (j_val_src && j_val_src->valuestring && strcmp(j_val_src->valuestring, "user_fill") == 0)
                is_user_fill = 1;
            if (j_is_uf && (j_is_uf->type == cJSON_True || (j_is_uf->type == cJSON_Number && j_is_uf->valueint)))
                is_user_fill = 1;
            effective_slot = is_user_fill ? -1 : slot_index;
            user_val_num = 0.0;
            if (is_user_fill)
            {
                j_user_val = cJSON_GetObjectItem(item, "user_value");
                if (j_user_val)
                {
                    if (j_user_val->valuestring)
                        user_val_num = atof(j_user_val->valuestring);
                    else if (cJSON_IsNumber(j_user_val))
                        user_val_num = j_user_val->valuedouble;
                }
            }
            fill_point(&pInv->read_points[idx], idx, key, datattr, effective_slot, chnl_id, dev_index);
            if (is_user_fill)
            {
                pInv->read_points[idx].is_user_fill = 1;
                pInv->read_points[idx].user_value_num = user_val_num;
            }
        }
    }

    write_arr = cJSON_GetObjectItem(grp, "write");
    if (write_arr && cJSON_IsArray(write_arr))
    {
        for (i = 0; i < cJSON_GetArraySize(write_arr); i++)
        {
            item = cJSON_GetArrayItem(write_arr, i);
            key = item ? (cJSON_GetObjectItem(item, "key") ? cJSON_GetObjectItem(item, "key")->valuestring : NULL) : NULL;
            if (!key)
                continue;
            idx = inv_write_index_by_key(key);
            if (idx < 0)
                continue;
            datattr = 0;
            item = cJSON_GetObjectItem(cJSON_GetArrayItem(write_arr, i), "datattr");
            if (item)
                datattr = item->valueint;
            slot_index = -1;
            item = cJSON_GetObjectItem(cJSON_GetArrayItem(write_arr, i), "slot_index");
            if (item)
                slot_index = item->valueint;
            item = cJSON_GetArrayItem(write_arr, i);
            is_user_fill = 0;
            j_val_src = cJSON_GetObjectItem(item, "value_source");
            j_is_uf = cJSON_GetObjectItem(item, "IsUserFill");
            if (j_val_src && j_val_src->valuestring && strcmp(j_val_src->valuestring, "user_fill") == 0)
                is_user_fill = 1;
            if (j_is_uf && (j_is_uf->type == cJSON_True || (j_is_uf->type == cJSON_Number && j_is_uf->valueint)))
                is_user_fill = 1;
            effective_slot = is_user_fill ? -1 : slot_index;
            user_val_num = 0.0;
            if (is_user_fill)
            {
                j_user_val = cJSON_GetObjectItem(item, "user_value");
                if (j_user_val)
                {
                    if (j_user_val->valuestring)
                        user_val_num = atof(j_user_val->valuestring);
                    else if (cJSON_IsNumber(j_user_val))
                        user_val_num = j_user_val->valuedouble;
                }
            }
            fill_point(&pInv->write_points[idx], idx, key, datattr, effective_slot, chnl_id, dev_index);
            if (is_user_fill)
            {
                pInv->write_points[idx].is_user_fill = 1;
                pInv->write_points[idx].user_value_num = user_val_num;
            }
        }
    }

    /* read_write[]：虚拟 DO/AO 读写绑定，字段与 read/write 相同 */
    rw_arr = cJSON_GetObjectItem(grp, "read_write");
    if (rw_arr && cJSON_IsArray(rw_arr))
    {
        for (i = 0; i < cJSON_GetArraySize(rw_arr); i++)
        {
            item = cJSON_GetArrayItem(rw_arr, i);
            key = item ? (cJSON_GetObjectItem(item, "key") ? cJSON_GetObjectItem(item, "key")->valuestring : NULL) : NULL;
            if (!key || strcmp(key, ANTI_REFLUX_RW_KEY_DYNAMIC_CONTROL) != 0)
                continue;
            datattr = 0;
            if (cJSON_GetObjectItem(item, "datattr"))
                datattr = cJSON_GetObjectItem(item, "datattr")->valueint;
            slot_index = -1;
            if (cJSON_GetObjectItem(item, "slot_index"))
                slot_index = cJSON_GetObjectItem(item, "slot_index")->valueint;
            is_user_fill = 0;
            j_val_src = cJSON_GetObjectItem(item, "value_source");
            j_is_uf = cJSON_GetObjectItem(item, "IsUserFill");
            if (j_val_src && j_val_src->valuestring && strcmp(j_val_src->valuestring, "user_fill") == 0)
                is_user_fill = 1;
            if (j_is_uf && (j_is_uf->type == cJSON_True || (j_is_uf->type == cJSON_Number && j_is_uf->valueint)))
                is_user_fill = 1;
            effective_slot = is_user_fill ? -1 : slot_index;
            user_val_num = 0.0;
            if (is_user_fill)
            {
                j_user_val = cJSON_GetObjectItem(item, "user_value");
                if (j_user_val)
                {
                    if (j_user_val->valuestring)
                        user_val_num = atof(j_user_val->valuestring);
                    else if (cJSON_IsNumber(j_user_val))
                        user_val_num = j_user_val->valuedouble;
                }
            }
            fill_point(&pInv->rw_dynamic_control, 0, key, datattr, effective_slot, chnl_id, dev_index);
            if (is_user_fill)
            {
                pInv->rw_dynamic_control.is_user_fill = 1;
                pInv->rw_dynamic_control.user_value_num = user_val_num;
            }
            break; /* 仅一条 DynamicAntiRefluxEnable */
        }
    }

    ret = 0;

__del:
    cJSON_Delete(root);
__free:
    db_cfg_jsonfile_delete(p);
    return ret;
}

/*********************************************************************************************************
** Function name:       anti_reflux_config_load
** Descriptions:        加载防逆流工作参数到 pCfg（strategy_control.json）
** input parameters:    config_base - 配置根路径
** output parameters:   pCfg - 输出防逆流 config
** Returned value:      0-成功；-1-失败
*********************************************************************************************************/
int anti_reflux_config_load(AntiRefluxConfig *pCfg, const char *config_base)
{
    char  path[ANTI_REFLUX_CFG_BASE_PATH_LEN];
    char *p;
    int   size, ret = -1;
    cJSON *root, *arr, *item0, *config_obj, *sid;

    if (!pCfg || !config_base)
        return -1;

    snprintf(path, sizeof(path), "%s/strategy/strategy_control.json", config_base);

    p = db_cfg_jsonfile_alloc(path, &size);
    if (!p)
        return -1;
    p = db_cfg_jsonfile_read(path, p, size);
    if (!p)
        goto __free;

    root = cJSON_Parse(p);
    if (!root)
        goto __free;

    if (!cJSON_IsArray(root))
        goto __del;
    arr = root;
    if (cJSON_GetArraySize(arr) < 1)
        goto __del;

    item0 = cJSON_GetArrayItem(arr, 0);
    sid = cJSON_GetObjectItem(item0, "StrategyId");
    if (!sid || !sid->valuestring || strcmp(sid->valuestring, STRATEGY_ID_ANTI_REFLUX) != 0)
        goto __del;

    config_obj = cJSON_GetObjectItem(item0, "config");
    if (!config_obj)
        goto __del;

    ret = parse_config_obj(config_obj, pCfg);

__del:
    cJSON_Delete(root);
__free:
    db_cfg_jsonfile_delete(p);
    return ret;
}

/*********************************************************************************************************
** Function name:       anti_reflux_model_load
** Descriptions:        加载防逆流设备模型（电表+逆变器）及测点 dblink_id
** input parameters:    config_base - 配置根路径
** output parameters:   pModel - 输出设备模型
** Returned value:      0-成功；-1-失败
*********************************************************************************************************/
int anti_reflux_model_load(AntiRefluxDeviceModel *pModel, const char *config_base)
{
    char  path[ANTI_REFLUX_CFG_BASE_PATH_LEN];
    char  chnl_path[ANTI_REFLUX_CFG_BASE_PATH_LEN];
    char  cfg_file[256];
    char *p;
    int   size, ret = -1;
    cJSON *root, *item0, *meter_ref, *inv_refs, *ref_item;
    int   chnl_id, dev_index;
    int   i, n_inv;

    if (!pModel || !config_base)
        return -1;

    memset(pModel, 0, sizeof(*pModel));
    pModel->meter.read_points[0].dblink_id = ANTI_REFLUX_DBLINK_ID_INVALID; /* 标记未加载 */

    snprintf(path, sizeof(path), "%s/strategy/strategy_control.json", config_base);

    p = db_cfg_jsonfile_alloc(path, &size);
    if (!p)
    {
        fprintf(stderr, "[anti_reflux] strategy_control.json not found: %s\n", path);
        return -1;
    }
    p = db_cfg_jsonfile_read(path, p, size);
    if (!p)
        goto __free;

    root = cJSON_Parse(p);
    if (!root)
        goto __free;

    if (!cJSON_IsArray(root) || cJSON_GetArraySize(root) < 1)
        goto __del;

    item0 = cJSON_GetArrayItem(root, 0);
    meter_ref = cJSON_GetObjectItem(item0, "MeterRef");
    if (!meter_ref || !cJSON_GetObjectItem(meter_ref, "ChnlId") || !cJSON_GetObjectItem(meter_ref, "DevIndex"))
        goto __del;

    chnl_id = (int)cJSON_GetObjectItem(meter_ref, "ChnlId")->valueint;
    dev_index = (int)cJSON_GetObjectItem(meter_ref, "DevIndex")->valueint;

    /* 通道配置路径固定为 config_base/sample/channels/chnl_N.json */
    path_chnl_subdir(chnl_path, sizeof(chnl_path), config_base, "sample/channels", chnl_id);
    if (get_cfg_file_from_channel(chnl_path, dev_index, cfg_file, sizeof(cfg_file)) != 0)
    {
        fprintf(stderr, "[anti_reflux] get_cfg_file_from_channel failed for meter ChnlId=%d DevIndex=%d path=%s\n", chnl_id, dev_index, chnl_path);
        goto __del;
    }
    if (load_meter_points_from_cfg(cfg_file, chnl_id, dev_index, &pModel->meter) != 0)
    {
        fprintf(stderr, "[anti_reflux] load_meter_points_from_cfg failed: %s\n", cfg_file);
        goto __del;
    }

    inv_refs = cJSON_GetObjectItem(item0, "InverterRefs");
    if (!inv_refs || !cJSON_IsArray(inv_refs))
    {
        ret = 0;
        goto __del;
    }

    n_inv = cJSON_GetArraySize(inv_refs);
    if (n_inv > (int)ANTI_REFLUX_MAX_INVERTERS)
        n_inv = (int)ANTI_REFLUX_MAX_INVERTERS;
    pModel->inverter_count = (uint32_t)n_inv;

    /* 未成功加载的逆变器写点保持 INVALID，避免误写节点 0 */
    for (i = 0; i < n_inv; i++)
    {
        uint32_t j;
        pModel->inverters[i].persist_enabled = 1;
        pModel->inverters[i].safety_relay_present = 0;
        pModel->inverters[i].safety_relay_enabled = 0;
        pModel->inverters[i].safety_relay_chnlid = 0;
        pModel->inverters[i].safety_relay_devindex = 0;
        pModel->inverters[i].safety_trip_bit0_15 = 0;
        pModel->inverters[i].safety_close_bit0_15 = 0;
        pModel->inverters[i].safety_state_bit0_15 = 0;
        pModel->inverters[i].safety_trip_dblink_id = ANTI_REFLUX_DBLINK_ID_INVALID;
        pModel->inverters[i].safety_close_dblink_id = ANTI_REFLUX_DBLINK_ID_INVALID;
        pModel->inverters[i].safety_state_dblink_id = ANTI_REFLUX_DBLINK_ID_INVALID;
        pModel->inverters[i].limit_power_step_w = 0.0;
        for (j = 0; j < ANTI_REFLUX_INV_READ_POINTS; j++)
            pModel->inverters[i].read_points[j].dblink_id = ANTI_REFLUX_DBLINK_ID_INVALID;
        for (j = 0; j < ANTI_REFLUX_INV_WRITE_POINTS; j++)
            pModel->inverters[i].write_points[j].dblink_id = ANTI_REFLUX_DBLINK_ID_INVALID;
        pModel->inverters[i].rw_dynamic_control.dblink_id = ANTI_REFLUX_DBLINK_ID_INVALID;
        pModel->inverters[i].rw_dynamic_control.is_user_fill = 0;
    }
    for (i = 0; i < n_inv; i++)
    {
        cJSON *en_js, *relay_bind, *relay_enabled_js;
        int relay_chnl_id, relay_dev_index;
        uint16_t trip_low, close_low, state_low;

        ref_item = cJSON_GetArrayItem(inv_refs, i);
        if (!ref_item || !cJSON_GetObjectItem(ref_item, "ChnlId") || !cJSON_GetObjectItem(ref_item, "DevIndex"))
        {
            fprintf(stderr, "[anti_reflux] InverterRefs[%d] missing ChnlId/DevIndex, skip\n", i);
            continue;
        }
        chnl_id = (int)cJSON_GetObjectItem(ref_item, "ChnlId")->valueint;
        dev_index = (int)cJSON_GetObjectItem(ref_item, "DevIndex")->valueint;
        pModel->inverters[i].ChnlId = chnl_id;
        pModel->inverters[i].DevIndex = dev_index;

        pModel->inverters[i].persist_enabled = 1;
        en_js = cJSON_GetObjectItem(ref_item, "enable");
        if (en_js)
            pModel->inverters[i].persist_enabled = (en_js->type == cJSON_True || en_js->valueint) ? 1 : 0;

        relay_bind = cJSON_GetObjectItem(ref_item, "safety_relay_bind");
        if (relay_bind && cJSON_IsObject(relay_bind))
        {
            pModel->inverters[i].safety_relay_present = 1;
            relay_enabled_js = cJSON_GetObjectItem(relay_bind, "enabled");
            pModel->inverters[i].safety_relay_enabled =
                (relay_enabled_js && (relay_enabled_js->type == cJSON_True || relay_enabled_js->valueint)) ? 1 : 0;

            if (pModel->inverters[i].safety_relay_enabled)
            {
                cJSON *relay_chnl_js = cJSON_GetObjectItem(relay_bind, "RelayChnlId");
                cJSON *relay_dev_js = cJSON_GetObjectItem(relay_bind, "RelayDevIndex");
                relay_chnl_id = relay_chnl_js ? relay_chnl_js->valueint : -1;
                relay_dev_index = relay_dev_js ? relay_dev_js->valueint : -1;

                if (relay_chnl_id > 0 && relay_dev_index > 0 &&
                    parse_hex_u16_str(relay_bind, "trip_write_bit0_15", &trip_low) == 0 &&
                    parse_hex_u16_str(relay_bind, "close_write_bit0_15", &close_low) == 0 &&
                    parse_hex_u16_str(relay_bind, "state_read_bit0_15", &state_low) == 0)
                {
                    pModel->inverters[i].safety_relay_chnlid = relay_chnl_id;
                    pModel->inverters[i].safety_relay_devindex = relay_dev_index;
                    pModel->inverters[i].safety_trip_bit0_15 = trip_low;
                    pModel->inverters[i].safety_close_bit0_15 = close_low;
                    pModel->inverters[i].safety_state_bit0_15 = state_low;
                    pModel->inverters[i].safety_trip_dblink_id =
                        (((uint32_t)(relay_chnl_id & 0xFF) << 24) |
                         ((uint32_t)(relay_dev_index & 0xFF) << 16) |
                         (uint32_t)trip_low);
                    pModel->inverters[i].safety_close_dblink_id =
                        (((uint32_t)(relay_chnl_id & 0xFF) << 24) |
                         ((uint32_t)(relay_dev_index & 0xFF) << 16) |
                         (uint32_t)close_low);
                    pModel->inverters[i].safety_state_dblink_id =
                        (((uint32_t)(relay_chnl_id & 0xFF) << 24) |
                         ((uint32_t)(relay_dev_index & 0xFF) << 16) |
                         (uint32_t)state_low);
                }
                else
                {
                    pModel->inverters[i].safety_relay_enabled = 0;
                    fprintf(stderr, "[anti_reflux] InverterRefs[%d] safety_relay_bind invalid, force disabled\n", i);
                }
            }
        }

        if (!pModel->inverters[i].persist_enabled)
        {
            fprintf(stderr, "[anti_reflux] InverterRefs[%d] ChnlId=%d DevIndex=%d enable=false, skip device strategy_points\n",
                    i, chnl_id, dev_index);
            continue;
        }

        path_chnl_subdir(chnl_path, sizeof(chnl_path), config_base, "sample/channels", chnl_id);
        if (get_cfg_file_from_channel(chnl_path, dev_index, cfg_file, sizeof(cfg_file)) != 0)
        {
            fprintf(stderr, "[anti_reflux] get_cfg_file_from_channel failed for inv[%d] ChnlId=%d DevIndex=%d\n", i, chnl_id, dev_index);
            continue;
        }
        load_inverter_points_from_cfg(cfg_file, chnl_id, dev_index, &pModel->inverters[i]);
    }
    ret = 0;

__del:
    cJSON_Delete(root);
__free:
    db_cfg_jsonfile_delete(p);
    return ret;
}
