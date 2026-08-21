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
{
    "cache":{
        "checked":           1,
        "inv_time":        300,
        "max_items":      8192,
        "max_rows":       2016,
        "max_alarm_nums":  256
    }
}
*/



/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int dc_cfg_load(struct datacache_ctx_t *pCtx, char *file)
{
    char    *p;
    int     size, ret = -1;
    cJSON   *pJson, *pJson_lay1, *pJson_Item;
    struct datacache_attr_t *pattr = &pCtx->attr;

    if(pCtx->debug)
    {
        printf("\nCFG: data cache%s\n", file);
    }
 
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
 
    /*  */
    pJson_Item=cJSON_GetObjectItem(pJson_lay1, "checked");
    if(pJson_Item)
        pattr->checked = pJson_Item->valueint;

    /*  */
    pJson_Item=cJSON_GetObjectItem(pJson_lay1, "inv_time");
    if(pJson_Item)
        pattr->inv_time = pJson_Item->valueint;

    /*  */
    pJson_Item=cJSON_GetObjectItem(pJson_lay1, "max_items");
    if(pJson_Item)
        pattr->max_items = pJson_Item->valueint;

    /*  */
    pJson_Item=cJSON_GetObjectItem(pJson_lay1, "max_rows");
    if(pJson_Item)
        pattr->max_rows = pJson_Item->valueint;

    /*  */
    pJson_Item=cJSON_GetObjectItem(pJson_lay1, "max_alarm_nums");
    if(pJson_Item)
        pattr->max_alarm_nums = pJson_Item->valueint;
    

    if(pCtx->debug)
    {
        printf("checked:        %05d\n", pattr->checked);
        printf("inv_time:       %05d\n", pattr->inv_time);
        printf("max_items:      %05d\n", pattr->max_items);
        printf("max_rows:       %05d\n", pattr->max_rows);
        printf("max_alarm_nums: %05d\n", pattr->max_alarm_nums);
    }

    ret = 0;

__err_json_format:
    cJSON_Delete(pJson);

__err_free:
    dc_cfg_jsonfile_delete(p);

    return ret;
}


