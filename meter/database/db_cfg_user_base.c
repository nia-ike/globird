/*
* Copyright (c) 2013,珠海派诺电子有限公司
* All rights reserved.
* 
* 文件名称：db_cfg.c
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
*  chenzy   2020.6.09   V1.00      创建
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <unistd.h> 
#include <sys/stat.h>  //stat
#include "cJSON.h"
#include "cJSON_Utils.h"

#include "db_cfg_user_base.h"
#include "db_struct.h"
#include "db_cp.h"

/*
    CFG map table
    参数与JSON映射表：
    用作解析json时匹配参数变量（避免在函数中大量if导致源码过长，难以维护修改）
    struct db_base_mapdef 结构成员 
        name   字符串   对应于 json参数和结构体相应的参数
        offset 该参数在sys_cfg（或者user_cfg）结构的偏移。 用作JSON参数读取检索时计算该参数的地址（这里不用地址用偏移，是因为参数配置的结构体实体是变化的，非常量）

        两个offsetof相加是因为，该参数相对于sys_cfg（或者user_cfg），包了2层结构体

    提示：
        这里定义的 参数名称：  结构体变量名称 与 JSON 配置的名称 必须一致！
        但 ：
             1. 顺序可以不一致
             2. 允许JSON的参数字段数量比 这里定义多，但不能少
        因此，只要以后增加配置，json增删可以做到兼容
*/

/*
    USER CFG MAP：

*/
#define DB_MAP_TABLE_BASE(x)                 DB_toString(x),(offsetof(struct db_cfg_user_base_para,x)+offsetof(struct db_cfg_user_para,base))
#define DB_MAP_TABLE_PULSE(x)                DB_toString(x),(offsetof(struct db_cfg_user_pulseout_para,x)+offsetof(struct db_cfg_user_para,pulseout))
#define DB_MAP_TABLE_DEMAND(x)               DB_toString(x),(offsetof(struct db_cfg_user_demand_para,x)+offsetof(struct db_cfg_user_para,demand))
#define DB_MAP_TABLE_LOADRUN(x)              DB_toString(x),(offsetof(struct db_cfg_user_loadrun_para,x)+offsetof(struct db_cfg_user_para,loadrun))
#define DB_MAP_TABLE_REFER(x)                DB_toString(x),(offsetof(struct db_cfg_user_refer_para,x)+offsetof(struct db_cfg_user_para,refer))
#define DB_MAP_TABLE_NETWORK(x)              DB_toString(x),(offsetof(struct db_cfg_user_network_para,x)+offsetof(struct db_cfg_user_para,network))
#define DB_MAP_TABLE_RUNING_ENERGY(x)        DB_toString(x),(offsetof(struct db_cfg_user_runing_para,x)+offsetof(struct db_cfg_user_para,runing))


typedef struct json_new_object_u32{
    cJSON   *object;
    char     name[32];
    uint32_t Val;
    uint32_t ValDef;
    uint32_t Max;
    uint32_t Min;
} json_new_object_u32_t;


typedef struct json_new_object_u16{
    cJSON   *object;
    char     name[32];
    uint16_t Val;
    uint16_t ValDef;
    uint16_t Max;
    uint16_t Min;
} json_new_object_u16_t;

/*
  USER CFG MAP STRUCT START
*/
static const struct db_base_mapdef db_user_base_map[] = 
{
    { DB_MAP_TABLE_BASE(PassWord    ), sizeof(uint16_t)},
    { DB_MAP_TABLE_BASE(Addr        ), sizeof(uint16_t)},
    { DB_MAP_TABLE_BASE(BaudRate    ), sizeof(uint32_t)},
    { DB_MAP_TABLE_BASE(CharSize    ), sizeof(uint16_t)},
    { DB_MAP_TABLE_BASE(ParityBit   ), sizeof(uint16_t)},
    { DB_MAP_TABLE_BASE(StopBit     ), sizeof(uint16_t)},
    { DB_MAP_TABLE_BASE(WiringMode  ), sizeof(uint16_t)},
    { DB_MAP_TABLE_BASE(PT          ), sizeof(uint32_t)},
    { DB_MAP_TABLE_BASE(CT          ), sizeof(uint32_t)},
    { DB_MAP_TABLE_BASE(RelayMode   ), sizeof(uint16_t)},
    { DB_MAP_TABLE_BASE(PhaseAdjust1), sizeof(uint16_t)},
    { DB_MAP_TABLE_BASE(PhaseAdjust2), sizeof(uint16_t)},
    { DB_MAP_TABLE_BASE(PhaseAdjust3), sizeof(uint16_t)},
    { DB_MAP_TABLE_BASE(Language    ), sizeof(uint16_t)},
};

static const struct db_base_mapdef db_user_pulse_map[] = 
{
    { DB_MAP_TABLE_PULSE(checked                ),  sizeof(uint16_t)},
    { DB_MAP_TABLE_PULSE(pulse[0].mode          ),  sizeof(uint16_t)},
    { DB_MAP_TABLE_PULSE(pulse[0].constant      ),  sizeof(uint16_t)},
    { DB_MAP_TABLE_PULSE(pulse[0].wide          ),  sizeof(uint16_t)},
    { DB_MAP_TABLE_PULSE(pulse[1].mode          ),  sizeof(uint16_t)},
    { DB_MAP_TABLE_PULSE(pulse[1].constant      ),  sizeof(uint16_t)},
    { DB_MAP_TABLE_PULSE(pulse[1].wide          ),  sizeof(uint16_t)},
};

static const struct db_base_mapdef db_user_demand_map[] = 
{
    { DB_MAP_TABLE_DEMAND(mode       ), sizeof(uint16_t)},
    { DB_MAP_TABLE_DEMAND(window     ), sizeof(uint16_t)},
    { DB_MAP_TABLE_DEMAND(slip_window ), sizeof(uint16_t)},
};


static const struct db_base_mapdef db_user_loadrun_map[] = 
{
    { DB_MAP_TABLE_LOADRUN(obj           ),  sizeof(uint16_t)},
    { DB_MAP_TABLE_LOADRUN(over_I        ),  sizeof(uint32_t)},
    { DB_MAP_TABLE_LOADRUN(over_V        ),  sizeof(uint32_t)},
    { DB_MAP_TABLE_LOADRUN(under_V       ),  sizeof(uint32_t)},
    { DB_MAP_TABLE_LOADRUN(over_Ptot     ),  sizeof(uint32_t)},
};

static const struct db_base_mapdef db_user_refer_map[] = 
{
    { DB_MAP_TABLE_REFER(refer_voltage        ),  sizeof(uint32_t)},
    { DB_MAP_TABLE_REFER(refer_current        ),  sizeof(uint32_t)},
    { DB_MAP_TABLE_REFER(refer_frequency      ),  sizeof(uint32_t)},
    { DB_MAP_TABLE_REFER(refer_Temperature    ),  sizeof(uint32_t)},
};

static const struct db_base_mapdef db_user_network_map[] = 
{
    { DB_MAP_TABLE_NETWORK(ip_addr),  sizeof(uint32_t)},
    { DB_MAP_TABLE_NETWORK(subnet),  sizeof(uint32_t)},
    { DB_MAP_TABLE_NETWORK(gateway),  sizeof(uint32_t)},
    { DB_MAP_TABLE_NETWORK(dns),  sizeof(uint32_t)},
    { DB_MAP_TABLE_NETWORK(dhcp),  sizeof(uint32_t)},
    
};

static const struct db_base_mapdef db_user_runing_energy_set_map[] = 
{
    { DB_MAP_TABLE_RUNING_ENERGY(setPtot),  sizeof(uint32_t)},
    { DB_MAP_TABLE_RUNING_ENERGY(setQtot),  sizeof(uint32_t)},
    { DB_MAP_TABLE_RUNING_ENERGY(setStot),  sizeof(uint32_t)},
};

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int db_cfg_json_writetofile(const char *file, char *jsonstring)
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
static char *db_cfg_jsonfile_alloc(const char *file, int *size)
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
static void db_cfg_jsonfile_delete(char *p)
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
static char *db_cfg_jsonfile_read(const char *file, char *p, int size)
{
    int n;
    FILE *fp;
    
    fp = fopen(file, "rb");
    if(fp == NULL)
        return p; // 返回传入的指针，表示文件不存在，避免内存泄漏

    n = fread(p, 1, size, fp);
    if(n <= 0)
        goto __err_close;

    p[size] = '\0';
    fclose(fp);
    return p;

__err_close:
    fclose(fp);

    return p; // 返回传入的指针，表示读取失败. 外部调用的程序已经申请了该内存了，必须原样返回才避免内存泄漏
}

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int db_cfg_json_item_base_read_u32(struct db_cfg_CellProperties_u32 *pProp, cJSON *pObject)
{
    cJSON *pItem;

    /* Value */
    pItem = cJSON_GetObjectItem(pObject, "Val"); 
    if(pItem == NULL)
        return -1;
    pProp->value = pItem->valuedouble;

    /* ValDef */
    pItem = cJSON_GetObjectItem(pObject, "ValDef"); 
    if(pItem == NULL)
        return -1;
    pProp->scope.ValDef = pItem->valuedouble;

    /* Max */
    pItem = cJSON_GetObjectItem(pObject, "Max"); 
    if(pItem == NULL)
        return -1;
    pProp->scope.max = pItem->valuedouble;

    /* Min */
    pItem = cJSON_GetObjectItem(pObject, "Min"); 
    if(pItem == NULL)
        return -1;
    pProp->scope.min = pItem->valuedouble;
/*    
    printf("read->pProp->Value %d \n",pProp->Value);
    printf("read->pProp->scope.ValDef %d \n",pProp->scope.ValDef);
    printf("read->pProp->scope.max %d \n",pProp->scope.max);
    printf("read->pProp->scope.min %d \n",pProp->scope.min);
*/    
    return 0;
}
/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int db_cfg_json_item_base_read(struct db_cfg_CellProperties_u16 *pProp, cJSON *pObject)
{
    cJSON *pItem;

    /* Value */
    pItem = cJSON_GetObjectItem(pObject, "Val"); 
    if(pItem == NULL)
        return -1;
    pProp->value = pItem->valueint;

    /* ValDef */
    pItem = cJSON_GetObjectItem(pObject, "ValDef"); 
    if(pItem == NULL)
        return -1;
    pProp->scope.ValDef = pItem->valueint;

    /* Max */
    pItem = cJSON_GetObjectItem(pObject, "Max"); 
    if(pItem == NULL)
        return -1;
    pProp->scope.max = pItem->valueint;

    /* Min */
    pItem = cJSON_GetObjectItem(pObject, "Min"); 
    if(pItem == NULL)
        return -1;
    pProp->scope.min = pItem->valueint;
/*    
    printf("read->pProp->Value %d \n",pProp->Value);
    printf("read->pProp->scope.ValDef %d \n",pProp->scope.ValDef);
    printf("read->pProp->scope.max %d \n",pProp->scope.max);
    printf("read->pProp->scope.min %d \n",pProp->scope.min);
*/    
    return 0;
}

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int db_cfg_json_item_base_read_u32_default(struct db_cfg_CellProperties_u32 *pProp, cJSON *pObject)
{
    cJSON *pItem;

    /* Value */
    pProp->value = 0;

    /* ValDef */
    pProp->scope.ValDef = 0;

    /* Max */
    pProp->scope.max = 0x7FFFFFFF;

    /* Min */
    pProp->scope.min = 0;

/*    
    printf("read->pProp->Value %d \n",pProp->Value);
    printf("read->pProp->scope.ValDef %d \n",pProp->scope.ValDef);
    printf("read->pProp->scope.max %d \n",pProp->scope.max);
    printf("read->pProp->scope.min %d \n",pProp->scope.min);
 */  
    return 0;
}
/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int db_cfg_json_item_base_read_default(struct db_cfg_CellProperties_u16 *pProp, cJSON *pObject)
{
    cJSON *pItem;

    /* Value */
    pProp->value = 0;

    /* ValDef */
    pProp->scope.ValDef = 0;

    /* Max */
    pProp->scope.max = 0x7FFF;

    /* Min */
    pProp->scope.min = 0;
/*    
    printf("read->pProp->Value %d \n",pProp->Value);
    printf("read->pProp->scope.ValDef %d \n",pProp->scope.ValDef);
    printf("read->pProp->scope.max %d \n",pProp->scope.max);
    printf("read->pProp->scope.min %d \n",pProp->scope.min);
*/    
    return 0;
}

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int db_cfg_json_group_base_read(void *pCfg, const struct db_base_mapdef *pMap, int MapSize, cJSON *pArrayItem)
{
    int         i, j;
    int         size;
    cJSON       *pObject, *pItem;
    struct db_cfg_CellProperties_u32 *pProp_u32; 
    struct db_cfg_CellProperties_u16 *pProp;    
    size = cJSON_GetArraySize(pArrayItem); 
    for(i=0; i<size; i++)
    {
        pObject = cJSON_GetArrayItem(pArrayItem, i); 
        pItem = cJSON_GetObjectItem(pObject, "name");
        if(pItem == NULL)
            return -1;

        /*
            go through the map table , find the node that match the json cfg
        */

        for(j=0; j<MapSize; j++)
        {
            if(!strcmp(pMap[j].pName, pItem->valuestring))
            {
                if(pMap[j].size == sizeof(uint32_t))
                {
                    pProp_u32 = (struct db_cfg_CellProperties_u32 *)(((char*)pCfg) + pMap[j].offset);
                    db_cfg_json_item_base_read_u32(pProp_u32, pObject);
                }
                else
                {
                    pProp = (struct db_cfg_CellProperties_u16 *)(((char*)pCfg) + pMap[j].offset);
                    db_cfg_json_item_base_read(pProp, pObject);
                }
                break;
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
static int db_cfg_json_item_base_write_u32(struct db_cfg_CellProperties_u32 *pProp, cJSON *pObject)
{
    cJSON   *pItem;

    /* Value */
    cJSON_ReplaceItemInObject(pObject, "Val", cJSON_CreateNumber(pProp->value));
    /* ValDef */
    cJSON_ReplaceItemInObject(pObject, "ValDef", cJSON_CreateNumber(pProp->scope.ValDef));
    /* Max */
    cJSON_ReplaceItemInObject(pObject, "Max", cJSON_CreateNumber(pProp->scope.max));
    /* Min */
    cJSON_ReplaceItemInObject(pObject, "Min", cJSON_CreateNumber(pProp->scope.min));

    return 0;
}

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int db_cfg_json_item_base_write(struct db_cfg_CellProperties_u16 *pProp, cJSON *pObject)
{
    cJSON   *pItem;

    /* Value */
    cJSON_ReplaceItemInObject(pObject, "Val", cJSON_CreateNumber(pProp->value));
    /* ValDef */
    cJSON_ReplaceItemInObject(pObject, "ValDef", cJSON_CreateNumber(pProp->scope.ValDef));
    /* Max */
    cJSON_ReplaceItemInObject(pObject, "Max", cJSON_CreateNumber(pProp->scope.max));
    /* Min */
    cJSON_ReplaceItemInObject(pObject, "Min", cJSON_CreateNumber(pProp->scope.min));

    return 0;
}

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int db_cfg_json_group_base_write(void *pCfg, const struct db_base_mapdef *pMap, int MapSize, cJSON *pArrayItem)
{
    int         i, j;
    int         size;
    cJSON       *pObject, *pItem;
    struct db_cfg_CellProperties_u32 *pProp_u32;
    struct db_cfg_CellProperties_u16 *pProp;
    
    size = cJSON_GetArraySize(pArrayItem); 
    for(i=0; i<size; i++)
    {
        pObject = cJSON_GetArrayItem(pArrayItem, i); 
        pItem = cJSON_GetObjectItem(pObject, "name");
        if(pItem == NULL)
            return -1;
        
        /*
            go through the map table , find the node that match the json cfg
        */
        for(j=0; j<MapSize; j++)
        {
            if(!strcmp(pMap[j].pName, pItem->valuestring))
            {
                if(pMap[j].size == sizeof(uint32_t))
                {
                    pProp_u32 = (struct db_cfg_CellProperties_u32 *)(((char*)pCfg) + pMap[j].offset);
                    db_cfg_json_item_base_write_u32(pProp_u32, pObject);
                }
                else
                {
                    pProp = (struct db_cfg_CellProperties_u16 *)(((char*)pCfg) + pMap[j].offset);
                    db_cfg_json_item_base_write(pProp, pObject);
                }

                break;
            }
        }
    }

    return 0;
}

/*************************************************************************/
/** Function Name:  db_cfg_object_u32_setdefault                       **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int db_cfg_object_u32_setdefault(struct db_cfg_CellProperties_u32 *pCfg,uint32_t val, uint32_t max, uint32_t min,uint32_t valdef)
{
    pCfg->value = val;
    pCfg->scope.max = max;
    pCfg->scope.min = min;
    pCfg->scope.ValDef = valdef;    
}



/*************************************************************************/
/** Function Name:  db_cfg_object_u32_setdefault                       **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int db_cfg_object_u16_setdefault(struct db_cfg_CellProperties_u16 *pCfg,uint16_t val, uint16_t max, uint16_t min,uint16_t valdef)
{
    pCfg->value = val;
    pCfg->scope.max = max;
    pCfg->scope.min = min;
    pCfg->scope.ValDef = valdef;    
}
/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int db_cfg_user_base_load(struct db_database_measure_base_type *pDB, const char *file)
{  
    char    *p, *pPri;
    int     size;
    int     ret;
    cJSON   *pJson, *pjsClass, *pjsGroup, *pItem;
    struct db_cfg_user_para *pCfg = &(pDB->db_cfg.user);

    /* prepare the buffer for json */
    p = db_cfg_jsonfile_alloc(file, &size);
    if(p == NULL)
        return -1;
   
    p = db_cfg_jsonfile_read(file, p, size);
    if(p == NULL)
        goto __err_free;
 
    pJson = cJSON_Parse(p);  
//    printf("%s \n",p);
    if(!pJson)
        goto __err_free;
  
    /*
        pJson("setting") --
            |-- pjsClass("adjust")
                |--pjsGroup(line)
    */

    pjsClass = cJSON_GetObjectItem(pJson, "setting"); 
    if(!pjsClass)
        goto __err_json_format;

    /* get base parameters from usercfg.json */
    pjsGroup=cJSON_GetObjectItem(pjsClass, "base");
    if(!pjsGroup)
        goto __err_json_format;

    ret = db_cfg_json_group_base_read(pCfg, &db_user_base_map[0], sizeof(db_user_base_map)/sizeof(struct db_base_mapdef), pjsGroup);
    if(ret < 0)
        goto __err_json_format;

    /* get pulseout parameters from usercfg.json */
    pjsGroup=cJSON_GetObjectItem(pjsClass, "pulse");
    if(!pjsGroup)
        goto __err_json_format;

    ret = db_cfg_json_group_base_read(pCfg, &db_user_pulse_map[0], sizeof(db_user_pulse_map)/sizeof(struct db_base_mapdef), pjsGroup);
    if(ret < 0)
        goto __err_json_format;

    /* get demand parameters from usercfg.json */
    pjsGroup=cJSON_GetObjectItem(pjsClass, "demand");
    if(!pjsGroup)
        goto __err_json_format;

    ret = db_cfg_json_group_base_read(pCfg, &db_user_demand_map[0], sizeof(db_user_demand_map)/sizeof(struct db_base_mapdef), pjsGroup);
    if(ret < 0)
        goto __err_json_format;

    /* get DEADZONE parameters from syscfg.json */
    pjsGroup=cJSON_GetObjectItem(pjsClass, "loadrun");
    if(!pjsGroup)
        goto __err_json_format;

    ret = db_cfg_json_group_base_read(pCfg, &db_user_loadrun_map[0], sizeof(db_user_loadrun_map)/sizeof(struct db_base_mapdef), pjsGroup);
    if(ret < 0)
        goto __err_json_format;

    /* get INTERFACE parameters from syscfg.json */
    pjsGroup=cJSON_GetObjectItem(pjsClass, "refer");
    if(!pjsGroup)
        goto __err_json_format;

    ret = db_cfg_json_group_base_read(pCfg, &db_user_refer_map[0], sizeof(db_user_refer_map)/sizeof(struct db_base_mapdef), pjsGroup);
    if(ret < 0)
        goto __err_json_format;

    /* get INTERFACE parameters from syscfg.json */
    pjsGroup=cJSON_GetObjectItem(pjsClass, "network");
    if(!pjsGroup)
        goto __err_json_format;
    ret = db_cfg_json_group_base_read(pCfg, &db_user_network_map[0], sizeof(db_user_network_map)/sizeof(struct db_base_mapdef), pjsGroup);
    if(ret < 0)
        goto __err_json_format;

    /* runing energy load*/
    pjsGroup=cJSON_GetObjectItem(pjsClass, "runing_energy"); 
    if(!pjsGroup)
    {
        db_cfg_object_u32_setdefault(&pCfg->runing.setPtot,0,99999999,0,0);
        db_cfg_object_u32_setdefault(&pCfg->runing.setQtot,0,99999999,0,0);  
        db_cfg_object_u32_setdefault(&pCfg->runing.setStot,0,99999999,0,0);    
    }
    if(NULL != pjsGroup)
    {
        ret = db_cfg_json_group_base_read(pCfg, &db_user_runing_energy_set_map[0], sizeof(db_user_runing_energy_set_map)/sizeof(struct db_base_mapdef), pjsGroup);
    }

//    printf("%s \n",p);

    cJSON_Delete(pJson);
    db_cfg_jsonfile_delete(p);

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
int __db_cfg_user_base_update(struct db_database_measure_base_type *pDB, const char *file, const char *default_file)
{
    char    *p, *pPri;
    int     size;
    int     ret;
    cJSON   *pJson, *pjsClass, *pjsGroup, *pItem;
    struct db_cfg_user_para *pCfg = &(pDB->db_cfg.user);

    /* prepare the buffer for json */
    /*
        use the default file for template.
        because the default file not be write aften, it is more save.
        this function will change all the parameters that cunstomer set,
        
    */
    p = db_cfg_jsonfile_alloc(default_file, &size);
    if(p == NULL)
        return -1;
   
    p = db_cfg_jsonfile_read(default_file, p, size);
    if(p == NULL)
        goto __err_free;
 
    pJson = cJSON_Parse(p);  
    if(!pJson)
    {
        printf("Json file[%s] error\n", file);
        goto __err_free;
    }
  
    /*
        pJson("setting") --
            |-- pjsClass("adjust")
                |--pjsGroup(line)
    */

    pjsClass = cJSON_GetObjectItem(pJson, "setting"); 
    if(!pjsClass)
        goto __err_json_format;

    /* get base parameters from usercfg.json */
    pjsGroup=cJSON_GetObjectItem(pjsClass, "base");
    if(!pjsGroup)
        goto __err_json_format;

    ret = db_cfg_json_group_base_write(pCfg, &db_user_base_map[0], sizeof(db_user_base_map)/sizeof(struct db_base_mapdef), pjsGroup);
    if(ret < 0)
        goto __err_json_format;

    /* get pulse parameters from usercfg.json */
    pjsGroup=cJSON_GetObjectItem(pjsClass, "pulse");
    if(!pjsGroup)
        goto __err_json_format;

    ret = db_cfg_json_group_base_write(pCfg, &db_user_pulse_map[0], sizeof(db_user_pulse_map)/sizeof(struct db_base_mapdef), pjsGroup);
    if(ret < 0)
        goto __err_json_format;

    /* get demand parameters from usercfg.json */
    pjsGroup=cJSON_GetObjectItem(pjsClass, "demand");
    if(!pjsGroup)
        goto __err_json_format;

    ret = db_cfg_json_group_base_write(pCfg, &db_user_demand_map[0], sizeof(db_user_demand_map)/sizeof(struct db_base_mapdef), pjsGroup);
    if(ret < 0)
        goto __err_json_format;

    /* get DEADZONE parameters from syscfg.json */
    pjsGroup=cJSON_GetObjectItem(pjsClass, "loadrun");
    if(!pjsGroup)
        goto __err_json_format;

    ret = db_cfg_json_group_base_write(pCfg, &db_user_loadrun_map[0], sizeof(db_user_loadrun_map)/sizeof(struct db_base_mapdef), pjsGroup);
    if(ret < 0)
        goto __err_json_format;

    /* get INTERFACE parameters from syscfg.json */
    pjsGroup=cJSON_GetObjectItem(pjsClass, "refer");
    if(!pjsGroup)
        goto __err_json_format;

    ret = db_cfg_json_group_base_write(pCfg, &db_user_refer_map[0], sizeof(db_user_refer_map)/sizeof(struct db_base_mapdef), pjsGroup);
    if(ret < 0)
        goto __err_json_format;

    /* get INTERFACE parameters from syscfg.json */
    pjsGroup=cJSON_GetObjectItem(pjsClass, "network");
    if(!pjsGroup)
        goto __err_json_format;

    ret = db_cfg_json_group_base_write(pCfg, &db_user_network_map[0], sizeof(db_user_network_map)/sizeof(struct db_base_mapdef), pjsGroup);
    if(ret < 0)
        goto __err_json_format;

    pjsGroup=cJSON_GetObjectItem(pjsClass, "runing_energy");
    if(!pjsGroup)
        goto __err_json_format;

    ret = db_cfg_json_group_base_write(pCfg, &db_user_runing_energy_set_map[0], sizeof(db_user_runing_energy_set_map)/sizeof(struct db_base_mapdef), pjsGroup);
    if(ret < 0)
        goto __err_json_format;

    /*  */
    pPri = cJSON_Print(pJson);
//    printf("%s \n",pPri);
    /* recovery the json */
    db_cfg_json_writetofile(file, pPri);
    /* free the buffer */
    free(pPri);

    cJSON_Delete(pJson);
    db_cfg_jsonfile_delete(p);

    return 0;

__err_json_format:
    printf("Json file[%s] format error\n", file);
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
void db_cfg_user_base_default(struct db_database_measure_base_type *pDB, uint16_t chnlno)
{
    const char *file = db_cfg_get_user_base_file(chnlno);
    const char *file_backup = db_cfg_get_user_base_file_backup(chnlno);
    const char *file_default = db_cfg_get_user_base_file_default(chnlno);

    if(!file || !file_backup || !file_default)
    {
        printf("db_cfg_user_base_default: invalid channel %d\n", chnlno);
        return;
    }

    unlink(file);
    unlink(file_backup);
    db_file_copy(file_default, file);
    db_file_copy(file_default, file_backup);
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int db_cfg_user_base_update(struct db_database_measure_base_type *pDB, uint16_t chnlno)
{
    int ret = 0;
    char file[256], file_backup[256], file_default[256];
    const char *file_ptr;
    
    /* 每次调用后立即复制，避免静态缓冲区被后续调用覆盖 */
    file_ptr = db_cfg_get_user_base_file(chnlno);
    if(!file_ptr)
    {
        printf("db_cfg_user_base_update: invalid channel %d\n", chnlno);
        return -1;
    }
    snprintf(file, sizeof(file), "%s", file_ptr);
    
    file_ptr = db_cfg_get_user_base_file_backup(chnlno);
    if(!file_ptr)
    {
        printf("db_cfg_user_base_update: invalid channel %d\n", chnlno);
        return -1;
    }
    snprintf(file_backup, sizeof(file_backup), "%s", file_ptr);
    
    file_ptr = db_cfg_get_user_base_file_default(chnlno);
    if(!file_ptr)
    {
        printf("db_cfg_user_base_update: invalid channel %d\n", chnlno);
        return -1;
    }
    snprintf(file_default, sizeof(file_default), "%s", file_ptr);


    ret = __db_cfg_user_base_update(pDB, file, file_default);
    ret = __db_cfg_user_base_update(pDB, file_backup, file_default);
    
    return ret;
}

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int db_cfg_user_base_init(struct db_database_measure_base_type *pDB, uint16_t chnlno)
{
    int ret;
    char file[256], file_backup[256];
    const char *file_ptr;
    
    /* 每次调用后立即复制，避免静态缓冲区被后续调用覆盖 */
    file_ptr = db_cfg_get_user_base_file(chnlno);
    if(!file_ptr)
    {
        printf("db_cfg_user_base_init: invalid channel %d\n", chnlno);
        return -1;
    }
    snprintf(file, sizeof(file), "%s", file_ptr);
    
    file_ptr = db_cfg_get_user_base_file_backup(chnlno);
    if(!file_ptr)
    {
        printf("db_cfg_user_base_init: invalid channel %d\n", chnlno);
        return -1;
    }
    snprintf(file_backup, sizeof(file_backup), "%s", file_ptr);

    ret = db_cfg_user_base_load(pDB, file);
    if(ret < 0)
        ret = db_cfg_user_base_load(pDB, file_backup);

    return ret;
}



/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
__inline struct db_cfg_user_para *db_cfg_get_user(struct db_database_measure_base_type *pDB)
{    
    return &pDB->db_cfg.user;
}

