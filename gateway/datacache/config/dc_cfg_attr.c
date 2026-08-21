/*
* Copyright (c) 2013,珠海一构物联有限公司
* All rights reserved.
*
* 文件名称：db_cfg.h
* 文件标识：
* 描    述：
*
* 当前版本：V1.0
* 作    者：chenzy
* 更改说明：
* 完成日期：20131108
*
* 修改记录
*  作者       时间         版本      修改描述
*  chenzy   2013.11.08   V1.00      创建
*/
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>


#include "cJSON.h"
#include "cJSON_Utils.h"

#include "dc_cfg_json.h"
#include "dc_cfg_attr.h"


/*
新配置示例（显式字段）：
{
    "attr":{
        "checked":        1,
        "inv_time":       300,
        "max_items":      8192,
        "max_rows":       2016,
        "max_alarm_nums": 256,
        "mode":           "roll",
        "high_mb":        10,
        "low_mb":         9,
        "history_days":   8,
        "history_path":   "/userdata/history"
    }
}

旧配置（无 mode/high_mb/...）→ 自动按 LEGACY_* 运行：roll + 8MB/6MB
mode 也可写数字：0=roll, 1=daily
*/


/*************************************************************************/
/** Function Name:     dc_cfg_attr_set_legacy                          **/
/** Purpose:           填充旧设备兼容默认参数                           **/
/** Params:            pattr - 配置属性                                 **/
/** Return:            None.                                           **/
/** Notice:            mode=roll, high=8MB, low=6MB                    **/
/*************************************************************************/
static void dc_cfg_attr_set_legacy(struct datacache_attr_t *pattr)
{
    pattr->mode = DC_CFG_LEGACY_MODE;
    pattr->high_size = DC_CFG_LEGACY_HIGH_SIZE;
    pattr->low_size = DC_CFG_LEGACY_LOW_SIZE;
    pattr->history_days = DC_CFG_LEGACY_HISTORY_DAYS;
    memset(pattr->history_path, 0, sizeof(pattr->history_path));
    strncpy(pattr->history_path, DC_CFG_LEGACY_HISTORY_PATH, sizeof(pattr->history_path) - 1);
}


/*************************************************************************/
/** Function Name:     dc_cfg_parse_mode                               **/
/** Purpose:           解析 JSON 中的 mode 字段                         **/
/** Params:            item - cJSON 节点                                **/
/** Return:            DC_CACHE_MODE_ROLL / DAILY                      **/
/** Notice:            支持字符串与数字；缺省按旧设计 roll              **/
/*************************************************************************/
static uint8_t dc_cfg_parse_mode(cJSON *item)
{
    if (!item)
        return DC_CFG_LEGACY_MODE;

    if (item->type == cJSON_String && item->valuestring)
    {
        if (strcmp(item->valuestring, "daily") == 0 ||
            strcmp(item->valuestring, "day") == 0 ||
            strcmp(item->valuestring, "today") == 0)
            return DC_CACHE_MODE_DAILY;
        if (strcmp(item->valuestring, "roll") == 0 ||
            strcmp(item->valuestring, "resume") == 0 ||
            strcmp(item->valuestring, "rolling") == 0)
            return DC_CACHE_MODE_ROLL;
    }

    if (item->type == cJSON_Number)
        return (item->valueint == 1) ? DC_CACHE_MODE_DAILY : DC_CACHE_MODE_ROLL;

    return DC_CFG_LEGACY_MODE;
}


/*************************************************************************/
/** Function Name:     dc_cfg_normalize_watermarks                     **/
/** Purpose:           校正高低水位合法性                               **/
/** Params:            pattr - 配置属性                                 **/
/** Return:            None.                                           **/
/** Notice:            保证 low_size < high_size                       **/
/*************************************************************************/
static void dc_cfg_normalize_watermarks(struct datacache_attr_t *pattr)
{
    if (pattr->high_size == 0)
        pattr->high_size = DC_CFG_LEGACY_HIGH_SIZE;

    if (pattr->low_size == 0)
        pattr->low_size = DC_CFG_LEGACY_LOW_SIZE;

    /* 低水位必须低于高水位；日模式低水位无裁剪用途，但仍保持合法 */
    if (pattr->low_size >= pattr->high_size)
    {
        if (pattr->high_size > (1u * 1024u * 1024u))
            pattr->low_size = pattr->high_size - (1u * 1024u * 1024u);
        else
            pattr->low_size = pattr->high_size / 2;
    }
}


/*************************************************************************/
/** Function Name:     dc_cfg_load                                     **/
/** Purpose:           加载 data_cache.json 配置                        **/
/** Params:            pCtx - 上下文；file - 配置文件路径               **/
/** Return:            0 成功；-1 失败                                 **/
/** Notice:            缺省字段保持 LEGACY 旧设计默认                   **/
/*************************************************************************/
int dc_cfg_load(struct datacache_ctx_t *pCtx, char *file)
{
    char    *p;
    int     size, ret = -1;
    cJSON   *pJson, *pJson_lay1, *pJson_Item;
    struct datacache_attr_t *pattr = &pCtx->attr;
    int has_high = 0, has_low = 0;

    if(pCtx->debug)
    {
        printf("\nCFG: data cache%s\n", file);
    }

    /* 先铺旧默认，旧 JSON 缺字段时即按旧设计运行 */
    dc_cfg_attr_set_legacy(pattr);
 
    /* prepare the buffer for json */
    p = dc_cfg_jsonfile_alloc(file, &size);
    if(p == NULL)
        return -1;
 
    p = dc_cfg_jsonfile_read(file, p, size);
    if(p == NULL)
        goto __err_free;
 
    pJson = cJSON_Parse(p);  
    if(!pJson)
        goto __err_free;

    pJson_lay1 = cJSON_GetObjectItem(pJson, "attr"); 
    if(!pJson_lay1)
        goto __err_json_format;
 
    pJson_Item=cJSON_GetObjectItem(pJson_lay1, "checked");
    if(pJson_Item)
        pattr->checked = pJson_Item->valueint;

    pJson_Item=cJSON_GetObjectItem(pJson_lay1, "inv_time");
    if(pJson_Item)
        pattr->inv_time = pJson_Item->valueint;

    pJson_Item=cJSON_GetObjectItem(pJson_lay1, "max_items");
    if(pJson_Item)
        pattr->max_items = pJson_Item->valueint;

    pJson_Item=cJSON_GetObjectItem(pJson_lay1, "max_rows");
    if(pJson_Item)
        pattr->max_rows = pJson_Item->valueint;

    pJson_Item=cJSON_GetObjectItem(pJson_lay1, "max_alarm_nums");
    if(pJson_Item)
        pattr->max_alarm_nums = pJson_Item->valueint;

    /* 扩展字段：有则覆盖 LEGACY */
    pJson_Item = cJSON_GetObjectItem(pJson_lay1, "mode");
    if (pJson_Item)
        pattr->mode = dc_cfg_parse_mode(pJson_Item);

    pJson_Item = cJSON_GetObjectItem(pJson_lay1, "high_mb");
    if (pJson_Item && pJson_Item->valueint > 0)
    {
        pattr->high_size = (uint32_t)pJson_Item->valueint * 1024u * 1024u;
        has_high = 1;
    }

    pJson_Item = cJSON_GetObjectItem(pJson_lay1, "low_mb");
    if (pJson_Item && pJson_Item->valueint > 0)
    {
        pattr->low_size = (uint32_t)pJson_Item->valueint * 1024u * 1024u;
        has_low = 1;
    }

    /* 若只配了高水位，低水位默认高水位-1MB（新设计习惯） */
    if (has_high && !has_low)
    {
        if (pattr->high_size > (1u * 1024u * 1024u))
            pattr->low_size = pattr->high_size - (1u * 1024u * 1024u);
    }

    pJson_Item = cJSON_GetObjectItem(pJson_lay1, "history_days");
    if (pJson_Item && pJson_Item->valueint > 0)
        pattr->history_days = (uint16_t)pJson_Item->valueint;

    pJson_Item = cJSON_GetObjectItem(pJson_lay1, "history_path");
    if (pJson_Item && pJson_Item->type == cJSON_String && pJson_Item->valuestring)
    {
        memset(pattr->history_path, 0, sizeof(pattr->history_path));
        strncpy(pattr->history_path, pJson_Item->valuestring, sizeof(pattr->history_path) - 1);
    }

    dc_cfg_normalize_watermarks(pattr);

    if(pCtx->debug)
    {
        printf("checked:        %05d\n", pattr->checked);
        printf("inv_time:       %05d\n", pattr->inv_time);
        printf("max_items:      %05d\n", pattr->max_items);
        printf("max_rows:       %05d\n", pattr->max_rows);
        printf("max_alarm_nums: %05d\n", pattr->max_alarm_nums);
        printf("mode:           %s\n",
               (pattr->mode == DC_CACHE_MODE_DAILY) ? "daily" : "roll");
        printf("high_size:      %u MB\n", pattr->high_size / (1024u * 1024u));
        printf("low_size:       %u MB\n", pattr->low_size / (1024u * 1024u));
        printf("history_days:   %u\n", pattr->history_days);
        printf("history_path:   %s\n", pattr->history_path);
    }

    ret = 0;

__err_json_format:
    cJSON_Delete(pJson);

__err_free:
    dc_cfg_jsonfile_delete(p);

    return ret;
}
