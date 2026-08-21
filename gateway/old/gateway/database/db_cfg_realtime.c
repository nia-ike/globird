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

#include "db.h"
#include "db_cfg_json.h"
#include "db_cfg_realtime.h"

/*
CHANNEL json 

{
    "Name":"uart",
    "info" : {
        "help":"This is Channel configure for UART. we leave id:1-16 for UART(rs232,rs485)"
    },
    "attr" : {
        "checked" : 1,
        "common" : {
            "id": 1,                                                                <=   we get this
            "checked": 1,
            "mode" : "poll",
            "link": "serial",
            "dev": "/dev/ttyU1",
            "serial":{
                "baud": 9600,
                "databit":8,
                "parity":"N",
                "stopbit":1
            },
            
            "repeat": 2,
            "inv_time": 500,
            "recvTimeout": 1000,
            "framing_time": 0
            
        },
        "devices":{
            "num"  : 1,	                                                            <=   we get this
            "attr" : [
                {
                    "logic_id": 0,                                                  <=   we get this
                    "phy_id": 0,
                    "loop_id": 0,
                    "protocol" :"modbusRTU",
                    "cfg_file": "/userdata/config/sample/devices/ego700.json",      <=   we get this
                    "desc": {
                        "cabinetNo":"P01-A0",
                        "name":"#1主进线",
                        "model":"SCB11-1000KVA",
                        "code":"FH07846545Y",
                        "capacity":1000,
                        "impedance":6
                    }
                }
            ]
        }
    }
}


DEVICE Json

{
    "devName":"EGO700",
    "info" : {
        "help":"meter's system factory setting. It is very importance parameter. this setting just can be set in factory!"
    },
    "attr" : {
        "checked" : 1,
        "sn" : "EGOM2007010001",
        "model" : "EGO700",
        "protocol" :"modbusRTU",
        "name" : "meter",
        "function": "power meter",
        "pkg_request" : {
            "pkt_num" : 2,
            "pkt_req_info" : [
                {"code":3,		"reg_no":20000, 		"reg_num":96},
                {"code":3,		"reg_no":20096, 	    "reg_num":118}
            ]
        },
        "node":{
            "node_num"  : 9,	                                                    <=   we get this
            "node_attr" : [
                {"id":0, 	"datatype":1,		"code":0,		"bitfield":0,	"bitnum":1, 	"datattr":0,		"reg_no":0,			"catagory":170,		"kfactor":1,			"name":"Commstatus",	"pt_flag1":1,			"ct_flag2": 1,			"unit":""},                         
                {"id":1, 	"datatype":9, 		"code":3, 		"bitfield":0, 	"bitnum":1, 	"datattr":0, 		"reg_no":20000, 	"catagory":11, 		"kfactor":0.01, 		"name":"Ua", 			"pt_flag1":1,         "ct_flag2":	1,			"unit":"V"},
                {"id":2, 	"datatype":9, 		"code":3, 		"bitfield":0, 	"bitnum":1, 	"datattr":0, 		"reg_no":20002, 	"catagory":12, 		"kfactor":0.01, 		"name":"Ub", 			"pt_flag1":1,         "ct_flag2":	1,			"unit":"V"},
                {"id":3, 	"datatype":9, 		"code":3, 		"bitfield":0, 	"bitnum":1, 	"datattr":0, 		"reg_no":20004, 	"catagory":13, 		"kfactor":0.01, 		"name":"Uc", 			"pt_flag1":1,         "ct_flag2":	1,			"unit":"V"},
                {"id":4, 	"datatype":9, 		"code":3, 		"bitfield":0, 	"bitnum":1, 	"datattr":0, 		"reg_no":20006, 	"catagory":19, 		"kfactor":0.01, 		"name":"Uab", 			"pt_flag1":1,         "ct_flag2":	1,			"unit":"V"},
                {"id":5, 	"datatype":9, 		"code":3, 		"bitfield":0, 	"bitnum":1, 	"datattr":0, 		"reg_no":20008, 	"catagory":20, 		"kfactor":0.01, 		"name":"Ubc", 			"pt_flag1":1,         "ct_flag2":	1,			"unit":"V"},
                {"id":6, 	"datatype":9, 		"code":3, 		"bitfield":0, 	"bitnum":1, 	"datattr":0, 		"reg_no":20010, 	"catagory":21, 		"kfactor":0.01, 		"name":"Uca", 			"pt_flag1":1,         "ct_flag2":	1,			"unit":"V"},
                {"id":7, 	"datatype":9, 		"code":3, 		"bitfield":0, 	"bitnum":1, 	"datattr":0, 		"reg_no":20012, 	"catagory":15, 	    "kfactor":0.001, 		"name":"Ia", 			"pt_flag1":1,	        "ct_flag2":1,		    "unit":"A"},
                {"id":8, 	"datatype":9, 		"code":3, 		"bitfield":0, 	"bitnum":1, 	"datattr":15, 		"reg_no":0, 	    "catagory":16, 		"kfactor":0.001, 		"name":"虚拟点1", 		 "pt_flag1":1,	        "ct_flag2":1,		    "unit":"A"}
                
           ]
        },
        "control":{
            "num":2,
            "attr" : [
                {"id":0, "datatype":1 , "funcode":16,		"regno":200 , "name":"CT"  },
                {"id":1, "datatype":1 , "funcode":16,		"regno":258 , "name":"消警"}
            ]
        },
        "alarm":{
            "num" :1,
            "attr" : [
                {
                    "ruleid" : 0, "name" : "leak", "type": "leak", "mode" :0, "htime" :20,
                    "alarm_cond" : [ 
                        {   "pointid":15,        "type":"const",    "level":1,         "method": 1,        "ref":300,    "ref_per":100,        "Hysteresis":10          },   
                        {   "pointid":15,        "type":"const",    "level":1,         "method": 2,        "ref":300,    "ref_per":100,        "Hysteresis":10          }
                    ]
                }
            ]
        }
        
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
static void _db_sort_swap(struct db_spl_remap_info_t index[], int low, int high)
{
    struct db_spl_remap_info_t tmp = index[low];

    index[low] = index[high];
    index[high] = tmp;
}

/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:            create remap linear index                      **/
/** Params:             index table and device count                   **/
/** Return:             None                                           **/
/** Notice:             None                                           **/
/*************************************************************************/
static void _db_remap_create_index(struct db_spl_remap_info_t index[], uint16_t devnum)
{
    int i;

    if(devnum == 0)
        return;

    index[0].index = 0;
    for(i=0; i<(devnum-1); i++)
    {
        index[i+1].index = index[i].index + index[i].node_num;
    }
}

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int _db_sort_partition(struct db_spl_remap_info_t index[], int low, int high)
{
    struct db_spl_remap_info_t point = index[low];

    while(low < high)
    {
        while(low < high && index[high].id.mix >= point.id.mix)//控制high指针比较并左移
        {
            high--;
        }  
        _db_sort_swap(index, low, high);

        while(low < high && index[low].id.mix <= point.id.mix)//控制low指针比较并右移
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
static void _db_do_sort(struct db_spl_remap_info_t index[], int low, int high)
{
    int point;

    if(low < high)
    {
        point = _db_sort_partition(index, low, high);//计算基准点
        _db_do_sort(index, low, point-1);  //对基准点的左边进行排序
        _db_do_sort(index, point+1, high);//对基准点的右边进行排序
    }
    
}

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static void _db_create_index(struct db_spl_realtime_data_tbl_t *pDB)
{
    struct db_spl_realtime_data_tbl_256_t *pDB256 = (struct db_spl_realtime_data_tbl_256_t *)pDB;

    /*
        由于设备节点不留空洞，设备与设备之间紧密挨着
        设备节点数量不定，因此需要索引来检索其实位置

        第0号设备，其索引即0
        第1号设备，其索引为0+node_num 
        以此类推
        第pDB->device_num-1号设备
    */
    if(pDB->device_num <= DB_CFG_MAX_DEV_COMPATIBLE)
    {
        _db_remap_create_index(pDB->index, pDB->device_num);
    }
    else 
    {
        _db_remap_create_index(pDB256->index, pDB->device_num);
    }
    
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int _db_cfg_rt_map_counts_from_device(struct db_spl_realtime_data_tbl_t *pDB, char *file, int *node_num, int *ctrl_num)
{
    char    *p;
    int     size;
    cJSON   *pJson, *pJson_lay1, *pJson_lay2, *pJson_Item;
    int     node_count = 0, ctrl_count = 0;
 
    if((node_num == NULL) || (ctrl_num == NULL))
        return -1;

    *node_num = 0;
    *ctrl_num = 0;

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
 
    /* get node parameters from devices.json */
    pJson_lay2=cJSON_GetObjectItem(pJson_lay1, "node");
    if(!pJson_lay2)
        goto __err_json_format;

    pJson_Item = cJSON_GetObjectItem(pJson_lay2, "node_num");
    if(pJson_Item == NULL)
        goto __err_json_format;

    node_count = pJson_Item->valueint;
    if(node_count < 0)
        node_count = 0;

    /* get control parameters from devices.json (optional) */
    pJson_lay2 = cJSON_GetObjectItem(pJson_lay1, "control");
    if(pJson_lay2 != NULL)
    {
        /* 兼容两种格式：control.num / control.control_num */
        pJson_Item = cJSON_GetObjectItem(pJson_lay2, "num");
        if(pJson_Item == NULL)
            pJson_Item = cJSON_GetObjectItem(pJson_lay2, "control_num");

        if(pJson_Item != NULL)
        {
            ctrl_count = pJson_Item->valueint;
            if(ctrl_count < 0)
                ctrl_count = 0;
        }
    }

    cJSON_Delete(pJson);
    db_cfg_jsonfile_delete(p);

    *node_num = node_count;
    *ctrl_num = ctrl_count;
    return 0;

__err_json_format:
    cJSON_Delete(pJson);

__err_free:
    db_cfg_jsonfile_delete(p);

    return -1;
}



/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int _db_cfg_rt_map_chnnl_load(struct database_ctx_t *pCtx, char *file)
{
    char    *p;
    int     size, ret = -1;
    uint8_t i;
    uint8_t chnlno = 0, devnum, devno = 0;
    cJSON   *pJson, *pJson_lay1, *pJson_lay2, *pJson_lay3, *pJson_lay4, *pJson_Item;
    struct db_spl_realtime_data_tbl_t *pDB = pCtx->pDB;
    struct db_spl_realtime_data_tbl_256_t *pDB256 = (struct db_spl_realtime_data_tbl_256_t *)pCtx->pDB;
    struct db_spl_ctrl_tbl_t *pCtrl = &pDB->ctrl_node_region[0].context;

    if(pCtx->debug)
    {
        printf("\nINIT: Channel:%s\n", file);
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
 
    /* get channel number parameters from channel.json */
    pJson_lay2=cJSON_GetObjectItem(pJson_lay1, "common");
    if(!pJson_lay2)
        goto __err_json_format;

    pJson_Item = cJSON_GetObjectItem(pJson_lay2, "id");
    if(pJson_Item == NULL)
        goto __err_json_format;
    
    chnlno = pJson_Item->valueint;

    /* get device number parameters from channel.json */
    pJson_lay2=cJSON_GetObjectItem(pJson_lay1, "devices");
    if(!pJson_lay2)
        goto __err_json_format;

    pJson_lay3 = cJSON_GetObjectItem(pJson_lay2, "attr");
    if(pJson_lay3 == NULL)
        goto __err_json_format;

    devnum = cJSON_GetArraySize(pJson_lay3); 
    for(i=0; i<devnum; i++)
    {
        int node_num = 0, ctrl_num = 0;

        pJson_lay4 = cJSON_GetArrayItem(pJson_lay3, i); 
        pJson_Item = cJSON_GetObjectItem(pJson_lay4, "logic_id");
        if(pJson_Item == NULL)
            goto __err_json_format;

        devno = atoi(pJson_Item->valuestring);

        if(pDB->device_num >= DB_CFG_MAX_DEV)
            goto __err_json_format;

        // 兼容设计
        if(pDB->device_num <= DB_CFG_MAX_DEV_COMPATIBLE)
        {
            pDB->index[pDB->device_num].id.split.ucnPort = chnlno;
            pDB->index[pDB->device_num].id.split.ucnDev  = devno;
        }
        else 
        {
            pDB256->index[pDB->device_num].id.split.ucnPort = chnlno;
            pDB256->index[pDB->device_num].id.split.ucnDev  = devno;
        }
        
        pJson_Item = cJSON_GetObjectItem(pJson_lay4, "cfg_file");
        if(pJson_Item == NULL)
            goto __err_json_format;

        if(_db_cfg_rt_map_counts_from_device(pDB, pJson_Item->valuestring, &node_num, &ctrl_num) != 0)
            goto __err_json_format;

        /* 
            获取当前设备的采集点数量
            采集点不留空洞，设备与设备之前采集点紧挨一起
         */
        // 兼容设计
        if(pDB->device_num <= DB_CFG_MAX_DEV_COMPATIBLE)
        {
            pDB->index[pDB->device_num].node_num = node_num;
            pDB->node_num += pDB->index[pDB->device_num].node_num;
        }
        else 
        {
            pDB256->index[pDB->device_num].node_num = node_num;
            pDB->node_num += pDB256->index[pDB->device_num].node_num;
        }

        if(pCtx->debug)
        {
            printf("       ID:%04X,   NodeNum:%04d  TotalNodeNum:%04X,  DEVICE:%s\n", pDB->index[pDB->device_num].id.mix,  pDB->index[pDB->device_num].node_num, pDB->node_num, pJson_Item->valuestring);
        }

        /*
            控制点映射：独立于采集点 node/index 管理
            控制点总数由设备 json 的 control.num/control_num 提供
         */
        if(pCtrl->device_num >= DB_CFG_MAX_DEV)
            goto __err_json_format;

        {
            pCtrl->index[pCtrl->device_num].id.split.ucnPort = chnlno;
            pCtrl->index[pCtrl->device_num].id.split.ucnDev  = devno;
            pCtrl->index[pCtrl->device_num].node_num = (uint16_t)ctrl_num;
        }
        pCtrl->node_num += pCtrl->index[pCtrl->device_num].node_num;
        pCtrl->device_num++;

        if(pCtx->debug)
        {
            printf("       CTRL: ID:%04X, NodeNum:%04d TotalCtrlNodeNum:%04X, DEVICE:%s\n",
                   pCtrl->index[pCtrl->device_num - 1].id.mix,
                   pCtrl->index[pCtrl->device_num - 1].node_num,
                   pCtrl->node_num,
                   pJson_Item->valuestring);
        }

        pDB->device_num++;
    }

    if(pCtx->debug)
    {
        if(devnum == 0)
        {
            printf("This Channel No devices\n");
        }
    }

    ret = 0;

__err_json_format:
    cJSON_Delete(pJson);

__err_free:
    db_cfg_jsonfile_delete(p);

    return ret;
}



/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int db_cfg_rt_map_load(struct database_ctx_t *pCtx, char *pcPath)
{
    struct dirent *dp;
    DIR *dfd;
    struct db_spl_realtime_data_tbl_t *pDB = pCtx->pDB;
    struct db_spl_ctrl_tbl_t *pCtrl = &pDB->ctrl_node_region[0].context;
    char cfgfile[128];

    if (!(dfd = opendir(pcPath))) {
        fprintf(stderr, "dirwalk:can not open %s\n", pcPath);
        return -1;
    }

    /* start to load the sampling map from JSON cfg file */
    pDB->device_num = 0;
    pDB->node_num = 0;
    pCtrl->device_num = 0;
    pCtrl->node_num = 0;

    /*
        从采集配置的通道目录，遍历chnl_x.json文件
        形成 以 PORT+DEVNO （端口号和设备号）为ID的索引表，以便其他进程检索使用
        形成 ID索引表，ID索引表排序，检索进程可用二分法检索

        ___________________
        |	 有效设备数		|
        ___________________
        |					|
        |     index     64/256  |  
        | _________________ |
        |     dev 1     256 |  
        |     dev 2         |  
        |     dev 3         |
        |	  ...           |	
        ---------------------
        
    */
    while((dp = readdir(dfd)) != NULL) 
    {
        /* 遍历目录下以  chnl_开头的json文件 */
        if(strstr(dp->d_name, "chnl_"))
        {
            /*
                根据配置生成索引表
            */
            snprintf(cfgfile, sizeof(cfgfile), "%s%s", pcPath, dp->d_name);
            _db_cfg_rt_map_chnnl_load(pCtx, cfgfile);
        }
        
    }

    closedir(dfd);

    /* 
        索引从小到大排序

        注意：本进程只需处理索引。  dev点表固定最大256个单元，根据顺序与索引匹配
     */
    if(pDB->device_num > 0)
        _db_do_sort(pDB->index, 0, pDB->device_num - 1);
    _db_create_index(pDB);

    /*
        控制点索引：按 PORT+DEV 进行排序并生成线性起始索引
     */
    if(pCtrl->device_num > 0)
        _db_do_sort(pCtrl->index, 0, pCtrl->device_num - 1);
    _db_remap_create_index(pCtrl->index, pCtrl->device_num);

    if(pCtx->debug)
    {
        printf("------------------------------------------------------------------\n");
        printf("Total: Devices Number:%05d\n", pDB->device_num);
        printf("       Node    Number:%05d\n", pDB->node_num);
        printf("       CtrlDev Number:%05d\n", pCtrl->device_num);
        printf("       CtrlNodeNumber:%05d\n", pCtrl->node_num);

    }

    return 0;

}