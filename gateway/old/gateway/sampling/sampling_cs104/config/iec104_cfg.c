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

#include "iec104_cfg.h"

/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:             
** output parameters:   NONE
** Returned value:      
*********************************************************************************************************/
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

/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:             
** output parameters:   NONE
** Returned value:      
*********************************************************************************************************/
static void config_devices_jsonfile_delete(char *p)
{
    if(p == NULL)
        return;
    
    free(p);
}

/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:             
** output parameters:   NONE
** Returned value:      
*********************************************************************************************************/
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

/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:             
** output parameters:   NONE
** Returned value:      
*********************************************************************************************************/
int32_t iec104_cfg_node_attr_size(struct config_devices_attr *pDevAttr)
{// 获取该设备的点属性表数量
    return pDevAttr->node_num;
}

/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:             
** output parameters:   NONE
** Returned value:      
*********************************************************************************************************/
struct config_node_attr *iec104_cfg_node_attr(struct config_devices_attr *pDevAttr, uint8_t id)
{// 获取该设备的点属性表
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
int32_t iec104_cfg_control_attr_size(struct config_devices_attr *pDevAttr)
{// 获取该设备的控制属性表数量
    return pDevAttr->control_num;
}

/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:             
** output parameters:   NONE
** Returned value:      
*********************************************************************************************************/
struct config_control_attr *iec104_cfg_control_attr(struct config_devices_attr *pDevAttr, uint8_t id)
{// 获取该设备的控制属性表（通过 id 查找）
    int i;
    
    for(i = 0; i < pDevAttr->control_num; i++)
    {
        if(pDevAttr->control_attr[i].id == id)
            return &pDevAttr->control_attr[i];
    }
    
    return NULL;
}

/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:             
** output parameters:   NONE
** Returned value:      
*********************************************************************************************************/
static int config_devices_json_group_node_read(struct config_devices_attr *pCfg, int num, cJSON *pArrayItem)
{// 加载测点
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

        pItem = cJSON_GetObjectItem(pObject, "datattr");
        if(pItem == NULL)
            return -1;
		else
			pCfg->node_attr[i].datattr = pItem->valueint;

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

        pItem = cJSON_GetObjectItem(pObject, "inf_addr");
        if(pItem == NULL)
            return -1;
		else
			pCfg->node_attr[i].inf_addr = pItem->valuedouble;    

    }
    return 0;
}


/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:             
** output parameters:   NONE
** Returned value:      
*********************************************************************************************************/
static int config_devices_json_group_control_read(struct config_devices_attr *pCfg, int num, cJSON *pArrayItem)
{// 加载遥控点
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
			pCfg->control_attr[i].id = pItem->valuedouble;

        pItem = cJSON_GetObjectItem(pObject, "inf_addr");
        if(pItem == NULL)
            return -1;
		else
			pCfg->control_attr[i].inf_addr = pItem->valuedouble;

        pItem = cJSON_GetObjectItem(pObject, "double_ctrl");
        if(pItem == NULL)
            pCfg->control_attr[i].double_ctrl = false;  // 默认单点遥控
		else
			pCfg->control_attr[i].double_ctrl = pItem->valuedouble;

        // kfactor 可选，仅用于 AO（浮点数遥调）
        pItem = cJSON_GetObjectItem(pObject, "kfactor");
        if(pItem == NULL)
            pCfg->control_attr[i].kfactor = 1.0;  // 默认值为1
		else
			pCfg->control_attr[i].kfactor = pItem->valuedouble;

    }

    return 0;
}


/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:             
** output parameters:   NONE
** Returned value:      
*********************************************************************************************************/
int iec104_cfg_load(struct config_devices_attr *pCfg, char *file)
{// 加载配置
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

    /* 
        分配内存 
        新版本使用测点使用动态申请节点属性表
        旧版本使用静态数组，存在浪费内存问题
        从而导致最大点数不敢设置多
    
    */
    pCfg->node_num = (pCfg->node_num > CFG_MAX_POINT) ? CFG_MAX_POINT : pCfg->node_num;
    pCfg->node_attr = malloc(sizeof(struct config_node_attr) * pCfg->node_num);
    if(pCfg->node_attr == NULL)
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

    pItem = cJSON_GetObjectItem(pjsGroup, "timing_cycle");
    if(!pItem)
        goto __err_json_format;
	else
		pCfg->timing_inv = pItem->valueint;

    if (pCfg->timing_inv < 0) 
        pCfg->timing_inv = 0;
    if (pCfg->timing_inv > 24 * 60) 
        pCfg->timing_inv = 24 * 60;

    pItem = cJSON_GetObjectItem(pjsGroup, "asdu_addr_size");
    if(!pItem)
        goto __err_json_format;
	else
		pCfg->addr_size = pItem->valueint;
    if (pCfg->addr_size < 1 || pCfg->addr_size > 2) 
        pCfg->addr_size = 1;

    pItem = cJSON_GetObjectItem(pjsGroup, "cause_size");
    if(!pItem)
        goto __err_json_format;
	else
		pCfg->cause_size = pItem->valueint;
    if (pCfg->cause_size < 1 || pCfg->cause_size > 2) 
        pCfg->cause_size = 1;

    pItem = cJSON_GetObjectItem(pjsGroup, "debug_enabled");
    if(!pItem)
        goto __err_json_format;
	else
		pCfg->debug_enabled = pItem->valueint;

    cJSON_Delete(pJson);
    config_devices_jsonfile_delete(p);

    return 0;

__err_json_format:
    cJSON_Delete(pJson);

__err_free:
    config_devices_jsonfile_delete(p);
    return -1;
}
