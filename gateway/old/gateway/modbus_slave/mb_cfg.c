/*
* Copyright (c) 2019,珠海科技股份有限公司
* All rights reserved.
* 
* 文件名称：modbusconfig.c
* 文件标识：
* 描    述：
* 
* 当前版本：V1.0
* 作    者：wangcq
* 更改说明：
* 完成日期：20190530
* 运行环境：
操作系统: RT-Thread
硬件平台：STM32F030
固件库  ：V3.1.2
* 修改记录 
*  作者       时间         版本      修改描述
*  wangcq   2019.05.30       V1.00      创建
*  chenzy   2019.09.18       V1.01      更改寄存器段配置方式
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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "cJSON.h"
#include "cJSON_Utils.h"

#include "i2c.h"
#include "mb_cfg.h"
#include "mb_reg_remap.h"
#include "mb_protocol.h"
#include "db_device.h"
#include "db_struct.h"
#include "mb_datatype.h"


#define CFG_DEBUG_FILE  "/userdata/config/forward/fwchnl_64.json"



// 快速排序算法。基于寄存器号
/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static void _db_sort_swap(struct config_chnl_node_attr index[], int low, int high)
{
    struct config_chnl_node_attr tmp = index[low];

    index[low] = index[high];
    index[high] = tmp;
}

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int _db_sort_partition(struct config_chnl_node_attr index[], int low, int high)
{
    struct config_chnl_node_attr point = index[low];

    while(low < high)
    {
        while(low < high && index[high].reg_no >= point.reg_no)//控制high指针比较并左移
        {
            high--;
        }  
        _db_sort_swap(index, low, high);

        while(low < high && index[low].reg_no <= point.reg_no)//控制low指针比较并右移
        {
            low++;
        }  
        _db_sort_swap(index, low, high);
    }
    return low;//返回基准点位置
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static void _db_do_sort(struct config_chnl_node_attr index[], int low, int high)
{
    int point;

    if(low < high)
    {
        point = _db_sort_partition(index, low, high);//计算基准点
        _db_do_sort(index, low, point-1);  //对基准点的左边进行排序
        _db_do_sort(index, point+1, high);//对基准点的右边进行排序
    }
    
}

// END OF 快速排序算法。基于寄存器号

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int mb_get_nodeattr_by_regno(struct config_chnl_node_attr index[], uint16_t reg_no, int low, int high)
{
    int mid ;

    while(low <= high)
    {
        mid = (low + high) / 2;
        if(index[mid].reg_no < reg_no)
        {
            low = mid + 1;
        }
        else if(index[mid].reg_no > reg_no)
        {
            high = mid - 1;
        }
        else 
        {
            return mid;
        }
    }

    return -1;
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static char *mb_cfg_jsonfile_alloc(char *file, int *size)
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
static void mb_cfg_jsonfile_delete(char *p)
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
static char *mb_cfg_jsonfile_read(char *file, char *p, int size)
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

    /* rs485_addr . add by chenzy 20241226 */
    pItem = cJSON_GetObjectItem(pjsGroup, "rs485_addr");
    if(pItem == NULL)
        pCfg->rs485_addr = 1;
    else 
        pCfg->rs485_addr = pItem->valueint;

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
static int config_devices_json_group_node_read(struct config_chnl_working_attr *pCfg, cJSON *pjsGroup, int start)
{
    int         size;
    int         i, pos;
    cJSON       *pObject, *pArrayItem, *pItem;

    /* white_list */
    pArrayItem = cJSON_GetObjectItem(pjsGroup, "node_attr");
    if(pArrayItem == NULL)
        return -1;

    size = cJSON_GetArraySize(pArrayItem);
    size = (start + size) < MB_FW_POINT_MAX ? size : MB_FW_POINT_MAX - start;
    for(i=0; i<size; i++)
    {
        pos = start + i;
        pObject = cJSON_GetArrayItem(pArrayItem, i); 

        /* reg_no */
        pItem = cJSON_GetObjectItem(pObject, "reg_no");
        if(pItem == NULL)
            return -1;
        pCfg->node_attr[pos].reg_no = pItem->valueint;

        /* func_code */
        pItem = cJSON_GetObjectItem(pObject, "func_code");
        if(pItem == NULL)
            return -1;
        pCfg->node_attr[pos].func_code = pItem->valueint;

        /* datatype */
        pItem = cJSON_GetObjectItem(pObject, "datatype");
        if(pItem == NULL)
            return -1;
        pCfg->node_attr[pos].datatype = pItem->valueint;

        /* datattr */
        pItem = cJSON_GetObjectItem(pObject, "datattr");
        if(pItem == NULL)
            return -1;
        pCfg->node_attr[pos].datattr = pItem->valueint;

        /* db_link */
        pItem = cJSON_GetObjectItem(pObject, "db_link");
        if(pItem == NULL)
            return -1;
        sscanf(pItem->valuestring, "%X", &pCfg->node_attr[pos].db_link);

        /* kfactor */
        pItem = cJSON_GetObjectItem(pObject, "kfactor");
        if(pItem == NULL)
            return -1;
        pCfg->node_attr[pos].kfactor = pItem->valuedouble;

        /* bfactor */
        pItem = cJSON_GetObjectItem(pObject, "bfactor");
        if(pItem == NULL)
            return -1;
        pCfg->node_attr[pos].bfactor = pItem->valuedouble;

    }

    pCfg->node_num = pCfg->node_num + size;

    if(pCfg->debug)
    {
        printf("node:--------------------------------\n");
            printf("        reg_no      db_link     datatype\n");
        for(i=0; i<size; i++)
        {
            printf("%04d => %5d         %08X       %d\n", start+i, pCfg->node_attr[start+i].reg_no, pCfg->node_attr[start+i].db_link, pCfg->node_attr[start+i].datatype);
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
static int mb_cfg_chnl_load(struct config_chnl_working_attr *pCfg, char *file)
{
    char    *p;
    int     size;
    int     ret;
    cJSON   *pJson, *pjsClass, *pjsGroup;


    /* prepare the buffer for json */
    p = mb_cfg_jsonfile_alloc(file, &size);
    if(p == NULL)
        return -1;
 
    p = mb_cfg_jsonfile_read(file, p, size);
    if(p == NULL)
        goto __err_free;
 
    pJson = cJSON_Parse(p);  
    if(!pJson)
        goto __err_free;
 
    /*
        {
    "Name":"fw",
    "version":"V1.0.0",
    "info" : {
        "help":""
    },
    "attr" : {
        "checked" : 1,
        "common" : {
            "id": 1,
            "checked": 1,
            "mode" : "poll",
            "link": "tcp server",
            "dev": "127.0.0.1:502",
            "serial":{
                "baud": 9600,
                "databit":8,
                "parity":"N",
                "stopbit":1
            },

            "protocol" :"modbusTCP",
            "register_type": "null",
            "register": ""
        },
        "ext":{
            "info":"用户匹配不同规约的特殊参数。modbus暂无，ext就只有info字段和白名单",
            "white_list":{
                "master_ip_1":"0.0.0.0",
                "master_ip_2":"0.0.0.0"
            }
        },
        "node":{
            "node_num"  : 9,	
            "node_attr" : [
                {"id":0, 	"datatype":1,		"db_link":"01010000",		"datattr":0,	"func_code":3, 	"reg_no":0,			"catagory":170,		"kfactor":1,		"bfactor":0,	"name":"Commstatus",	"unit":""},                         
                {"id":1, 	"datatype":9, 		"db_link":"01010001", 		"datattr":0, 	"func_code":3, 	"reg_no":2, 	    "catagory":11, 		"kfactor":0.01, 	"bfactor":0,	"name":"Ua", 			"unit":"V"}
           ]
        }
        
    }
}
    */
    pjsClass = cJSON_GetObjectItem(pJson, "attr"); 
    if(!pjsClass)
        goto __err_json_format;

    /* get common parameters from mb_regsetting.json */
    pjsGroup=cJSON_GetObjectItem(pjsClass, "common");
    if(!pjsGroup)
        goto __err_json_format;

    ret = config_devices_json_group_common_read(pCfg, pjsGroup);
    if(ret < 0)
        goto __err_json_format;

    /* get ext parameters from mb_regsetting.json */
    pjsGroup=cJSON_GetObjectItem(pjsClass, "ext");
    if(!pjsGroup)
        goto __err_json_format;

    ret = config_devices_json_group_ext_read(pCfg, pjsGroup);
    if(ret < 0)
        goto __err_json_format;

    /* get node parameters from mb_regsetting.json */
    pjsGroup=cJSON_GetObjectItem(pjsClass, "node");
    if(!pjsGroup)
        goto __err_json_format;

    ret = config_devices_json_group_node_read(pCfg, pjsGroup, 0);
    if(ret < 0)
        goto __err_json_format;

    cJSON_Delete(pJson);
    mb_cfg_jsonfile_delete(p);

    return 0;

__err_json_format:
    cJSON_Delete(pJson);

__err_free:
    mb_cfg_jsonfile_delete(p);

    return -1;
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int mb_cfg_chnl_load_node_only(struct config_chnl_working_attr *pCfg, char *file)
{
    char    *p;
    int     size;
    int     ret;
    cJSON   *pJson, *pjsClass, *pjsGroup;


    /* prepare the buffer for json */
    p = mb_cfg_jsonfile_alloc(file, &size);
    if(p == NULL)
        return -1;
 
    p = mb_cfg_jsonfile_read(file, p, size);
    if(p == NULL)
        goto __err_free;
 
    pJson = cJSON_Parse(p);  
    if(!pJson)
        goto __err_free;
 
    /*
        {
    "Name":"fw",
    "version":"V1.0.0",
    "info" : {
        "help":""
    },
    "attr" : {
        "checked" : 1,
        "common" : {
            "id": 1,
            "checked": 1,
            "mode" : "poll",
            "link": "tcp server",
            "dev": "127.0.0.1:502",
            "serial":{
                "baud": 9600,
                "databit":8,
                "parity":"N",
                "stopbit":1
            },

            "protocol" :"modbusTCP",
            "register_type": "null",
            "register": ""
        },
        "ext":{
            "info":"用户匹配不同规约的特殊参数。modbus暂无，ext就只有info字段和白名单",
            "white_list":{
                "master_ip_1":"0.0.0.0",
                "master_ip_2":"0.0.0.0"
            }
        },
        "node":{
            "node_num"  : 9,	
            "node_attr" : [
                {"id":0, 	"datatype":1,		"db_link":"01010000",		"datattr":0,	"func_code":3, 	"reg_no":0,			"catagory":170,		"kfactor":1,		"bfactor":0,	"name":"Commstatus",	"unit":""},                         
                {"id":1, 	"datatype":9, 		"db_link":"01010001", 		"datattr":0, 	"func_code":3, 	"reg_no":2, 	    "catagory":11, 		"kfactor":0.01, 	"bfactor":0,	"name":"Ua", 			"unit":"V"}
           ]
        }
        
    }
}
    */
    pjsClass = cJSON_GetObjectItem(pJson, "attr"); 
    if(!pjsClass)
        goto __err_json_format;

    /* get node parameters from mb_regsetting.json */
    pjsGroup=cJSON_GetObjectItem(pjsClass, "node");
    if(!pjsGroup)
        goto __err_json_format;

    ret = config_devices_json_group_node_read(pCfg, pjsGroup, pCfg->node_num);
    if(ret < 0)
        goto __err_json_format;

    cJSON_Delete(pJson);
    mb_cfg_jsonfile_delete(p);

    return 0;

__err_json_format:
    cJSON_Delete(pJson);

__err_free:
    mb_cfg_jsonfile_delete(p);

    return -1;
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int mb_cfg_project_load(struct config_chnl_working_attr *pCfg, char *file)
{
    char    *p;
    int     size;
    int     ret;
    cJSON   *pJson, *pjsClass, *pjsGroup;


    /* prepare the buffer for json */
    p = mb_cfg_jsonfile_alloc(file, &size);
    if(p == NULL)
        return -1;
 
    p = mb_cfg_jsonfile_read(file, p, size);
    if(p == NULL)
        goto __err_free;
 
    pJson = cJSON_Parse(p);  
    if(!pJson)
        goto __err_free;
 
    /*
        {
            "Name": null,
            "info": {
                "help": null
            },
            "attr": {
                "station": "project",
                "rooms": "room",
                "gatewayname": "GATEWAY",
                "owner": "test",
                "company": "",
                "companyurl": "",
                "logo": ""
            }
        }
    */
    pjsClass = cJSON_GetObjectItem(pJson, "attr"); 
    if(!pjsClass)
        goto __err_json_format;

    /* get common parameters from mb_regsetting.json */
    pjsGroup=cJSON_GetObjectItem(pjsClass, "gatewayname");
    if(!pjsGroup)
        goto __err_json_format;
    
    snprintf(pCfg->name, sizeof(pCfg->name), "%s", pjsGroup->valuestring);

    cJSON_Delete(pJson);
    mb_cfg_jsonfile_delete(p);

    return 0;

__err_json_format:
    cJSON_Delete(pJson);

__err_free:
    mb_cfg_jsonfile_delete(p);

    return -1;
}

/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_network_file_read(char *file, char *strval, uint8_t len)
{
    int fd, n;

    fd = open(file, O_RDONLY);
    if(fd < 0)
        return -1;

    n = read(fd, strval, len);

    close(fd);

    return n;

}


/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
int mb_config_static_info(struct config_chnl_working_attr *pCfg_chnlattr, char *pbuf, uint16_t len, uint8_t *mac, uint8_t mac_len)
{
    char ip[16], netmask[16], gateway[16];
    char strmac[32];
    int  imac[6];

    mb_network_file_read("/var/run/network/mac", strmac, sizeof(strmac));
    sscanf(strmac, "%X:%X:%X:%X:%X:%X", &imac[0], &imac[1], &imac[2], &imac[3], &imac[4], &imac[5]);
    mac[0] = imac[0];
    mac[1] = imac[1];
    mac[2] = imac[2];
    mac[3] = imac[3];
    mac[4] = imac[4];
    mac[5] = imac[5];

    /*ip*/
    mb_network_file_read("/var/run/network/ip", ip, sizeof(ip));
    mb_network_file_read("/var/run/network/netmask", netmask, sizeof(netmask));
    mb_network_file_read("/var/run/network/gateway", gateway, sizeof(gateway));

    return snprintf(pbuf, len, "{\"devtype\":\"TASO1000\","
                          "\"IP\":\"%s\","
                          "\"Netmask\":\"%s\","
                          "\"Gateway\":\"%s\","
                          "\"Mac\":\"%s\","
                          "\"Name\":\"%s\","
                          "\"SN\":\"%s\""
                        "}",
                        ip,
                        netmask,
                        gateway,
                        strmac,
                        pCfg_chnlattr->name,
                        pCfg_chnlattr->sn);

}

/*********************************************************************************************************
** Function name:       mb_config_init
** Descriptions:        modbus rtu 协议配置初始化,主要是从EEPROM中加载配置，并判断配置是否正确
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
int  mb_config_init(struct config_chnl_working_attr *pCfg_chnlattr, char *cfg_file)
{
    int ret;
    
    ret = mb_cfg_chnl_load(pCfg_chnlattr, cfg_file);
    if(ret < 0)
    {
        printf("can not load %s success\n", cfg_file);
        return -1;
    }

    /*
        兼容旧的从502端口做管理通道的方式，
        新版配置软件增加了调试通道（即配置南向后，点击保存会自动生成全部测点的转发点表）

        实现机制是， 在原 主502 端口的modbus管理通道的 30000 地址开始放置 上面生成的全部测点用于调试。 预留0-29999 寄存器号给 主modbusTCP 502
        用于动态加载点
    
    */

    /* 
        不使用502 30000 地址作为调试通道。 
        因为应用软件的调试已经改为按每个转发modbus通道各自读取了
    
    */
 //   if(pCfg_chnlattr->id == 1)
 //       mb_cfg_chnl_load_node_only(pCfg_chnlattr, CFG_DEBUG_FILE);

    /* 转发表按寄存器号排序，用于加快查找效率 */
    if(pCfg_chnlattr->node_num)
        _db_do_sort(&pCfg_chnlattr->node_attr[0], 0, pCfg_chnlattr->node_num - 1);

    return 0;

}




/*********************************************************************************************************
** Function name:       
** Descriptions:        modbus rtu
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void mb_config_destory(void)
{

}








