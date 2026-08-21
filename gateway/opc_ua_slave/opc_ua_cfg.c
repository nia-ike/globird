/*
*  Copyright (c) 2019,广州碳索智控科技有限公司
*  All rights reserved.
* 
*  文件名称：
*  文件标识：
*  描    述：
* 
*  当前版本：V1.0
*  作    者：chenzy
*  运行环境：
*  操作系统: linux
*  硬件平台：
*  固件库  ：
*  修改记录 
*  作者       时间         版本      修改描述
*  chenzy          V1.00      
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <unistd.h> 
#include <sys/stat.h>  //stat
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>

#include "cJSON.h"
#include "cJSON_Utils.h"
#include "db_struct.h"
#include "opc_ua_cfg.h"


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static struct db_device_list_t *db_device_create(void)
{
    return malloc(sizeof(struct db_device_list_t));
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static void db_device_destory(struct db_device_list_t *pdev_list)
{
    free(pdev_list);
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static struct db_node_list_t *db_node_create(void)
{
    return malloc(sizeof(struct db_node_list_t));
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static void db_node_destory(struct db_node_list_t *pnode_list)
{
    free(pnode_list);
}




/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static char *opc_ua_cfg_jsonfile_alloc(char *file, int *size)
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
static void opc_ua_cfg_jsonfile_delete(char *p)
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
static char *opc_ua_cfg_jsonfile_read(char *file, char *p, int size)
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
static int config_devices_json_group_common_read(struct config_chnl_working_attr *pCfg, cJSON *pjsGroup)
{
    cJSON       *pObject, *pItem;

    /* id */
    pItem = cJSON_GetObjectItem(pjsGroup, "id");
    if(pItem == NULL)
        return -1;
    pCfg->id = pItem->valueint;

//    /* checked */
//    pItem = cJSON_GetObjectItem(pjsGroup, "checked");
//    if(pItem == NULL)
//        return -1;
//    pCfg->checked = pItem->valueint;
    pCfg->checked = 1;

    /* dev */
    pItem = cJSON_GetObjectItem(pjsGroup, "dev");
    if(pItem == NULL)
        return -1;
    snprintf(pCfg->dev, sizeof(pCfg->dev), "%s", pItem->valuestring);

    /* mode */
    pItem = cJSON_GetObjectItem(pjsGroup, "mode");
    if(pItem == NULL)
        return -1;
    snprintf(pCfg->mode, sizeof(pCfg->mode), "%s", pItem->valuestring);

    /* link */
    pItem = cJSON_GetObjectItem(pjsGroup, "link");
    if(pItem == NULL)
        return -1;
    snprintf(pCfg->link, sizeof(pCfg->link), "%s", pItem->valuestring);

    /* protocol */
    pItem = cJSON_GetObjectItem(pjsGroup, "protocol");
    if(pItem == NULL)
        return -1;
    snprintf(pCfg->protocol, sizeof(pCfg->protocol), "%s", pItem->valuestring);

    /* protocol */
    pItem = cJSON_GetObjectItem(pjsGroup, "local_port");
    if(pItem == NULL)
        return -1;
    pCfg->opc_ua_comm_ctx.port = pItem->valueint;

    /* register_type */
    pItem = cJSON_GetObjectItem(pjsGroup, "register_type");
    if(pItem == NULL)
        return -1;
    snprintf(pCfg->register_type, sizeof(pCfg->register_type), "%s", pItem->valuestring);

    /* register_context */
    pItem = cJSON_GetObjectItem(pjsGroup, "register_context");
    if(pItem == NULL)
        return -1;
    snprintf(pCfg->register_context, sizeof(pCfg->register_context), "%s", pItem->valuestring);

    /* serial */
    pObject = cJSON_GetObjectItem(pjsGroup, "serial");
    if(pObject == NULL)
        return -1;
    
    /* baud */
    pItem = cJSON_GetObjectItem(pObject, "baud");
    if(pItem == NULL)
        return -1;
    pCfg->serial_attr.baud = pItem->valueint;

    /* datasize */
    pItem = cJSON_GetObjectItem(pObject, "databit");
    if(pItem == NULL)
        return -1;
    pCfg->serial_attr.datasize = pItem->valueint;

    /* parity */
    pItem = cJSON_GetObjectItem(pObject, "parity");
    if(pItem == NULL)
        return -1;
    pCfg->serial_attr.parity = pItem->valuestring[0];

    /* stopbit */
    pItem = cJSON_GetObjectItem(pObject, "stopbit");
    if(pItem == NULL)
        return -1;
    pCfg->serial_attr.stopbit = pItem->valueint;

    if(pCfg->debug)
    {
        printf("common--------------------------------\n");
        printf("id:                 %d\n", pCfg->id);
        printf("dev:                %s\n", pCfg->dev);
        printf("link:               %s\n", pCfg->link);
        printf("mode:               %s\n", pCfg->mode);
        printf("protocol:           %s\n", pCfg->protocol);
        printf("port:               %d\n", pCfg->opc_ua_comm_ctx.port);
        printf("register_type:      %s\n", pCfg->register_type);
        printf("register_context:   %s\n", pCfg->register_context);
        printf("baud:               %d\n", pCfg->serial_attr.baud);
        printf("parity:             %c\n", pCfg->serial_attr.parity);

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
static int config_devices_json_group_ext_read(struct config_chnl_working_attr *pCfg, cJSON *pjsGroup)
{
    cJSON       *pObject, *pItem;

    /* white_list */
    pObject = cJSON_GetObjectItem(pjsGroup, "white_list");
    if(pObject == NULL)
        return -1;
    
    /* master_ip_1 */
    pItem = cJSON_GetObjectItem(pObject, "master_ip_1");
    if(pItem == NULL)
        return -1;
    snprintf(pCfg->ext_attr.whitelist1, sizeof(pCfg->ext_attr.whitelist1), "%s", pItem->valuestring);
    
    /* master_ip_2 */
    pItem = cJSON_GetObjectItem(pObject, "master_ip_2");
    if(pItem == NULL)
        return -1;
    snprintf(pCfg->ext_attr.whitelist2, sizeof(pCfg->ext_attr.whitelist2), "%s", pItem->valuestring);

    if(pCfg->debug)
    {
        printf("ext--------------------------------\n");
        printf("whitelist1:%s\n", pCfg->ext_attr.whitelist1);
        printf("whitelist2:%s\n", pCfg->ext_attr.whitelist2);
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
static struct db_device_list_t *config_devices_list_find(struct db_device_list_t *pdev_list_head, union db_node_index_id *id)
{
    struct db_device_list_t *pdev_attr;

    slist_for_each_entry(pdev_attr, &pdev_list_head->list, list)
    {
        if(pdev_attr->id.mix == id->mix)
            return pdev_attr;
    }

    return NULL;
}

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int config_devices_json_group_node_read(struct config_chnl_working_attr *pCfg, struct db_device_list_t *pdev_list_head, cJSON *pjsGroup)
{
    int         size;
    int         i;
    union db_node_index_id id;
    cJSON       *pObject, *pArrayItem, *pItem;
    struct db_node_list_t *pNode_attr;
    struct db_device_list_t *pDev_attr;

    /* white_list */
    pArrayItem = cJSON_GetObjectItem(pjsGroup, "node_attr");
    if(pArrayItem == NULL)
        return -1;

    size = cJSON_GetArraySize(pArrayItem); 

    for(i=0; i<size; i++)
    {
        pNode_attr = db_node_create();
        if(pNode_attr == NULL)
        {
            printf("_config_devices_node_create == NULL\n");
            goto __err;
        }
            
        pObject = cJSON_GetArrayItem(pArrayItem, i); 

        /* datattr */
        pItem = cJSON_GetObjectItem(pObject, "datattr");
        if(pItem == NULL)
            return -1;
        pNode_attr->datattr = pItem->valueint;

        /* db_link */
        pItem = cJSON_GetObjectItem(pObject, "db_link");
        if(pItem == NULL)
            return -1;
        sscanf(pItem->valuestring, "%X", &pNode_attr->db_link);

        /* kfactor */
        pItem = cJSON_GetObjectItem(pObject, "kfactor");
        if(pItem == NULL)
            return -1;
        pNode_attr->kfactor = pItem->valuedouble;

        /* bfactor */
        pItem = cJSON_GetObjectItem(pObject, "name");
        if(pItem == NULL)
            return -1;

        snprintf(pNode_attr->name, sizeof(pNode_attr->name), "%s", pItem->valuestring);

        /*
            双层链表方式进行测点分类：

            devlist :  dev -> dev -> dev -> 
                        |
                        |  
                        -> node -> node -> node 
            --------------------------------------
            devlist 以 chnlno 和 devno作为匹配

         */
        id.split.ucnPort = DB_CONVENT_ID_2_CHNLNO(pNode_attr->db_link);
        id.split.ucnDev  = DB_CONVENT_ID_2_DEVNO(pNode_attr->db_link);

        pDev_attr = config_devices_list_find(pdev_list_head, &id);
        if(pDev_attr == NULL)
        {
            // 新设备，创建新设备链表节点
            pDev_attr = db_device_create();
            if(pDev_attr == NULL)
                continue;

            pDev_attr->id = id;
            
            // 新建设备， 该设备的测点链表需要初始化
            slist_init(&pDev_attr->node_list.list);
            // 挂载该设备节点到  设备链表上
            slist_append(&pdev_list_head->list, &pDev_attr->list);
            
        }

        // 挂载该测点到 该设备的测点链表上
        slist_append(&pDev_attr->node_list.list, &pNode_attr->list);
    }

__err:
    if(pCfg->debug)
    {
        slist_for_each_entry(pDev_attr, &pdev_list_head->list, list)
        {
            printf("Dev id: %04X\n", pDev_attr->id.mix);
            slist_for_each_entry(pNode_attr, &pDev_attr->node_list.list, list)
            {
                printf("Node id: %08X\n", pNode_attr->db_link);
            }
        }
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
static int opc_ua_cfg_chnl_load(struct config_chnl_working_attr *pCfg, char *file)
{
    char    *p;
    int     size;
    int     ret;
    cJSON   *pJson, *pjsClass, *pjsGroup, *pjsnode, *pjsext, *pjsitem;

    /* prepare the buffer for json */
    p = opc_ua_cfg_jsonfile_alloc(file, &size);
    if(p == NULL)
        return -1;

    p = opc_ua_cfg_jsonfile_read(file, p, size);
    if(p == NULL)
        goto __err_free;
 
    pJson = cJSON_Parse(p);  
    if(!pJson)
        goto __err_free;
 
    /*
    {
    "Name": "fw",
    "version": "V1.0.0",
    "info": {
        "help": ""
    },
    "attr": {
        "checked": 1,
        "common": {
            "id": 2,
            "mode": "poll",
            "link": "",
            "dev": "",
            "serial": {
                "baud": 9600,
                "databit": 8,
                "parity": "N",
                "stopbit": 1
            },
            "protocol": "opc_ua",
            "register_send_type": "none",
            "register_send_period": 60,
            "register_type": "sn",
            "register_context": "",
            "samplechnl_id": 1,
            "local_port": 0,
            "rs485_addr": 1
        },
        "ext": {
            "info": "用户匹配不同规约的特殊参数。modbus暂无，ext就只有info字段和白名单",
            "white_list": {
                "master_ip_1": null,
                "master_ip_2": null
            }
        },
        "node": {
            "node_num": 32,
            "node_attr": [
                {
                    "db_link": "01010100",
                    "datattr": 1,
                    "catagory": 0,
                    "name": "CommStatus",
                    "unit": "",
                    "id": 0,
                    "alias": "",
                    "kfactor": 1
                },
                {
                    "db_link": "01010101",
                    "datattr": 1,
                    "catagory": 0,
                    "name": "S1-JL1TD",
                    "unit": "",
                    "id": 1,
                    "alias": "",
                    "kfactor": 1
                }
            ]
        },
        "node_ex": null,
        "extend_param": null
    }
}
    */

    pjsClass = cJSON_GetObjectItem(pJson, "attr"); 
    if(!pjsClass)
        goto __err_json_format;

    /* get common parameters from opc_ua_regsetting.json */
    pjsGroup=cJSON_GetObjectItem(pjsClass, "common");
    if(!pjsGroup)
        goto __err_json_format;

    ret = config_devices_json_group_common_read(pCfg, pjsGroup);
    if(ret < 0)
        goto __err_json_format;

    /* get ext parameters from opc_ua_regsetting.json */
    pjsGroup=cJSON_GetObjectItem(pjsClass, "ext");
    if(!pjsGroup)
        goto __err_json_format;

    // context
        ret = config_devices_json_group_ext_read(pCfg, pjsGroup);
        if(ret < 0)
            goto __err_json_format;

    /* get node parameters from opc_ua_regsetting.json */
    pjsGroup=cJSON_GetObjectItem(pjsClass, "node_ex");
    if(!pjsGroup)
        goto __err_json_format;

    pjsext=cJSON_GetObjectItem(pjsClass, "extend_param");
    if(!pjsext)
        goto __err_json_format;

    /* get node parameters from opc_ua_regsetting.json */
    pjsGroup=cJSON_GetObjectItem(pjsClass, "node");
    if(!pjsGroup)
        goto __err_json_format;

    // context
        ret = config_devices_json_group_node_read(pCfg, &pCfg->device_list, pjsGroup);
        if(ret < 0)
            goto __err_json_format;     

    cJSON_Delete(pJson);
    opc_ua_cfg_jsonfile_delete(p);

    return 0;

__err_json_format:
    cJSON_Delete(pJson);

__err_free:
    opc_ua_cfg_jsonfile_delete(p);

    return -1;
}


/*********************************************************************************************************
** Function name:       opc_ua_config_init
** Descriptions:        modbus rtu 协议配置初始化,主要是从EEPROM中加载配置，并判断配置是否正确
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
int opc_ua_config_init(struct config_chnl_working_attr *pCfg_chnlattr, char *cfg_file)
{
    int ret;
    
    ret = opc_ua_cfg_chnl_load(pCfg_chnlattr, cfg_file);
    if(ret < 0)
    {
        printf("can not load %s success\n", cfg_file);
        return -1;
    }

    /* 转发表按寄存器号排序，用于加快查找效率 */
//    if(pCfg_chnlattr->node_num)
//        _db_do_sort(&pCfg_chnlattr->node_attr[0], 0, pCfg_chnlattr->node_num - 1);

    return 0;
}




/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void opc_ua_config_destory(void)
{

}


