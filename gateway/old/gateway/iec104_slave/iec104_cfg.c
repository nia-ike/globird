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

#include "iec104_cfg.h"

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
int iec104_get_nodeattr_by_regno(struct config_chnl_node_attr index[], uint16_t reg_no, int low, int high)
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
static char *iec104_cfg_jsonfile_alloc(char *file, int *size)
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
static void iec104_cfg_jsonfile_delete(char *p)
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
static char *iec104_cfg_jsonfile_read(char *file, char *p, int size)
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
static struct config_chnl_node_attr *_config_devices_node_create(void)
{
    return malloc(sizeof(struct config_chnl_node_attr));
}

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static void _config_devices_node_delete(struct config_chnl_node_attr *p)
{
    return free(p);
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int config_devices_json_group_node_read(struct config_chnl_working_attr *pCfg, struct config_node_group *pNode_group, cJSON *pjsGroup, char *name)
{
    int         size;
    int         i;
    cJSON       *pObject, *pArrayItem, *pItem;
    struct config_chnl_node_attr *pNode_attr;

    /* white_list */
    pArrayItem = cJSON_GetObjectItem(pjsGroup, "node_attr");
    if(pArrayItem == NULL)
        return -1;

    size = cJSON_GetArraySize(pArrayItem); 

    for(i=0; i<size; i++)
    {
        pNode_attr = _config_devices_node_create();
        if(pNode_attr == NULL)
        {
            printf("_config_devices_node_create == NULL\n");
            goto __err;
        }
            
        pObject = cJSON_GetArrayItem(pArrayItem, i); 

        /* reg_no */
        pItem = cJSON_GetObjectItem(pObject, "reg_no");
        if(pItem == NULL)
            return -1;
        pNode_attr->reg_no = pItem->valueint;

        /* func_code */
        pItem = cJSON_GetObjectItem(pObject, "func_code");
        if(pItem == NULL)
            return -1;
        pNode_attr->func_code = pItem->valueint;

        /* datatype */
        pItem = cJSON_GetObjectItem(pObject, "datatype");
        if(pItem == NULL)
            return -1;
        pNode_attr->datatype = pItem->valueint;

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
        pItem = cJSON_GetObjectItem(pObject, "bfactor");
        if(pItem == NULL)
            return -1;
        pNode_attr->bfactor = pItem->valuedouble;

        slist_append(&pNode_group->node_attr.list, &pNode_attr->list);
    }

__err:
    pNode_group->node_num = size;

    if(pCfg->debug)
    {
        printf("node:  %s --------------------------------\n", name);
            printf("        reg_no      db_link     datatype    funcode\n");

        slist_for_each_entry(pNode_attr, &pNode_group->node_attr.list, list)
        {
            printf("      %5d         %08X       %d           %d\n", pNode_attr->reg_no, pNode_attr->db_link, pNode_attr->datatype, pNode_attr->func_code);
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
static int iec104_cfg_chnl_load(struct config_chnl_working_attr *pCfg, char *file)
{
    char    *p;
    int     size;
    int     ret;
    cJSON   *pJson, *pjsClass, *pjsGroup, *pjsnode, *pjsext, *pjsitem;

    /* prepare the buffer for json */
    p = iec104_cfg_jsonfile_alloc(file, &size);
    if(p == NULL)
        return -1;

    p = iec104_cfg_jsonfile_read(file, p, size);
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
            "link": "tcp server",
            "dev": "0.0.0.0:2404",
            "serial": {
                "baud": 9600,
                "databit": 8,
                "parity": "N",
                "stopbit": 1
            },
            "protocol": "iec104",
            "register_send_type": "none",
            "register_send_period": 60,
            "register_type": "sn",
            "register_context": "",
            "samplechnl_id": 0,
            "local_port": 0
        },
        "ext": {
            "info": "用户匹配不同规约的特殊参数。modbus暂无，ext就只有info字段和白名单",
            "white_list": {
                "master_ip_1": null,
                "master_ip_2": null
            }
        },
        "node": {
            "node_num": 0,
            "node_attr": []
        },
        "node_ex": {
            "MMEA": {
                "node_num": 0,
                "node_attr": []
            },
            "MMEB": {
                "node_num": 0,
                "node_attr": []
            },
            "MMEC": {
                "node_num": 103,
                "node_attr": [
                    {
                        "db_link": "02010001",
                        "datattr": 0,
                        "catagory": 11,
                        "name": "相电压",
                        "unit": "V",
                        "reg_no": 0,
                        "func_code": 13,
                        "kfactor": 1,
                        "bfactor": 0,
                        "datatype": 13,
                        "id": 0
                    }
                ]
            },
            "CCI": {
                "node_num": 0,
                "node_attr": []
            },
            "MSP": {
                "node_num": 0,
                "node_attr": []
            },
            "MDP": {
                "node_num": 0,
                "node_attr": []
            },
            "CSC": {
                "node_num": 0,
                "node_attr": []
            },
            "CDC": {
                "node_num": 0,
                "node_attr": []
            },
            "CRC": {
                "node_num": 0,
                "node_attr": []
            },
            "CSEA": {
                "node_num": 0,
                "node_attr": []
            },
            "CSEB": {
                "node_num": 0,
                "node_attr": []
            },
            "CSEC": {
                "node_num": 0,
                "node_attr": []
            }
        },
        "extend_param": {
            "HOST": 1,
            "ADDR": 1,
            "ATTR": "0x0",
            "T1": 15,
            "T2": 10,
            "T3": 20,
            "K": 12,
            "W": 8,
            "INTERVAL": 5,
            "N": 32768,
            "SEL_TO": 30,
            "RS": 0,
            "MMEA_ADDR": "0x0",
            "MMEB_ADDR": "0x0",
            "MMEC_ADDR": "0x4001",
            "MSP_ADDR": "0x1",
            "MDP_ADDR": "0x201",
            "CCI_ADDR": "0x6401",
            "CSC_ADDR": "0x6001",
            "CDC_ADDR": "0x6101",
            "CRC_ADDR": "0x6601",
            "CSEA_ADDR": "0x6201",
            "CSEB_ADDR": "0x6251",
            "CSEC_ADDR": "0x6301"
        }
    }
}
    */

    pjsClass = cJSON_GetObjectItem(pJson, "attr"); 
    if(!pjsClass)
        goto __err_json_format;

    /* get common parameters from iec104_regsetting.json */
    pjsGroup=cJSON_GetObjectItem(pjsClass, "common");
    if(!pjsGroup)
        goto __err_json_format;

    ret = config_devices_json_group_common_read(pCfg, pjsGroup);
    if(ret < 0)
        goto __err_json_format;

    /* get ext parameters from iec104_regsetting.json */
    pjsGroup=cJSON_GetObjectItem(pjsClass, "ext");
    if(!pjsGroup)
        goto __err_json_format;

    ret = config_devices_json_group_ext_read(pCfg, pjsGroup);
    if(ret < 0)
        goto __err_json_format;

    /* get node parameters from iec104_regsetting.json */
    pjsGroup=cJSON_GetObjectItem(pjsClass, "node_ex");
    if(!pjsGroup)
        goto __err_json_format;

    pjsext=cJSON_GetObjectItem(pjsClass, "extend_param");
    if(!pjsext)
        goto __err_json_format;
        

// MMEA
            pjsnode=cJSON_GetObjectItem(pjsGroup, "MMEA");
            if(!pjsnode)
                goto __err_json_format;

            ret = config_devices_json_group_node_read(pCfg, &pCfg->MMEA, pjsnode, "MMEA");
            if(ret < 0)
                goto __err_json_format;

            pjsitem=cJSON_GetObjectItem(pjsext, "MMEA_ADDR");
            if(!pjsitem)
                goto __err_json_format;   
            sscanf(pjsitem->valuestring, "0x%X", &pCfg->MMEA.addr);

// MMEB
            pjsnode=cJSON_GetObjectItem(pjsGroup, "MMEB");
            if(!pjsnode)
                goto __err_json_format;

            ret = config_devices_json_group_node_read(pCfg, &pCfg->MMEB, pjsnode, "MMEB");
            if(ret < 0)
                goto __err_json_format;

            pjsitem=cJSON_GetObjectItem(pjsext, "MMEB_ADDR");
            if(!pjsitem)
                goto __err_json_format;   
            sscanf(pjsitem->valuestring, "0x%X", &pCfg->MMEB.addr);

// MMEC
            pjsnode=cJSON_GetObjectItem(pjsGroup, "MMEC");
            if(!pjsnode)
                goto __err_json_format;

            ret = config_devices_json_group_node_read(pCfg, &pCfg->MMEC, pjsnode, "MMEC");
            if(ret < 0)
                goto __err_json_format;     

            pjsitem=cJSON_GetObjectItem(pjsext, "MMEC_ADDR");
            if(!pjsitem)
                goto __err_json_format;   
            sscanf(pjsitem->valuestring, "0x%X", &pCfg->MMEC.addr);

// CCI
            pjsnode=cJSON_GetObjectItem(pjsGroup, "CCI");
            if(!pjsnode)
                goto __err_json_format;

            ret = config_devices_json_group_node_read(pCfg, &pCfg->CCI, pjsnode, "CCI");
            if(ret < 0)
                goto __err_json_format;    

            pjsitem=cJSON_GetObjectItem(pjsext, "CCI_ADDR");
            if(!pjsitem)
                goto __err_json_format;   
            sscanf(pjsitem->valuestring, "0x%X", &pCfg->CCI.addr);

// MSP
            pjsnode=cJSON_GetObjectItem(pjsGroup, "MSP");
            if(!pjsnode)
                goto __err_json_format;

            ret = config_devices_json_group_node_read(pCfg, &pCfg->MSP, pjsnode, "MSP");
            if(ret < 0)
                goto __err_json_format;

            pjsitem=cJSON_GetObjectItem(pjsext, "MSP_ADDR");
            if(!pjsitem)
                goto __err_json_format;   
            sscanf(pjsitem->valuestring, "0x%X", &pCfg->MSP.addr);

// MDP
            pjsnode=cJSON_GetObjectItem(pjsGroup, "MDP");
            if(!pjsnode)
                goto __err_json_format;

            ret = config_devices_json_group_node_read(pCfg, &pCfg->MDP, pjsnode, "MDP");
            if(ret < 0)
                goto __err_json_format;

            pjsitem=cJSON_GetObjectItem(pjsext, "MDP_ADDR");
            if(!pjsitem)
                goto __err_json_format;   
            sscanf(pjsitem->valuestring, "0x%X", &pCfg->MDP.addr);

// CSC
            pjsnode=cJSON_GetObjectItem(pjsGroup, "CSC");
            if(!pjsnode)
                goto __err_json_format;

            ret = config_devices_json_group_node_read(pCfg, &pCfg->CSC, pjsnode, "CSC");
            if(ret < 0)
                goto __err_json_format;  

            pjsitem=cJSON_GetObjectItem(pjsext, "CSC_ADDR");
            if(!pjsitem)
                goto __err_json_format;   
            sscanf(pjsitem->valuestring, "0x%X", &pCfg->CSC.addr);

// CDC
            pjsnode=cJSON_GetObjectItem(pjsGroup, "CDC");
            if(!pjsnode)
                goto __err_json_format;

            ret = config_devices_json_group_node_read(pCfg, &pCfg->CDC, pjsnode, "CDC");
            if(ret < 0)
                goto __err_json_format; 

            pjsitem=cJSON_GetObjectItem(pjsext, "CDC_ADDR");
            if(!pjsitem)
                goto __err_json_format;   
            sscanf(pjsitem->valuestring, "0x%X", &pCfg->CDC.addr);


// CRC
            pjsnode=cJSON_GetObjectItem(pjsGroup, "CRC");
            if(!pjsnode)
                goto __err_json_format;

            ret = config_devices_json_group_node_read(pCfg, &pCfg->CRC, pjsnode, "CRC");
            if(ret < 0)
                goto __err_json_format;

            pjsitem=cJSON_GetObjectItem(pjsext, "CRC_ADDR");
            if(!pjsitem)
                goto __err_json_format;   
            sscanf(pjsitem->valuestring, "0x%X", &pCfg->CRC.addr);

// CSEA
            pjsnode=cJSON_GetObjectItem(pjsGroup, "CSEA");
            if(!pjsnode)
                goto __err_json_format;

            ret = config_devices_json_group_node_read(pCfg, &pCfg->CSEA, pjsnode, "CSEA");
            if(ret < 0)
                goto __err_json_format;

            pjsitem=cJSON_GetObjectItem(pjsext, "CSEA_ADDR");
            if(!pjsitem)
                goto __err_json_format;   
            sscanf(pjsitem->valuestring, "0x%X", &pCfg->CSEA.addr);

// CSEB
            pjsnode=cJSON_GetObjectItem(pjsGroup, "CSEB");
            if(!pjsnode)
                goto __err_json_format;

            ret = config_devices_json_group_node_read(pCfg, &pCfg->CSEB, pjsnode, "CSEB");
            if(ret < 0)
                goto __err_json_format;    

            pjsitem=cJSON_GetObjectItem(pjsext, "CSEB_ADDR");
            if(!pjsitem)
                goto __err_json_format;   
            sscanf(pjsitem->valuestring, "0x%X", &pCfg->CSEB.addr);

// CSEC
            pjsnode=cJSON_GetObjectItem(pjsGroup, "CSEC");
            if(!pjsnode)
                goto __err_json_format;

            ret = config_devices_json_group_node_read(pCfg, &pCfg->CSEC, pjsnode, "CSEC");
            if(ret < 0)
                goto __err_json_format;  

            pjsitem=cJSON_GetObjectItem(pjsext, "CSEC_ADDR");
            if(!pjsitem)
                goto __err_json_format;   
            sscanf(pjsitem->valuestring, "0x%X", &pCfg->CSEC.addr); 



    cJSON_Delete(pJson);
    iec104_cfg_jsonfile_delete(p);

    if(pCfg->debug)
    {
        printf("MMEA :   0x%X\n", pCfg->MMEA.addr);
        printf("MMEB :   0x%X\n", pCfg->MMEB.addr);
        printf("MMEC :   0x%X\n", pCfg->MMEC.addr);
        printf("CCI  :   0x%X\n", pCfg->CCI.addr);
        printf("MSP  :   0x%X\n", pCfg->MSP.addr);
        printf("MDP  :   0x%X\n", pCfg->MDP.addr);
        printf("CSC  :   0x%X\n", pCfg->CSC.addr);
        printf("CDC  :   0x%X\n", pCfg->CDC.addr);
        printf("CRC  :   0x%X\n", pCfg->CRC.addr);
        printf("CSEA :   0x%X\n", pCfg->CSEA.addr);
        printf("CSEB :   0x%X\n", pCfg->CSEB.addr);
        printf("CSEC :   0x%X\n", pCfg->CSEC.addr);
    }


    return 0;

__err_json_format:
    cJSON_Delete(pJson);

__err_free:
    iec104_cfg_jsonfile_delete(p);

    return -1;
}


/*********************************************************************************************************
** Function name:       iec104_config_init
** Descriptions:        modbus rtu 协议配置初始化,主要是从EEPROM中加载配置，并判断配置是否正确
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
int iec104_config_init(struct config_chnl_working_attr *pCfg_chnlattr, char *cfg_file)
{
    int ret;
    
    ret = iec104_cfg_chnl_load(pCfg_chnlattr, cfg_file);
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
void iec104_config_destory(void)
{

}


