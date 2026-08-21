/*
 * File      : config_devices.c
 * This file is part of xgate5-fire
 * COPYRIGHT (C) 2006 - 2018, Pilot development, Communication team, chenzy.
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
#include "mb_com_baseclass.h"


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

    p[size] = '\0';
    fclose(fp);
    return p;

__err_close:
    fclose(fp);

    return NULL;
}


#if 1
/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int config_lora_workingattr(struct mb_optarg_t  *pLora, cJSON *pjsGroup)
{
	cJSON *pItem;
   
	/*  mode */
	pItem = cJSON_GetObjectItem(pjsGroup, "mode");
	if(!pItem)
		return -1;

    if(strcmp(pItem->valuestring, "lora") == 0)
    {
        pLora->lora_pktmode = SAMPLING_CFG_LORA_PILOT;
    }
    else if(strcmp(pItem->valuestring, "lora_trans") == 0)
    {
        pLora->lora_pktmode = SAMPLING_CFG_LORA_TRAN;
    }
    else
    {
        return -1;
    }

    /*  checked */
	pItem = cJSON_GetObjectItem(pjsGroup, "checked");
	if(!pItem)
		return -1;
	pLora->checked = pItem->valueint;
    
    /* cmd do not set. use the cfg file setting */
    if(pLora->signal_chnl == 0xff)
    {
        /*  signal_chnl */
        pItem = cJSON_GetObjectItem(pjsGroup, "signal_chnl");
        if(!pItem)
            return -1;
        pLora->signal_chnl = pItem->valueint; 
    }

    if(pLora->spreading_factor == 0xff)
    {
        /*  spreading_factor */
        pItem = cJSON_GetObjectItem(pjsGroup, "spreading_factor");
        if(!pItem)
            return -1;
        pLora->spreading_factor = pItem->valueint; 
    }

    if(pLora->band_width == 0xff)
    {
        /*  band_width */
        pItem = cJSON_GetObjectItem(pjsGroup, "band_width");
        if(!pItem)
            return -1;
        pLora->band_width = pItem->valueint; 
    }

    if(pLora->coding_rate == 0xff)
    {
        /*  coding_rate */
        pItem = cJSON_GetObjectItem(pjsGroup, "coding_rate");
        if(!pItem)
            return -1;
        pLora->coding_rate = pItem->valueint; 
    }

    if(pLora->center_frequency == 0xffffffff)
    {
	/*  center_frequency */
        pItem = cJSON_GetObjectItem(pjsGroup, "center_frequency");
        if(!pItem)
            return -1;
        pLora->center_frequency = pItem->valueint; 
    }

    if(pLora->LowDataRateOptimize == 0xff)
    {
    /*  LowDataRateOptimize */
        pItem = cJSON_GetObjectItem(pjsGroup, "LowDataRateOptimize");
        if(!pItem)
        {
            pLora->LowDataRateOptimize = 1 ;//default;
        }
        else
        {
            pLora->LowDataRateOptimize = ((pItem->valueint < 2) ? pItem->valueint : 1); 
        }
    }

    if(pLora->preamble_size == 0xffff)
    {
    /*  preamble_size */
        pItem = cJSON_GetObjectItem(pjsGroup, "preamble_size");
        if(!pItem)
        {
            pLora->preamble_size = 8;//default
        }
        else
        {
            if(pItem->valueint < 6 )
            {
                pItem->valueint = 6;
            }
            pLora->preamble_size = pItem->valueint; 
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
void config_lora_default(struct mb_optarg_t  *pLora)
{
    pLora->center_frequency = 470030000;
    pLora->signal_chnl = 0;
    pLora->coding_rate = 3;
    pLora->spreading_factor = 9;
    pLora->band_width = 7;

    pLora->preamble_size = 8;
    pLora->LowDataRateOptimize = 1; //pilot default open
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int config_lora_load(struct mb_optarg_t  *pLora, char *file)
{
    char    				*p;
	uint8_t					i;
    uint8_t     			chnl_num;
	int						size;
    int     				ret;
    cJSON   				*pJson, *pjsClass, *pjsGroup;

	if(access(file, F_OK) != 0)
	{
		return -1;
	}

	/*
		{
			"PollInv":0, 
			"channel":[
				{
					"id":0,
					"name":"rs485-1",
					"dev":"/dev/ttyS1",
					"Mode":"tty",
					"device":[
						{"phy_id":1, "logic_id":1, "cfg_file":"/userdata/config/collect/device/EGO700_1_1.json"},
						{"phy_id":2, "logic_id":2, "cfg_file":"/userdata/config/collect/device/EGO700_1_1.json"},
					]
				},
				{
					"id":1,
					"name":"lora-1",
					"dev":"/dev/lora",
					"Mode":"lora",
					"device":[
						{"phy_id":1, "logic_id":1, "cfg_file":"/userdata/config/collect/device/EGO700_1_1.json"},
						{"phy_id":2, "logic_id":2, "cfg_file":"/userdata/config/collect/device/EGO700_1_1.json"},
					]
				}
			]
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
        goto __err_free;

    pjsClass = cJSON_GetObjectItem(pJson, "channel"); 
    if(!pjsClass)
	{
        goto __err_json_format;
	}
	/* 
		channel numbers
	 */
	chnl_num = cJSON_GetArraySize(pjsClass);
	/*
		for each channel 
	*/
	for(i=0; i<chnl_num; i++)
	{
		/* get channel array */
		pjsGroup = cJSON_GetArrayItem(pjsClass, i); 
		if(!pjsGroup)
			continue;

		/* get channel work attribute */
		ret = config_lora_workingattr(pLora, pjsGroup);
		if(ret < 0)
			continue;
	}

	cJSON_Delete(pJson);
    config_devices_jsonfile_delete(p);

    return chnl_num;

__err_json_format:
    cJSON_Delete(pJson);

__err_free:
    config_devices_jsonfile_delete(p);
    return -1;

}

#endif