/*
 * File      : config_devices.c
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
#include "config_devices.h"
#include "sampling_cfg.h"	
#include "debug.h"

#include "iec104_cfg.h"


// 快速排序算法。基于inf_addr
/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static void _db_sort_swap(struct config_node_attr index[], int low, int high)
{
    struct config_node_attr tmp = index[low];

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
static int _db_sort_partition(struct config_node_attr index[], int low, int high)
{
    struct config_node_attr point = index[low];

    while(low < high)
    {
        while(low < high && index[high].inf_addr >= point.inf_addr)//控制high指针比较并左移
        {
            high--;
        }  
        _db_sort_swap(index, low, high);

        while(low < high && index[low].inf_addr <= point.inf_addr)//控制low指针比较并右移
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
static void _db_do_sort(struct config_node_attr index[], int low, int high)
{
    int point;

    if(low < high)
    {
        point = _db_sort_partition(index, low, high);//计算基准点
        _db_do_sort(index, low, point-1);  //对基准点的左边进行排序
        _db_do_sort(index, point+1, high);//对基准点的右边进行排序
    }
    
}

// END OF 快速排序算法。基于inf_addr

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int mb_get_nodeattr_by_infaddr(struct config_node_attr index[], uint16_t inf_addr, int low, int high)
{
    int mid ;

    while(low <= high)
    {
        mid = (low + high) / 2;
        if(index[mid].inf_addr < inf_addr)
        {
            low = mid + 1;
        }
        else if(index[mid].inf_addr > inf_addr)
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
static int db_cfg_json_writetofile(char *file, char *jsonstring)
{
    FILE *fp = NULL;

    if(!file)
        return -1;
    if(!jsonstring)
        return -1;

    fp = fopen(file, "wt");
    if(fp == NULL)
        return -1;

    fprintf(fp, "%s", jsonstring);

    fclose(fp);

    return 0;
}


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
static int config_channel_workingattr(struct config_channel_attr *pChannelAttr, cJSON *pJson_lay1)
{
	cJSON 					*pItem;
	cJSON   				*pJson_lay2, *pJson_lay3;

	/*  checked */
	pItem = cJSON_GetObjectItem(pJson_lay1, "checked");
	if(!pItem)
		return -1;
	pChannelAttr->working_attr.checked = pItem->valueint; 

/* common lay2 */
	
	pJson_lay2 = cJSON_GetObjectItem(pJson_lay1, "common");
	if(!pJson_lay2)
		return -1;

	/*  id */
	pItem = cJSON_GetObjectItem(pJson_lay2, "id");
	if(!pItem)
		return -1;
	pChannelAttr->working_attr.id = pItem->valueint; 

	/*  name. no set */
	memset(pChannelAttr->working_attr.name, 0, sizeof(pChannelAttr->working_attr.name));

	/*  mode */
	pItem = cJSON_GetObjectItem(pJson_lay2, "mode");
	if(!pItem)
		return -1;
	strncpy(pChannelAttr->working_attr.mode, pItem->valuestring, sizeof(pChannelAttr->working_attr.mode)-1);

	/*  link */
	pItem = cJSON_GetObjectItem(pJson_lay2, "link");
	if(!pItem)
		return -1;
	strncpy(pChannelAttr->working_attr.link, pItem->valuestring, sizeof(pChannelAttr->working_attr.link)-1);

	/*  dev */
	pItem = cJSON_GetObjectItem(pJson_lay2, "dev");
	if(!pItem)
		return -1;	
	strncpy(pChannelAttr->working_attr.dev, pItem->valuestring, sizeof(pChannelAttr->working_attr.dev)-1);
	sscanf(pChannelAttr->working_attr.dev, "%[^:]:%d", (char *)&pChannelAttr->working_attr.dst_url[0], &pChannelAttr->working_attr.dst_port);

	/*  repeat */
	pItem = cJSON_GetObjectItem(pJson_lay2, "repeat");
	if(!pItem)
		return -1;
	pChannelAttr->working_attr.repeat = pItem->valueint; 
    if(pChannelAttr->working_attr.repeat == 0){
        pChannelAttr->working_attr.repeat = 1;
    }
        

	/*  inv_time */
	pItem = cJSON_GetObjectItem(pJson_lay2, "inv_time");
	if(!pItem)
		return -1;
	pChannelAttr->working_attr.inv_time = pItem->valueint; 

	/*  RecvTimeOut */
	pItem = cJSON_GetObjectItem(pJson_lay2, "RecvTimeOut");
	if(!pItem)
		return -1;
	pChannelAttr->working_attr.RecvTimeOut = pItem->valueint; 
    if(pChannelAttr->working_attr.RecvTimeOut == 0)
    {
        pChannelAttr->working_attr.RecvTimeOut = 1000; 
    }
        

	/*  framing_time */
	pItem = cJSON_GetObjectItem(pJson_lay2, "framing_time");
	if(!pItem)
		return -1;
	pChannelAttr->working_attr.framing_time = pItem->valueint; 

	/*  serial */
	pJson_lay3 = cJSON_GetObjectItem(pJson_lay2, "serial");
	if(!pJson_lay3)
		return -1;

	/*  baud */
	pItem = cJSON_GetObjectItem(pJson_lay3, "baud");
	if(!pItem)
		return -1;
	pChannelAttr->working_attr.serial_attr.baud = pItem->valueint; 

	/*  databit */
	pItem = cJSON_GetObjectItem(pJson_lay3, "databit");
	if(!pItem)
		return -1;
	pChannelAttr->working_attr.serial_attr.datasize = pItem->valueint; 

	/*  parity */
	pItem = cJSON_GetObjectItem(pJson_lay3, "parity");
	if(!pItem)
		return -1;
	
	pChannelAttr->working_attr.serial_attr.parity = pItem->valuestring[0];

	/*  stopbit */
	pItem = cJSON_GetObjectItem(pJson_lay3, "stopbit");
	if(!pItem)
		return -1;
	pChannelAttr->working_attr.serial_attr.stopbit = pItem->valueint; 

	//if(pChnlPollMgr->poptarg->dump)
	{
		printf("checked:%d \n", pChannelAttr->working_attr.checked);
		printf("     id:%d \n", pChannelAttr->working_attr.id);
		printf("   mode:%s \n", pChannelAttr->working_attr.mode);
		printf("   link:%s \n", pChannelAttr->working_attr.link);
		printf("    dev:%s \n", pChannelAttr->working_attr.dev);
		printf("    url:%s \n", pChannelAttr->working_attr.dst_url);
		printf("   port:%d \n", pChannelAttr->working_attr.dst_port);
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
static void config_device_attr_init(struct config_channel_attr *pChannelAttr)
{
	/* initial the devices list head */
	slist_init(&pChannelAttr->devices_list_head.slist);

}

	
/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static struct config_devices_info_attr *config_device_info_attr_new(void)
{
	return calloc(1, sizeof(struct config_devices_info_attr));
}

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static void config_device_info_attr_delete(struct config_devices_info_attr *pDevInfoAttr)
{
	if(pDevInfoAttr)
		free(pDevInfoAttr);
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int config_device_info_attr_append(struct config_channel_attr *pChannelAttr, struct config_devices_info_attr *pDevInfoAttr)
{
	if(!pDevInfoAttr)
		return -1;

	slist_append(&pChannelAttr->devices_list_head.slist, &pDevInfoAttr->slist);
	
	return 0;
}

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int config_device_info_attr_parse(struct config_channel_attr *pChannelAttr, struct config_devices_info_attr *pDevInfoAttr, cJSON *pjsGroupDev)
{
	cJSON 		*pItem;
	char 		*pstr;
	
	/* logic id, sequeue number, start from 0 */
	pItem = cJSON_GetObjectItem(pjsGroupDev, "logic_id");
	if(!pItem)
		return -1;
	pDevInfoAttr->logic_id = strtoll(pItem->valuestring, &pstr, 10); 

	/* phy id */
	pItem = cJSON_GetObjectItem(pjsGroupDev, "phy_id");
	if(!pItem)
		return -1;
	pDevInfoAttr->phy_id = strtoll(pItem->valuestring, &pstr, 10); 

	/* loop_id */
//	pItem = cJSON_GetObjectItem(pjsGroupDev, "loop_id");
//	if(!pItem)
//		return -1;
//	pDevInfoAttr->loop_id = pItem->valueint; 

	/* get this device dll path,  */
	pItem = cJSON_GetObjectItem(pjsGroupDev, "dll");
	if(!pItem)
		return -1;

	strncpy(pDevInfoAttr->dll, pItem->valuestring, sizeof(pDevInfoAttr->dll) - 1);

	/* get this device protocol path,  */
	pItem = cJSON_GetObjectItem(pjsGroupDev, "protocol");
	if(!pItem)
		return -1;

	strncpy(pDevInfoAttr->protocol, pItem->valuestring, sizeof(pDevInfoAttr->protocol) - 1);

	/* get this device cfg file path,  */
	pItem = cJSON_GetObjectItem(pjsGroupDev, "cfg_file");
	if(!pItem)
		return -1;

	strncpy(pDevInfoAttr->cfg_file, pItem->valuestring, sizeof(pDevInfoAttr->cfg_file) - 1);

	{
		printf("------------------------\n");
		printf("	logic_id:%d \n", pDevInfoAttr->logic_id);
		printf("	  phy_id:%lld \n", pDevInfoAttr->phy_id);
		printf("	     dll:%s \n", pDevInfoAttr->dll);
		printf("    protocol:%s \n", pDevInfoAttr->protocol);
		printf("	cfg_file:%s \n", pDevInfoAttr->cfg_file);
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
static int config_channel_devlistattr(struct config_channel_attr *pChannelAttr, cJSON *pjsGroup)
{	
	uint8_t 					i;
	uint8_t 					dev_num;
	int 						ret;
	cJSON						*pjsClassDev, *pjsGroupDev;
	struct config_devices_info_attr  *pDevInfoAttr;

	/* get device  */
	pjsClassDev = cJSON_GetObjectItem(pjsGroup, "devices");
	if(!pjsClassDev)
		return -1;

	/* get device attribute */
	pjsClassDev = cJSON_GetObjectItem(pjsClassDev, "attr");
	if(!pjsClassDev)
		return -1;	

	config_device_attr_init(pChannelAttr);

	/* 
		dev numbers
	*/
	dev_num = cJSON_GetArraySize(pjsClassDev);
	// iec104 网络通道目前只支持单个设备
//	dev_num = dev_num > 1 ? 1 : dev_num;
	for(i=0; i<dev_num; i++)
	{
		pjsGroupDev = cJSON_GetArrayItem(pjsClassDev, i);
		if(!pjsGroupDev)
			return -1;

		pDevInfoAttr = config_device_info_attr_new();
		if(!pDevInfoAttr)
			return -1;

		/* channel id */
		pDevInfoAttr->channel_id = pChannelAttr->working_attr.id;

		/* get dev attribute */
		ret = config_device_info_attr_parse(pChannelAttr, pDevInfoAttr, pjsGroupDev);
		if(ret < 0)
		{
			config_device_info_attr_delete(pDevInfoAttr);
			return -1;
		}

		// 加载设备配置文件
		iec104_cfg_load(&pDevInfoAttr->DevAttr, pDevInfoAttr->cfg_file);

		// 每台设备节点数排序，用于快速查找
		if(pDevInfoAttr->DevAttr.node_num > 0)
			_db_do_sort(&pDevInfoAttr->DevAttr.node_attr[0], 0,pDevInfoAttr->DevAttr.node_num - 1);

		config_device_info_attr_append(pChannelAttr, pDevInfoAttr);
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
static int config_channel_load(struct config_channel_attr *pChannelAttr, char *file)
{
    char    				*p;
	int						size;
    int     				ret;
    cJSON   				*pJson, *pJson_lay1;

	if(access(file, F_OK) != 0)
	{
		return -1;
	}

	/*
		{
			"Name":"uart",
			"info" : {
				"help":"This is Channel configure for UART. we leave id:1-16 for UART(rs232,rs485)"
			},
			"attr" : {
				"checked" : 1,
				"common" : {
					"id": 1,
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
					"num"  : 1,	
					"attr" : [
						{
							"logic_id": "1",
                    		"phy_id": "1",
							"loop_id": 0,
							"protocol" :"modbusRTU",
							"dll" : "/userdata/config/dll/modbusRTU.so", 
							"cfg_file": "/userdata/config/sample/devices/ego700.json",
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
	*/
	/* prepare the buffer for json */
    p = config_devices_jsonfile_alloc(file, &size);
    if(p == NULL)
        return -1;
  
    p = config_devices_jsonfile_read(file, p, size);
    if(p == NULL)
        goto __err_free;

    pJson = cJSON_Parse(p);  
    if(!pJson)
	{
        goto __err_free;
	}

	pJson_lay1 = cJSON_GetObjectItem(pJson, "attr"); 
	if(!pJson_lay1)
        goto __err_json_format;

	/* get channel work attribute */
	ret = config_channel_workingattr(pChannelAttr, pJson_lay1);
	if(ret < 0)
		goto __err_json_format;

	ret = config_channel_devlistattr(pChannelAttr, pJson_lay1);

	cJSON_Delete(pJson);
    config_devices_jsonfile_delete(p);

    return 0;

__err_json_format:
    cJSON_Delete(pJson);

__err_free:
    config_devices_jsonfile_delete(p);
    return -1;

}



/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int config_channel_init(struct cs104_mgr_t *pcs104Mgr, char *file)
{
	int ret;

	ret = config_channel_load(&pcs104Mgr->ChannelAttr, file);
	if(ret < 0)
		return -1;

	return 0;
}









