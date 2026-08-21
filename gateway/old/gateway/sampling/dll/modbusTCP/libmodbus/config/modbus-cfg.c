/*
 * File      : modbus-cfg.c
 * This file is part of xgate5-fire
 * COPYRIGHT (C) 2006 - 2018, 19 development, Communication team, chenzy.
 *
 * 
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-05-09     chenzy       channel-device-node setting.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <unistd.h> 
#include <sys/stat.h>  //stat
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <errno.h>

#include "cJSON.h"
#include "cJSON_Utils.h"

#include "modbus-cfg.h"



/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static char *config_devices_jsonfile_alloc(char *file, int *size)
{ 
    struct stat filestat;
 
    if(NULL == file)
        return NULL;
   
    /* Get file size */
    if(0 != stat(file, &filestat))
    {  
        printf("Json file[%s] not exist\n", file);
        return NULL;
    }
    *size = filestat.st_size;
   return malloc(filestat.st_size + 1);
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static void config_devices_jsonfile_delete(char *p)
{
    if(p == NULL)
        return;
    
    free(p);
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static char *config_devices_jsonfile_read(char *file, char *p, int size)
{
    int n;
    FILE *fp;
    
    fp = fopen(file, "rb");
    if(fp == NULL)
        return NULL;

    n = fread(p, 1, size, fp);
    if(n <= 0)
        goto __err_close;

    fclose(fp);
    return p;

__err_close:
    fclose(fp);

    return NULL;
}



/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int config_devices_json_group_pkt_read(struct config_devices_attr *pCfg, int num, cJSON *pArrayItem)
{
    int         i;
    int         size;
    cJSON       *pObject, *pItem;

    size = cJSON_GetArraySize(pArrayItem); 
	if(size != num)
	{
		return -1;
	}
    for(i=0; i<size; i++)
    {
        pObject = cJSON_GetArrayItem(pArrayItem, i); 
		if(!pObject)
			return -1;
			
        pItem = cJSON_GetObjectItem(pObject, "code");
        if(pItem == NULL)
            return -1;
		else
			pCfg->pkg_request.pkt_req_info[i].code = pItem->valueint;

        pItem = cJSON_GetObjectItem(pObject, "reg_no");
        if(pItem == NULL)
            return -1;
		else
			pCfg->pkg_request.pkt_req_info[i].reg_no = pItem->valueint;

        pItem = cJSON_GetObjectItem(pObject, "reg_num");
        if(pItem == NULL)
            return -1;
		else
			pCfg->pkg_request.pkt_req_info[i].reg_num = pItem->valueint;

		printf("        reg_no:%d reg_num:%d \n", pCfg->pkg_request.pkt_req_info[i].reg_no, pCfg->pkg_request.pkt_req_info[i].reg_num);
    }

    return 0;
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int config_devices_json_group_node_read(struct config_devices_attr *pCfg, int num, cJSON *pArrayItem)
{
    int         i;
    int         size;
    cJSON       *pObject, *pItem;

    size = cJSON_GetArraySize(pArrayItem); 
	if(size != num)
	{
		printf("collector cfg file error: node_num != node_array num!\n");
		return -1;
	}

    for(i=0; i<size; i++)
    {
        pObject = cJSON_GetArrayItem(pArrayItem, i); 
		if(pObject == NULL)
			return -1;

        pItem = cJSON_GetObjectItem(pObject, "id");
        if(pItem == NULL)
            return -1;
		else
			pCfg->node_attr[i].id = pItem->valueint;

        pItem = cJSON_GetObjectItem(pObject, "datatype");
        if(pItem == NULL)
            return -1;
		else
			pCfg->node_attr[i].datatype = pItem->valueint;

        pItem = cJSON_GetObjectItem(pObject, "code");
        if(pItem == NULL)
            return -1;
		else
			pCfg->node_attr[i].code = pItem->valueint;

        pItem = cJSON_GetObjectItem(pObject, "bitfield");
        if(pItem == NULL)
            return -1;
		else
			pCfg->node_attr[i].bitfield = pItem->valueint;

        pItem = cJSON_GetObjectItem(pObject, "datattr");
        if(pItem == NULL)
            return -1;
		else
			pCfg->node_attr[i].flag = pItem->valueint;

        pItem = cJSON_GetObjectItem(pObject, "reg_no");
        if(pItem == NULL)
            return -1;
		else
			pCfg->node_attr[i].reg_no = pItem->valueint;

        pItem = cJSON_GetObjectItem(pObject, "catagory");
        if(pItem == NULL)
            return -1;
		else
			pCfg->node_attr[i].catagory = pItem->valueint;

        pItem = cJSON_GetObjectItem(pObject, "kfactor");
        if(pItem == NULL)
            return -1;
		else
			pCfg->node_attr[i].kfactor = pItem->valuedouble;

        pItem = cJSON_GetObjectItem(pObject, "pt_flag1");
        if(pItem == NULL)
            pCfg->node_attr[i].pt_flag1 = 0;
		else
			pCfg->node_attr[i].pt_flag1 = pItem->valuedouble;

        pItem = cJSON_GetObjectItem(pObject, "ct_flag2");
        if(pItem == NULL)
            pCfg->node_attr[i].ct_flag2 = 0;
		else
			pCfg->node_attr[i].ct_flag2 = pItem->valuedouble;

        // 后期增加的bfactor，无字段不返回
        pItem = cJSON_GetObjectItem(pObject, "bfactor");
        if(pItem == NULL)
            pCfg->node_attr[i].bfactor = 0;
		else
			pCfg->node_attr[i].bfactor = pItem->valuedouble;

        pItem = cJSON_GetObjectItem(pObject, "range_checked");
        if(pItem == NULL)
            pCfg->node_attr[i].range_checked = 0;
		else
			pCfg->node_attr[i].range_checked = pItem->valueint;

        pItem = cJSON_GetObjectItem(pObject, "range_min");
        if(pItem == NULL)
            pCfg->node_attr[i].range_min = 0;
		else
			pCfg->node_attr[i].range_min = pItem->valueint;

        pItem = cJSON_GetObjectItem(pObject, "range_max");
        if(pItem == NULL)
            pCfg->node_attr[i].range_max = 0;
		else
			pCfg->node_attr[i].range_max = pItem->valueint;

        pItem = cJSON_GetObjectItem(pObject, "dataoffset");
        if(pItem == NULL)
            pCfg->node_attr[i].dataoffset = 0;
		else
			pCfg->node_attr[i].dataoffset = pItem->valueint;

		printf("            reg_no:%d \n",  pCfg->node_attr[i].reg_no);
    }
    return 0;
}



/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int config_devices_json_group_control_read(struct config_devices_attr *pCfg, int num, cJSON *pArrayItem)
{
    int         i;
    int         size;
    cJSON       *pObject, *pItem;
    size = cJSON_GetArraySize(pArrayItem); 
	if(size != num)
	{
		return -1;
	}
    for(i=0; i<size; i++)
    {
        pObject = cJSON_GetArrayItem(pArrayItem, i); 
		if(pObject == NULL)
			return -1;

        pItem = cJSON_GetObjectItem(pObject, "id");
        if(pItem == NULL)
            return -1;
		else
			pCfg->control_attr[i].id = pItem->valueint;

        pItem = cJSON_GetObjectItem(pObject, "datatype");
        if(pItem == NULL)
            return -1;
		else
			pCfg->control_attr[i].datatype = pItem->valueint;

        pItem = cJSON_GetObjectItem(pObject, "funcode");
        if(pItem == NULL)
            return -1;
		else
			pCfg->control_attr[i].funcode = pItem->valueint;

        pItem = cJSON_GetObjectItem(pObject, "regno");
        if(pItem == NULL)
            return -1;
		else
			pCfg->control_attr[i].regno = pItem->valueint;

        // add by chenzy 20240624.  compatible with old version that no these segment.
        pItem = cJSON_GetObjectItem(pObject, "regno0");
        if(pItem == NULL)
            pCfg->control_attr[i].regno0 = 0;
		else
			pCfg->control_attr[i].regno0 = pItem->valueint;

        pItem = cJSON_GetObjectItem(pObject, "regno1");
        if(pItem == NULL)
            pCfg->control_attr[i].regno1 = 0;
		else
			pCfg->control_attr[i].regno1 = pItem->valueint;

        pItem = cJSON_GetObjectItem(pObject, "regvalue0");
        if(pItem == NULL)
            pCfg->control_attr[i].regvalue0 = 0;
		else
			pCfg->control_attr[i].regvalue0 = pItem->valueint;

        pItem = cJSON_GetObjectItem(pObject, "regvalue1");
        if(pItem == NULL)
            pCfg->control_attr[i].regvalue1 = 0;
		else
			pCfg->control_attr[i].regvalue1 = pItem->valueint;

        pItem = cJSON_GetObjectItem(pObject, "kfactor");
        if(pItem == NULL)
            pCfg->control_attr[i].kfactor = 1;
		else
			pCfg->control_attr[i].kfactor = pItem->valuedouble;
        //pItem = cJSON_GetObjectItem(pObject, "name");
        //if(pItem == NULL)
        //    return -1;
		//else
		//	memcpy(pCfg->control_attr[i].name, pItem->valuestring,strlen(pItem->valuestring));

printf("                ctrl  reg_no:%d \n",pCfg->control_attr[i].regno);
    }

    return 0;
}



/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int config_devices_json_group_alarm_cond_read(struct config_devices_attr *pCfg, cJSON *pArrayItem)
{
    int         i;
    int         size;
    cJSON       *pObject, *pItem;

	/*
	
		"alarm":{
            "alarm_num" :1,
            "alarm_attr" : [       
                {
                    "ruleid" : 0, "name" : "leak", "type": "leak", "mode" :0, "htime" :20,
                    "alarm_cond" : [ 		<---  entry
                        {   "pointid":15,        "type":"const",    "level":1,         "method": 1,        "ref":300,    "ref_per":100,        "Hysteresis":10          },   
                        {   "pointid":15,        "type":"const",    "level":1,         "method": 2,        "ref":300,    "ref_per":100,        "Hysteresis":10          }
                    ]
                }
            ]
        }

	
	*/

    size = cJSON_GetArraySize(pArrayItem); 

    for(i=0; i<size; i++)
    {

        pObject = cJSON_GetArrayItem(pArrayItem, i); 
		if(pObject == NULL)
			return -1;

        pItem = cJSON_GetObjectItem(pObject, "pointid");
        if(pItem == NULL)
            return -1;
		pCfg->alarm_attr[i].alarm_cond->pointid = pItem->valueint;



        pItem = cJSON_GetObjectItem(pObject, "type");
        if(pItem == NULL)
            return -1;
		memcpy(pCfg->alarm_attr[i].alarm_cond->type,pItem->valuestring,strlen(pItem->valuestring));

        pItem = cJSON_GetObjectItem(pObject, "level");
        if(pItem == NULL)
            return -1;
		pCfg->alarm_attr[i].alarm_cond->level = pItem->valueint;

        pItem = cJSON_GetObjectItem(pObject, "method");
        if(pItem == NULL)
            return -1;
		pCfg->alarm_attr[i].alarm_cond->method = pItem->valueint;

        pItem = cJSON_GetObjectItem(pObject, "ref");
        if(pItem == NULL)
            return -1;
		pCfg->alarm_attr[i].alarm_cond->ref = pItem->valueint;

        pItem = cJSON_GetObjectItem(pObject, "ref_per");
        if(pItem == NULL)
            return -1;
		pCfg->alarm_attr[i].alarm_cond->ref_per = pItem->valueint;

        pItem = cJSON_GetObjectItem(pObject, "Hysteresis");
        if(pItem == NULL)
            return -1;
		pCfg->alarm_attr[i].alarm_cond->Hysteresis = pItem->valueint;

//printf("                  alarm pointid:%X level:%d \n", pCfg->alarm_attr[i].alarm_cond->pointid, pCfg->alarm_attr[i].alarm_cond->level);
    }

    return 0;
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int config_devices_json_group_alarm_read(struct config_devices_attr *pCfg, int num, cJSON *pArrayItem)
{
	int         i, ret;
    int         size;
    cJSON       *pObject, *pItem;

	/*
	
		"alarm":{
            "alarm_num" :1,
            "alarm_attr" : [       	<---  entry
                {
                    "ruleid" : 0, "name" : "leak", "type": "leak", "mode" :0, "htime" :20,
                    "alarm_cond" : [ 		
                        {   "pointid":15,        "type":"const",    "level":1,         "method": 1,        "ref":300,    "ref_per":100,        "Hysteresis":10          },   
                        {   "pointid":15,        "type":"const",    "level":1,         "method": 2,        "ref":300,    "ref_per":100,        "Hysteresis":10          }
                    ]
                }
            ]
        }

	
	*/

    size = cJSON_GetArraySize(pArrayItem); 

	if(size != num)
	{
		return -1;
	}
    for(i=0; i<size; i++)
    {

        pObject = cJSON_GetArrayItem(pArrayItem, i); 
		if(pObject == NULL)
			return -1;

        pItem = cJSON_GetObjectItem(pObject, "ruleid");
        if(pItem == NULL)
            return -1;
		pCfg->alarm_attr[i].ruleid = pItem->valueint;

        //pItem = cJSON_GetObjectItem(pObject, "name");
        //if(pItem == NULL)
        //    return -1;
		//strncpy(pCfg->alarm_attr[i].name, pItem->valuestring, sizeof(pCfg->alarm_attr[i].name));

        pItem = cJSON_GetObjectItem(pObject, "type");
        if(pItem == NULL)
            return -1;
		strncpy(pCfg->alarm_attr[i].type, pItem->valuestring, sizeof(pCfg->alarm_attr[i].type));

        pItem = cJSON_GetObjectItem(pObject, "mode");
        if(pItem == NULL)
            return -1;
		pCfg->alarm_attr[i].mode = pItem->valueint;

        pItem = cJSON_GetObjectItem(pObject, "htime");
        if(pItem == NULL)
            return -1;
		pCfg->alarm_attr[i].htime = pItem->valueint;

		/* [array]   alarm_cond */
		pObject = cJSON_GetObjectItem(pObject, "alarm_cond");
        if(pObject == NULL)
            return -1;
//printf("                alarm name:%s ruleid:%d \n", pCfg->alarm_attr[i].name, pCfg->alarm_attr[i].ruleid);
		ret = config_devices_json_group_alarm_cond_read(pCfg, pObject);
		if(ret < 0)
			return -1;


    }

    return 0;
}




/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int modbus_cfg_load(struct config_devices_attr *pCfg, char *file)
{
    char    *p;
    int     size;
    int     ret;
    cJSON   *pJson, *pjsClass, *pjsGroup, *pItem;

    /* prepare the buffer for json */
    p = config_devices_jsonfile_alloc(file, &size);
    if(p == NULL)
        return -1;
   
    p = config_devices_jsonfile_read(file, p, size);
    if(p == NULL)
        goto __err_free;
 
    pJson = cJSON_Parse(p);  
    if(!pJson)
        goto __err_free;
    
    /*
        pJson("attr") --
            |-- pjsClass("pkg_request")
                |--pjsGroup(line)
			|-- pjsClass("node")
                |--pjsGroup(line)
			|-- pjsClass("ctl")
                |--pjsGroup(line)
			|-- pjsClass("alarm")
                |--pjsGroup(line)
    */

    pjsClass = cJSON_GetObjectItem(pJson, "attr"); 
    if(!pjsClass)
        goto __err_json_format;

    pItem = cJSON_GetObjectItem(pjsClass, "checked");
    if(!pItem)
        goto __err_json_format;
	else
		pCfg->checked = (uint8_t)pItem->valueint;
	
    //pItem = cJSON_GetObjectItem(pjsClass, "sn");
    //if(!pItem)
    //    goto __err_json_format;
	//else
	//	memcpy(pCfg->sn,pItem->valuestring,strlen(pItem->valuestring));
//
    //pItem = cJSON_GetObjectItem(pjsClass, "model");
    //if(!pItem)
    //    goto __err_json_format;
	//else
	//	memcpy(pCfg->model,pItem->valuestring,strlen(pItem->valuestring));
//
    //pItem = cJSON_GetObjectItem(pjsClass, "name");
    //if(!pItem)
    //    goto __err_json_format;
	//else
	//	memcpy(pCfg->name,pItem->valuestring,strlen(pItem->valuestring));
//
    //pItem = cJSON_GetObjectItem(pjsClass, "function");
    //if(!pItem)
    //    goto __err_json_format;
	//else
	//	memcpy(pCfg->function,pItem->valuestring,strlen(pItem->valuestring));
	
    /* get pkg_request parameters from json */
    pjsGroup=cJSON_GetObjectItem(pjsClass, "pkg_request");
    if(!pjsGroup)
        goto __err_json_format;

    pItem = cJSON_GetObjectItem(pjsGroup, "pkt_num");
    if(!pItem)
        goto __err_json_format;
	else
		pCfg->pkg_request.pkt_num = pItem->valueint;
	
    pjsGroup=cJSON_GetObjectItem(pjsGroup, "pkt_req_info");
    if(!pjsGroup)
        goto __err_json_format;
    
    pCfg->pkg_request.pkt_num = cJSON_GetArraySize(pjsGroup); 
    ret = config_devices_json_group_pkt_read(pCfg,pCfg->pkg_request.pkt_num, pjsGroup);
    if(ret < 0)
        goto __err_json_format;

    /* get DEADZONE parameters from syscfg.json */
    pjsGroup=cJSON_GetObjectItem(pjsClass, "node");
    if(!pjsGroup)
        goto __err_json_format;

    pItem = cJSON_GetObjectItem(pjsGroup, "node_num");
    if(!pItem)
        goto __err_json_format;
	else
		pCfg->node_num = pItem->valueint;

    pjsGroup=cJSON_GetObjectItem(pjsGroup, "node_attr");
    if(!pjsGroup)
        goto __err_json_format;

    ret = config_devices_json_group_node_read(pCfg, pCfg->node_num, pjsGroup);
    if(ret < 0)
        goto __err_json_format;

    /* get INTERFACE parameters from syscfg.json */
    pjsGroup=cJSON_GetObjectItem(pjsClass, "control");
    if(!pjsGroup)
        goto __err_json_format;
    pItem = cJSON_GetObjectItem(pjsGroup, "control_num");
    if(!pItem)
        goto __err_json_format;
	else
		pCfg->control_num = pItem->valueint;

    pjsGroup=cJSON_GetObjectItem(pjsGroup, "control_attr");
    if(!pjsGroup)
        goto __err_json_format;

    ret = config_devices_json_group_control_read(pCfg, pCfg->control_num, pjsGroup);
    if(ret < 0)
        goto __err_json_format;

    pjsGroup=cJSON_GetObjectItem(pjsClass, "alarm");
    if(!pjsGroup)
        goto __err_json_format;

    pItem = cJSON_GetObjectItem(pjsGroup, "alarm_num");
    if(!pItem)
        goto __err_json_format;
	else
		pCfg->alarm_num = pItem->valueint;

    pjsGroup=cJSON_GetObjectItem(pjsGroup, "alarm_attr");
    if(!pjsGroup)
        goto __err_json_format;

    ret = config_devices_json_group_alarm_read(pCfg,  pCfg->alarm_num, pjsGroup);
    if(ret < 0)
        goto __err_json_format;


    cJSON_Delete(pJson);
    config_devices_jsonfile_delete(p);

    return 0;

__err_json_format:
    cJSON_Delete(pJson);

__err_free:
    config_devices_jsonfile_delete(p);
    return -1;
}



/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:             
** output parameters:   NONE
** Returned value:      
*********************************************************************************************************/
int32_t modbus_cfg_node_attr_size(struct config_devices_attr *pDevAttr)
{
    return pDevAttr->node_num;
}


/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:             
** output parameters:   NONE
** Returned value:      
*********************************************************************************************************/
struct config_node_attr *modbus_cfg_node_attr(struct config_devices_attr *pDevAttr, uint16_t id)
{
    if(id >= CFG_MAX_POINT)
		return NULL;

    return &pDevAttr->node_attr[id];
}



/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:             
** output parameters:   NONE
** Returned value:      
*********************************************************************************************************/
int32_t modbus_cfg_nodectrl_attr_size(struct config_devices_attr *pDevAttr)
{
    return pDevAttr->control_num;
}


/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:             
** output parameters:   NONE
** Returned value:      
*********************************************************************************************************/
struct config_control_attr *modbus_cfg_nodectrl_attr(struct config_devices_attr *pDevAttr, uint16_t id)
{
    if(id >= CFG_MAX_POINT)
		return NULL;

    return &pDevAttr->control_attr[id];
}