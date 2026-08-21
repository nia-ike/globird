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

#include "db_cfg_json.h"
#include "rm_cfg_attr.h"


/*

{
    "attr":{
        "checked":1,
        "url":"ego.pmac.com.cn:1883",
        "user":"pilot",
        "passwd":"pilot@123456"
    }
}

*/


/*************************************************************************/
/** Function Name:                                     **/
/** Purpose:                                         **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
static int url_split(char *url, char *ip, int *port)
{
    int ret;
    if(url == NULL)
        return -1;

    ret = sscanf(url, "%[^:]:%d", ip, port);
    if(ret < 0)
        return -1;

    return 0;
}

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int rm_cfg_load(struct remote_m_ctx_t *pCtx, char *file)
{
    char    *p;
    int     size, ret = -1;
    cJSON   *pJson, *pJson_lay1, *pJson_Item;
    struct remote_m_attr_t *pattr = &pCtx->attr;

    if(pCtx->debug)
    {
        printf("\nCFG: remote manage %s\n", file);
    }
 
    /* prepare the buffer for json */
    p = db_cfg_jsonfile_alloc(file, &size);
    if(p == NULL)
        return -1;
 
    p = db_cfg_jsonfile_read(file, p, size);
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
    pJson_Item=cJSON_GetObjectItem(pJson_lay1, "url");
    if(pJson_Item)
        snprintf(pattr->url, sizeof(pattr->url), "%s", pJson_Item->valuestring);

    /*  */
    pJson_Item=cJSON_GetObjectItem(pJson_lay1, "user");
    if(pJson_Item)
        snprintf(pattr->user, sizeof(pattr->user), "%s", pJson_Item->valuestring);

    /*  */
    pJson_Item=cJSON_GetObjectItem(pJson_lay1, "passwd");
    if(pJson_Item)
        snprintf(pattr->passwd, sizeof(pattr->passwd), "%s", pJson_Item->valuestring);

    /* split IP and PORT */
    url_split(pattr->url, pattr->ip, &pattr->port);

    ret = 0;

__err_json_format:
    cJSON_Delete(pJson);

__err_free:
    db_cfg_jsonfile_delete(p);

    return ret;
}


/*
{
    "Name": null,
    "info": {
        "help": null
    },
    "attr": {
        "station": "",
        "rooms": "",
        "gatewayname": "",
        "owner": "",
        "company": "",
        "companyurl": "",
        "logo": ""
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
int rm_cfg_project_info_load(struct remote_m_ctx_t *pCtx, char *file)
{
    char    *p;
    int     size, ret = -1;
    cJSON   *pJson, *pJson_lay1, *pJson_Item;
    struct remote_m_attr_t *pattr = &pCtx->attr;

    if(pCtx->debug)
    {
        printf("\nCFG: remote manage %s\n", file);
    }
 
    /* prepare the buffer for json */
    p = db_cfg_jsonfile_alloc(file, &size);
    if(p == NULL)
        return -1;
 
    p = db_cfg_jsonfile_read(file, p, size);
    if(p == NULL)
        goto __err_free;
 
    pJson = cJSON_Parse(p);  
    if(!pJson)
        goto __err_free;

    pJson_lay1 = cJSON_GetObjectItem(pJson, "attr"); 
    if(!pJson_lay1)
        goto __err_json_format;
 

    /*  */
    pJson_Item=cJSON_GetObjectItem(pJson_lay1, "station");
    if(pJson_Item)
        snprintf(pattr->project.station, sizeof(pattr->project.station)-1, "%s", pJson_Item->valuestring);

    /*  */
    pJson_Item=cJSON_GetObjectItem(pJson_lay1, "rooms");
    if(pJson_Item)
        snprintf(pattr->project.rooms, sizeof(pattr->project.rooms)-1, "%s", pJson_Item->valuestring);

/*  */
    pJson_Item=cJSON_GetObjectItem(pJson_lay1, "gatewayname");
    if(pJson_Item)
        snprintf(pattr->project.gatewayname, sizeof(pattr->project.gatewayname)-1, "%s", pJson_Item->valuestring);

/*  */
    pJson_Item=cJSON_GetObjectItem(pJson_lay1, "owner");
    if(pJson_Item)
        snprintf(pattr->project.owner, sizeof(pattr->project.owner)-1, "%s", pJson_Item->valuestring);

/*  */
    pJson_Item=cJSON_GetObjectItem(pJson_lay1, "company");
    if(pJson_Item)
        snprintf(pattr->project.company, sizeof(pattr->project.company)-1, "%s", pJson_Item->valuestring);


    if(pCtx->debug)
    {
        printf("station:       %s\n", pattr->project.station);
        printf("rooms:         %s\n", pattr->project.rooms);
        printf("gatewayname:   %s\n", pattr->project.gatewayname);
        printf("owner:         %s\n", pattr->project.owner);
        printf("company:       %s\n", pattr->project.company);
    }

    ret = 0;

__err_json_format:
    cJSON_Delete(pJson);

__err_free:
    db_cfg_jsonfile_delete(p);

    return ret;
}