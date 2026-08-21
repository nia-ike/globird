#include "edge_script_cfg_load.h"
#include "db_cfg_json.h"
#include "cJSON.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

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
/** Function Name:       parse_trigger                                     **/
/** Purpose:             解析脚本 trigger 对象，缺省 type 为 thread         **/
/** Params:            obj - trigger JSON 对象；trigger - 输出结构体       **/
/** Return:            0-成功；-1-trigger 指针无效                        **/
/** Notice:            obj 为 NULL 时使用默认 thread 调度类型               **/
/*************************************************************************/
static int parse_trigger(const cJSON *obj, EdgeScriptTrigger *trigger)
{
    if (!trigger)
        return -1;

    memset(trigger, 0, sizeof(*trigger));
    strncpy(trigger->type, "thread", sizeof(trigger->type) - 1);

    if (!obj)
        return 0;

    if (copy_json_string(obj, "type", trigger->type, sizeof(trigger->type)) != 0)
        strncpy(trigger->type, "thread", sizeof(trigger->type) - 1);

    return 0;
}

/*************************************************************************/
/** Function Name:       parse_script_entry                                **/
/** Purpose:             解析 scripts 数组中单条脚本配置                    **/
/** Params:            item - 脚本 JSON 对象；config_base - 配置根路径；   **/
/**                      entry - 输出脚本条目                               **/
/** Return:            0-成功；-1-失败                                     **/
/** Notice:            同时拼接 script_path 绝对路径                       **/
/*************************************************************************/
static int parse_script_entry(const cJSON *item, const char *config_base, EdgeScriptEntry *entry)
{
    cJSON *trigger;

    if (!item || !config_base || !entry)
        return -1;

    memset(entry, 0, sizeof(*entry));

    if (copy_json_string(item, "id", entry->id, sizeof(entry->id)) != 0)
        return -1;
    if (copy_json_string(item, "name", entry->name, sizeof(entry->name)) != 0)
        strncpy(entry->name, entry->id, sizeof(entry->name) - 1);

    {
        cJSON *enabled = cJSON_GetObjectItem((cJSON *)item, "enabled");
        if (enabled && cJSON_IsBool(enabled))
            entry->enabled = cJSON_IsTrue(enabled);
        else
            entry->enabled = true;
    }

    if (copy_json_string(item, "script_file", entry->script_file, sizeof(entry->script_file)) != 0)
        return -1;

    trigger = cJSON_GetObjectItem((cJSON *)item, "trigger");
    parse_trigger(trigger, &entry->trigger);

    snprintf(entry->script_path, sizeof(entry->script_path),
             "%s/edge_script/scripts/%s", config_base, entry->script_file);
    return 0;
}

/*************************************************************************/
/** Function Name:       edge_script_config_load                           **/
/** Purpose:             加载 edge_scripts.json v2.0 到 EdgeScriptConfig     **/
/** Params:            cfg - 输出配置；config_base - 配置根路径            **/
/** Return:            0-成功（含配置文件不存在时的空配置）；-1-读/解析失败  **/
/** Notice:            路径为 {config_base}/edge_script/edge_scripts.json  **/
/**                      旧设备无 edge_script 目录时返回空配置，供进程待命   **/
/*************************************************************************/
int edge_script_config_load(EdgeScriptConfig *cfg, const char *config_base)
{
    char json_path[EDGE_SCRIPT_CFG_BASE_PATH_LEN + 64];
    struct stat st;
    char *buf = NULL;
    int size = 0;
    cJSON *root = NULL;
    cJSON *scripts = NULL;
    int count;
    int i;

    if (!cfg || !config_base)
        return -1;

    memset(cfg, 0, sizeof(*cfg));

    snprintf(json_path, sizeof(json_path), "%s/edge_script/edge_scripts.json", config_base);

    if (stat(json_path, &st) != 0)
    {
        if (errno == ENOENT)
        {
            strncpy(cfg->version, "2.0", sizeof(cfg->version) - 1);
            cfg->script_count = 0;
            return 0;
        }
        return -1;
    }

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
        return -1;

    copy_json_string(root, "version", cfg->version, sizeof(cfg->version));

    scripts = cJSON_GetObjectItem(root, "scripts");
    if (!scripts || !cJSON_IsArray(scripts))
    {
        cJSON_Delete(root);
        return -1;
    }

    count = cJSON_GetArraySize(scripts);
    if (count > EDGE_SCRIPT_MAX_SCRIPTS)
        count = EDGE_SCRIPT_MAX_SCRIPTS;

    for (i = 0; i < count; i++)
    {
        cJSON *item = cJSON_GetArrayItem(scripts, i);
        if (!item)
            continue;
        if (parse_script_entry(item, config_base, &cfg->scripts[cfg->script_count]) == 0)
            cfg->script_count++;
    }

    cJSON_Delete(root);
    return 0;
}
