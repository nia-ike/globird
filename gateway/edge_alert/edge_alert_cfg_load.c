/*
* Copyright (c) 2024
* All rights reserved.
*
* 文件名称：edge_alert_cfg_load.c
* 文件标识：
* 描    述：edge_alert 配置加载实现（参照 edge_script 风格）
*
* 当前版本：V1.0
* 作    者：chenzy
* 更改说明：创建
* 完成日期：20240702
*
*/

#include "edge_alert_cfg_load.h"
#include "edge_alert_log.h"

#include "db_cfg_json.h"
#include "db_struct.h"
#include "cJSON.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

/*************************************************************************/
/** Function Name:       copy_json_string                                  **/
/** Purpose:             从 JSON 对象复制指定字符串字段到输出缓冲区         **/
/** Params:            obj - JSON 对象；key - 字段名；out - 输出缓冲区；   **/
/**                      out_len - 缓冲区长度                              **/
/** Return:            0-成功；-1-失败                                     **/
/** Notice:            字符串超长时截断并保证以 '\0' 结尾                   **/
/*************************************************************************/
static int copy_json_string(const cJSON *obj, const char *key, char *out, size_t out_len)
{
    cJSON *item;

    if (!obj || !key || !out || out_len == 0)
        return -1;

    item = cJSON_GetObjectItem((cJSON *)obj, key);
    if (!item || !cJSON_IsString(item) || !item->valuestring)
        return -1;

    strncpy(out, item->valuestring, out_len - 1);
    out[out_len - 1] = '\0';
    return 0;
}

/*************************************************************************/
/** Function Name:       edge_alert_cfg_parse_method                       **/
/** Purpose:             解析比较方法字符串                                **/
/*************************************************************************/
enum AlarmCompareMethod edge_alert_cfg_parse_method(const char *str)
{
    if (!str)
        return ALARM_CMP_GT;

    if (strcmp(str, "upper") == 0 || strcmp(str, "gt") == 0)
        return ALARM_CMP_GT;
    if (strcmp(str, "lower") == 0 || strcmp(str, "lt") == 0)
        return ALARM_CMP_LT;
    if (strcmp(str, "equal") == 0 || strcmp(str, "eq") == 0)
        return ALARM_CMP_EQ;
    if (strcmp(str, "not_equal") == 0 || strcmp(str, "neq") == 0)
        return ALARM_CMP_NEQ;
    if (strcmp(str, "bit_on") == 0)
        return ALARM_CMP_BIT_ON;
    if (strcmp(str, "bit_off") == 0)
        return ALARM_CMP_BIT_OFF;

    return ALARM_CMP_GT;
}

/*************************************************************************/
/** Function Name:       edge_alert_cfg_parse_logic                        **/
/** Purpose:             解析逻辑组合字符串                                **/
/*************************************************************************/
enum AlarmLogic edge_alert_cfg_parse_logic(const char *str)
{
    if (!str)
        return ALARM_LOGIC_OR;

    if (strcmp(str, "and") == 0)
        return ALARM_LOGIC_AND;

    return ALARM_LOGIC_OR;
}

/*************************************************************************/
/** Function Name:       edge_alert_cfg_parse_condition                    **/
/** Purpose:             解析单个条件配置                                  **/
/*************************************************************************/
static int edge_alert_cfg_parse_condition(cJSON *cond_json, struct AlarmCondition *cond,
                                         uint8_t chnl_id, uint8_t dev_id)
{
    cJSON *item;
    char *method_str = NULL;
    uint32_t point_idx;
    uint16_t datattr = 0;

    if (!cond_json || !cond)
        return -1;

    memset(cond, 0, sizeof(struct AlarmCondition));

    /* datattr - 数据属性/类型 (0=AI, 1=DI, 2=ACC)，默认0 */
    item = cJSON_GetObjectItem(cond_json, "datattr");
    if (item && cJSON_IsNumber(item))
        datattr = (uint16_t)item->valuedouble;
    cond->datattr = datattr;

    /* pointid - 点位序号，与datattr组合成完整dblinkid */
    item = cJSON_GetObjectItem(cond_json, "pointid");
    if (item && cJSON_IsNumber(item))
    {
        point_idx = (uint32_t)item->valuedouble;
        /* DB_CONVENT2ID: chnl<<24 | dev<<16 | type<<8 | id，type从datattr取 */
        cond->point_id = DB_CONVENT2ID(chnl_id, dev_id, datattr, point_idx);
        edge_log(LOG_DEBUG, "Parsed condition: point_idx=%u, datattr=%u, point_id=%08X",
                 point_idx, datattr, cond->point_id);
    }
    else
    {
        edge_log(LOG_WARN, "Condition missing 'pointid' field");
    }

    /* alias */
    copy_json_string(cond_json, "alias", cond->alias, sizeof(cond->alias));

    /* method */
    item = cJSON_GetObjectItem(cond_json, "method");
    if (item && cJSON_IsString(item))
        method_str = item->valuestring;
    cond->method = edge_alert_cfg_parse_method(method_str);

    /* threshold */
    item = cJSON_GetObjectItem(cond_json, "threshold");
    if (item && cJSON_IsNumber(item))
        cond->threshold = item->valuedouble;

    /* hysteresis_pct */
    item = cJSON_GetObjectItem(cond_json, "hysteresis_pct");
    if (item && cJSON_IsNumber(item))
        cond->hysteresis_pct = (uint8_t)item->valuedouble;

    /* level */
    item = cJSON_GetObjectItem(cond_json, "level");
    if (item && cJSON_IsNumber(item))
        cond->level = (uint8_t)item->valuedouble;

    return 0;
}

/*************************************************************************/
/** Function Name:       edge_alert_cfg_parse_trigger                      **/
/** Purpose:             解析触发器配置                                    **/
/*************************************************************************/
static int edge_alert_cfg_parse_trigger(cJSON *trigger_json, struct AlarmTrigger *trigger,
                                       uint8_t chnl_id, uint8_t dev_id)
{
    cJSON *conditions_array;
    cJSON *cond_item;
    cJSON *item;
    char *logic_str = NULL;
    int i;

    if (!trigger_json || !trigger)
        return -1;

    memset(trigger, 0, sizeof(struct AlarmTrigger));

    /* logic */
    item = cJSON_GetObjectItem(trigger_json, "logic");
    if (item && cJSON_IsString(item))
        logic_str = item->valuestring;
    trigger->logic = edge_alert_cfg_parse_logic(logic_str);

    /* conditions */
    conditions_array = cJSON_GetObjectItem(trigger_json, "conditions");
    if (conditions_array && cJSON_IsArray(conditions_array))
    {
        trigger->condition_count = (uint8_t)cJSON_GetArraySize(conditions_array);
        if (trigger->condition_count > 8)
            trigger->condition_count = 8;

        for (i = 0; i < trigger->condition_count; i++)
        {
            cond_item = cJSON_GetArrayItem(conditions_array, i);
            edge_alert_cfg_parse_condition(cond_item, &trigger->conditions[i], chnl_id, dev_id);
        }
    }

    return 0;
}

/*************************************************************************/
/** Function Name:       edge_alert_cfg_parse_context_points               **/
/** Purpose:             解析上下文点位配置                                **/
/*************************************************************************/
static int edge_alert_cfg_parse_context_points(cJSON *ctx_array, struct AlarmRule *rule,
                                             uint8_t chnl_id, uint8_t dev_id)
{
    cJSON *ctx_item;
    cJSON *item;
    int i;
    uint32_t point_idx;

    if (!ctx_array || !rule)
        return -1;

    if (!cJSON_IsArray(ctx_array))
        return -1;

    rule->context_count = (uint8_t)cJSON_GetArraySize(ctx_array);
    if (rule->context_count > 16)
        rule->context_count = 16;

    for (i = 0; i < rule->context_count; i++)
    {
        uint16_t datattr = 0;

        ctx_item = cJSON_GetArrayItem(ctx_array, i);
        if (!ctx_item)
            continue;

        /* datattr - 数据属性/类型，默认0 */
        item = cJSON_GetObjectItem(ctx_item, "datattr");
        if (item && cJSON_IsNumber(item))
            datattr = (uint16_t)item->valuedouble;
        rule->context_points[i].datattr = datattr;

        /* pointid - 点位序号，与datattr组合成完整dblinkid */
        item = cJSON_GetObjectItem(ctx_item, "pointid");
        if (item && cJSON_IsNumber(item))
        {
            point_idx = (uint32_t)item->valuedouble;
            rule->context_points[i].point_id = DB_CONVENT2ID(chnl_id, dev_id, datattr, point_idx);
            edge_log(LOG_DEBUG, "Parsed context point %d: point_idx=%u, datattr=%u, point_id=%08X",
                     i, point_idx, datattr, rule->context_points[i].point_id);
        }
        else
        {
            edge_log(LOG_WARN, "Context point %d missing 'pointid' field", i);
        }

        item = cJSON_GetObjectItem(ctx_item, "alias");
        if (item && cJSON_IsString(item))
        {
            strncpy(rule->context_points[i].alias, item->valuestring,
                    sizeof(rule->context_points[i].alias) - 1);
            rule->context_points[i].alias[sizeof(rule->context_points[i].alias) - 1] = '\0';
        }
    }

    return 0;
}

/*************************************************************************/
/** Function Name:       edge_alert_cfg_parse_rule                         **/
/** Purpose:             解析单条告警规则                                  **/
/*************************************************************************/
static int edge_alert_cfg_parse_rule(cJSON *rule_json, struct AlarmRule *rule,
                                      uint8_t chnl_id, uint8_t dev_id)
{
    cJSON *trigger_json;
    cJSON *ctx_array;
    cJSON *item;
    char *method_str = NULL;

    if (!rule_json || !rule)
        return -1;

    memset(rule, 0, sizeof(struct AlarmRule));

    /* 归属信息 */
    rule->chnl_id = chnl_id;
    rule->dev_id = dev_id;

    /* 检查关键字段是否存在 (用于调试) */
    if (!cJSON_GetObjectItem(rule_json, "id"))
        edge_log(LOG_WARN, "Rule missing 'id' field");
    if (!cJSON_GetObjectItem(rule_json, "pointid"))
        edge_log(LOG_WARN, "Rule missing 'pointid' field");
    if (!cJSON_GetObjectItem(rule_json, "method"))
        edge_log(LOG_WARN, "Rule missing 'method' field");

    /* id */
    item = cJSON_GetObjectItem(rule_json, "id");
    if (item && cJSON_IsNumber(item))
        rule->rule_id = (uint16_t)item->valuedouble;
    else
        edge_log(LOG_WARN, "Rule 'id' field missing or not a number");

    /* enabled */
    item = cJSON_GetObjectItem(rule_json, "enabled");
    if (item)
    {
        if (cJSON_IsBool(item))
            rule->enabled = cJSON_IsTrue(item) ? 1 : 0;
        else if (cJSON_IsNumber(item))
            rule->enabled = (uint8_t)item->valuedouble;
        else
            rule->enabled = 1;
    }
    else
    {
        rule->enabled = 1;
    }

    /* name */
    copy_json_string(rule_json, "name", rule->name, sizeof(rule->name));

    /* type */
    copy_json_string(rule_json, "type", rule->type, sizeof(rule->type));

    /* datattr - 数据属性/类型 (0=AI, 1=DI, 2=ACC)，默认0 */
    item = cJSON_GetObjectItem(rule_json, "datattr");
    if (item && cJSON_IsNumber(item))
        rule->datattr = (uint16_t)item->valuedouble;
    else
        rule->datattr = 0;

    /* pointid - 点位序号，与datattr组合成完整dblinkid */
    item = cJSON_GetObjectItem(rule_json, "pointid");
    if (item && cJSON_IsNumber(item))
    {
        uint32_t point_idx = (uint32_t)item->valuedouble;
        rule->point_id = DB_CONVENT2ID(chnl_id, dev_id, rule->datattr, point_idx);
    }

    /* method */
    item = cJSON_GetObjectItem(rule_json, "method");
    if (item && cJSON_IsString(item))
        method_str = item->valuestring;
    rule->method = edge_alert_cfg_parse_method(method_str);

    /* threshold */
    item = cJSON_GetObjectItem(rule_json, "threshold");
    if (item && cJSON_IsNumber(item))
        rule->threshold = item->valuedouble;

    /* level */
    item = cJSON_GetObjectItem(rule_json, "level");
    if (item && cJSON_IsNumber(item))
        rule->level = (uint8_t)item->valuedouble;
    else
        rule->level = 1;

    /* delay_sec */
    item = cJSON_GetObjectItem(rule_json, "delay_sec");
    if (item && cJSON_IsNumber(item))
        rule->delay_sec = (uint16_t)item->valuedouble;

    /* hysteresis_pct */
    item = cJSON_GetObjectItem(rule_json, "hysteresis_pct");
    if (item && cJSON_IsNumber(item))
        rule->hysteresis_pct = (uint8_t)item->valuedouble;
    else
        rule->hysteresis_pct = 10;

    /* trigger */
    trigger_json = cJSON_GetObjectItem(rule_json, "trigger");
    if (trigger_json)
        edge_alert_cfg_parse_trigger(trigger_json, &rule->trigger, chnl_id, dev_id);

    /* context_points */
    ctx_array = cJSON_GetObjectItem(rule_json, "context_points");
    if (ctx_array)
        edge_alert_cfg_parse_context_points(ctx_array, rule, chnl_id, dev_id);

    return 0;
}

/*************************************************************************/
/** Function Name:       edge_alert_cfg_load_device_rules                  **/
/** Purpose:             从单个设备配置文件加载告警规则                    **/
/*************************************************************************/
static int edge_alert_cfg_load_device_rules(const char *filepath, uint8_t chnl_id, uint8_t dev_id,
                                            struct EdgeAlertConfig *cfg)
{
    char *buf = NULL;
    int size = 0;
    cJSON *root = NULL;
    cJSON *attr_obj = NULL;
    cJSON *alarm_obj = NULL;
    cJSON *rules_array = NULL;
    cJSON *rule_item = NULL;
    int i;
    int rule_count = 0;

    if (!filepath || !cfg)
        return -1;

    /* 使用 db_cfg_json API 读取文件 */
    buf = db_cfg_jsonfile_alloc((char *)filepath, &size);
    if (!buf)
    {
        edge_log(LOG_WARN, "Failed to alloc/read device config: %s", filepath);
        return -1;
    }

    if (!db_cfg_jsonfile_read((char *)filepath, buf, size))
    {
        db_cfg_jsonfile_delete(buf);
        return -1;
    }

    /* 确保字符串null终止（db_cfg_jsonfile_read不会添加） */
    buf[size] = '\0';

    /* 解析JSON */
    root = cJSON_Parse(buf);
    db_cfg_jsonfile_delete(buf);

    if (!root)
    {
        edge_log(LOG_ERROR, "Failed to parse JSON: %s", filepath);
        return -1;
    }

    /* 查找attr对象 */
    attr_obj = cJSON_GetObjectItem(root, "attr");
    if (!attr_obj)
    {
        edge_log(LOG_WARN, "No 'attr' object found in %s", filepath);
        cJSON_Delete(root);
        return 0; /* 没有attr配置，非错误 */
    }

    /* 在attr中查找alarm对象 */
    alarm_obj = cJSON_GetObjectItem(attr_obj, "alarm");
    if (!alarm_obj)
    {
        edge_log(LOG_WARN, "No 'alarm' object found in attr of %s", filepath);
        cJSON_Delete(root);
        return 0; /* 没有告警配置，非错误 */
    }
    /* 查找rules数组 */
    rules_array = cJSON_GetObjectItem(alarm_obj, "rules");
    if (!rules_array)
    {
        edge_log(LOG_WARN, "No 'rules' item found in alarm object in %s", filepath);
        cJSON_Delete(root);
        return 0; /* 没有告警规则，非错误 */
    }
    if (!cJSON_IsArray(rules_array))
    {
        edge_log(LOG_WARN, "'rules' is not an array in %s", filepath);
        cJSON_Delete(root);
        return 0; /* 不是数组，非错误 */
    }

    /* 解析每条规则 */
    rule_count = cJSON_GetArraySize(rules_array);
    for (i = 0; i < rule_count && cfg->rule_count < EDGE_ALERT_CFG_MAX_RULES; i++)
    {
        rule_item = cJSON_GetArrayItem(rules_array, i);
        if (!rule_item)
        {
            edge_log(LOG_WARN, "Failed to get rule item %d from %s", i, filepath);
            continue;
        }

        if (edge_alert_cfg_parse_rule(rule_item, &cfg->rules[cfg->rule_count], chnl_id, dev_id) == 0)
        {
            cfg->rule_count++;
        }
        else
        {
            edge_log(LOG_WARN, "Failed to parse rule %d from %s", i, filepath);
        }
    }

    cJSON_Delete(root);
    edge_log(LOG_INFO, "Completed loading from %s, current total rules: %d", 
             filepath, cfg->rule_count);

    return 0;
}

/*************************************************************************/
/** Function Name:       edge_alert_cfg_parse_device_filename              **/
/** Purpose:             从设备文件名解析通道ID和设备ID                    **/
/** Notice:              文件名格式：C{chnl}_D{dev}_*.json                  **/
/*************************************************************************/
static int edge_alert_cfg_parse_device_filename(const char *filename, uint8_t *chnl_id, uint8_t *dev_id)
{
    int chnl, dev;

    if (!filename || !chnl_id || !dev_id)
        return -1;

    /* 尝试解析 Cx_Dx_ 格式 */
    if (sscanf(filename, "C%d_D%d_", &chnl, &dev) == 2)
    {
        if (chnl >= 0 && chnl <= 255 && dev >= 0 && dev <= 255)
        {
            *chnl_id = (uint8_t)chnl;
            *dev_id = (uint8_t)dev;
            return 0;
        }
    }

    return -1;
}

/*************************************************************************/
/** Function Name:       edge_alert_cfg_load_device_directory              **/
/** Purpose:             遍历设备配置目录加载所有告警规则                   **/
/*************************************************************************/
static int edge_alert_cfg_load_device_directory(const char *devices_dir,
                                                struct EdgeAlertConfig *cfg)
{
    DIR *dir = NULL;
    struct dirent *entry = NULL;
    char filepath[EDGE_ALERT_CFG_BASE_PATH_LEN + 64];
    uint8_t chnl_id, dev_id;
    int count = 0;

    if (!devices_dir || !cfg)
        return -1;

    dir = opendir(devices_dir);
    if (!dir)
    {
        edge_log(LOG_WARN, "Failed to open devices directory: %s", devices_dir);
        return 0; /* 目录不存在，非致命错误 */
    }

    while ((entry = readdir(dir)) != NULL)
    {
        /* 只处理 .json 文件 */
        if (strstr(entry->d_name, ".json") == NULL)
            continue;

        /* 解析文件名获取通道ID和设备ID */
        if (edge_alert_cfg_parse_device_filename(entry->d_name, &chnl_id, &dev_id) != 0)
            continue;

        /* 构建完整路径 */
        snprintf(filepath, sizeof(filepath), "%s/%s", devices_dir, entry->d_name);

        /* 加载该设备的告警规则 */
        if (edge_alert_cfg_load_device_rules(filepath, chnl_id, dev_id, cfg) == 0)
        {
            count++;
        }
    }

    closedir(dir);
    edge_log(LOG_INFO, "Loaded rules from %d device configs in %s", count, devices_dir);

    return 0;
}

/*************************************************************************/
/** Function Name:       edge_alert_cfg_load                               **/
/** Purpose:             加载 edge_alert.json 及设备告警规则配置           **/
/** Params:            cfg - 输出配置结构体                                **/
/**                      config_base - 配置根路径（如 /userdata/config）  **/
/** Return:            0-成功（含配置文件不存在时的空配置）               **/
/**                    -1-文件存在但读/解析失败                           **/
/** Notice:            路径为 {config_base}/edge_alert/edge_alert.json      **/
/**                    设备规则从 {config_base}/sample/devices/*.json 解析 **/
/*************************************************************************/
int edge_alert_cfg_load(struct EdgeAlertConfig *cfg, const char *config_base)
{
    char json_path[EDGE_ALERT_CFG_BASE_PATH_LEN + 64];
    char devices_dir[EDGE_ALERT_CFG_BASE_PATH_LEN + 64];
    struct stat st;
    char *buf = NULL;
    int size = 0;
    cJSON *root = NULL;
    cJSON *item = NULL;
    cJSON *attr_obj = NULL;

    if (!cfg || !config_base)
        return -1;

    memset(cfg, 0, sizeof(*cfg));

    /* 设置默认值 */
    cfg->max_records = EDGE_ALERT_CFG_MAX_RECORDS;
    cfg->poll_interval_ms = 1000;
    strncpy(cfg->ramfs_path, EDGE_ALERT_DB_PATH, sizeof(cfg->ramfs_path) - 1);

    /* 构建告警服务配置文件路径 */
    snprintf(json_path, sizeof(json_path), "%s/edge_alert/edge_alert.json", config_base);

    /* 检查文件是否存在 */
    if (stat(json_path, &st) != 0)
    {
        if (errno == ENOENT)
        {
            strncpy(cfg->version, "1.0", sizeof(cfg->version) - 1);
            cfg->rule_count = 0;
            /* 目录不存在，继续加载设备规则（可能只配置了设备告警） */
        }
        else
        {
            return -1;
        }
    }
    else
    {
        /* 使用 db_cfg_json API 读取文件 */
        buf = db_cfg_jsonfile_alloc(json_path, &size);
        if (!buf)
            return -1;

        if (!db_cfg_jsonfile_read(json_path, buf, size))
        {
            db_cfg_jsonfile_delete(buf);
            return -1;
        }

        root = cJSON_Parse(buf);
        db_cfg_jsonfile_delete(buf);

        if (!root)
        {
            edge_log(LOG_ERROR, "Failed to parse edge_alert.json");
            return -1;
        }

        /* 解析 version */
        copy_json_string(root, "version", cfg->version, sizeof(cfg->version));

        /* 解析 max_alarm_nums */
        item = cJSON_GetObjectItem(root, "max_alarm_nums");
        if (item && cJSON_IsNumber(item))
            cfg->max_records = (uint16_t)item->valuedouble;
        if (cfg->max_records == 0)
            cfg->max_records = EDGE_ALERT_CFG_MAX_RECORDS;

        /* 解析 inv_time (轮询间隔) */
        item = cJSON_GetObjectItem(root, "inv_time");
        if (item && cJSON_IsNumber(item))
            cfg->poll_interval_ms = (uint16_t)item->valuedouble;

        /* 解析 attr 对象 */
        attr_obj = cJSON_GetObjectItem(root, "attr");
        if (attr_obj)
        {
            /* 预留：解析其他属性 */
        }

        cJSON_Delete(root);
    }

    /* 加载设备告警规则 */
    snprintf(devices_dir, sizeof(devices_dir), "%s/sample/devices", config_base);
    edge_alert_cfg_load_device_directory(devices_dir, cfg);

    edge_log(LOG_INFO, "EdgeAlert config loaded: version=%s, max_records=%d, rules=%d",
             cfg->version, cfg->max_records, cfg->rule_count);

    return 0;
}
