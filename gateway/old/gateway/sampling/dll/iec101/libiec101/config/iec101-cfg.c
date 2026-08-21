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

#include "iec101-cfg.h"
#include "iec101-utils.h"

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

static void config_devices_jsonfile_delete(char *p)
{
    if(p == NULL)
        return;
    
    free(p);
}

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

int32_t iec101_cfg_node_attr_size(struct config_devices_attr *pDevAttr)
{// 获取该设备的点属性表数量
    return pDevAttr->node_num;
}

struct config_node_attr *iec101_cfg_node_attr(struct config_devices_attr *pDevAttr, uint8_t id)
{// 获取该设备的点属性表
    if(id >= CFG_MAX_POINT)
		return NULL;

    return &pDevAttr->node_attr[id];
}

static int config_devices_json_group_node_read(struct config_devices_attr *pCfg, int num, cJSON *pArrayItem)
{// 加载测点
    int         i;
    int         size;
    cJSON       *pObject;
    bool succeed;

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

        pCfg->node_attr[i].id = json_get_int(pObject, "id", 0, &succeed);
        if (!succeed) return -1;

        pCfg->node_attr[i].datattr = json_get_int(pObject, "datattr", 0, &succeed);
        if (!succeed) return -1;

        pCfg->node_attr[i].catagory = json_get_int(pObject, "catagory", 0, &succeed);
        if (!succeed) return -1;

        pCfg->node_attr[i].kfactor = json_get_double(pObject, "kfactor", 1, &succeed);
        if (!succeed) return -1;

        pCfg->node_attr[i].inf_addr = json_get_int(pObject, "inf_addr", 0, &succeed);
        if (!succeed) return -1;

		printf("            inf_addr:%d \n",  pCfg->node_attr[i].inf_addr);
    }
    return 0;
}

static int config_devices_json_group_control_read(struct config_devices_attr *pCfg, int num, cJSON *pArrayItem)
{// 加载遥控点
    int         i;
    int         size;
    cJSON       *pObject;
    bool succeed;

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

        pCfg->control_attr[i].id = json_get_int(pObject, "id", 0, &succeed);
        if (!succeed) return -1;

        pCfg->control_attr[i].inf_addr = json_get_int(pObject, "inf_addr", 0, &succeed);
        if (!succeed) return -1;

        pCfg->control_attr[i].double_ctrl = json_get_bool(pObject, "double_ctrl", false, &succeed);
        if (!succeed) return -1;

        printf("                ctrl  inf_addr:%d \n",pCfg->control_attr[i].inf_addr);
    }

    return 0;
}

int iec101_cfg_load(struct config_devices_attr *pCfg, char *file)
{// 加载配置
    char    *p;
    int     size;
    int     ret;
    cJSON   *pJson, *pjsClass, *pjsGroup, *pItem;
    bool succeed = false;

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
			|-- pjsClass("node")
                |--pjsGroup(line)
			|-- pjsClass("ctl")
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

    /* get extend_param */
    pjsGroup=cJSON_GetObjectItem(pjsClass, "extend_param");
    pCfg->timing_inv = json_get_int(pjsGroup, "timing_cycle", 0, &succeed);
    if (pCfg->timing_inv < 0) pCfg->timing_inv = 0;
    if (pCfg->timing_inv > 24 * 60) pCfg->timing_inv = 24 * 60;
    pCfg->addr_size = json_get_int(pjsGroup, "asdu_addr_size", 1, &succeed);
    if (pCfg->addr_size < 1 || pCfg->addr_size > 2) pCfg->addr_size = 1;
    pCfg->cause_size = json_get_int(pjsGroup, "cause_size", 1, &succeed);
    if (pCfg->cause_size < 1 || pCfg->cause_size > 2) pCfg->cause_size = 1;
    pCfg->debug_enabled = json_get_bool(pjsGroup, "debug_enabled", false, &succeed);

    cJSON_Delete(pJson);
    config_devices_jsonfile_delete(p);

    return 0;

__err_json_format:
    cJSON_Delete(pJson);

__err_free:
    config_devices_jsonfile_delete(p);
    return -1;
}
