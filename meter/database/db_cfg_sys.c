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

#include "db_cfg_sys.h"
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
    SYS CFG MAP：

*/
#define DB_MAP_TABLE_ADJUST(x)                  DB_toString(x),(offsetof(struct db_cfg_sys_adjust_para,x)+offsetof(struct db_cfg_sys_para,adjust))
#define DB_MAP_TABLE_DEADZONE(x)                DB_toString(x),(offsetof(struct db_cfg_sys_deadzone_para,x)+offsetof(struct db_cfg_sys_para,deadzone))
#define DB_MAP_TABLE_INTERFACE(x)               DB_toString(x),(offsetof(struct db_cfg_sys_interface_para,x)+offsetof(struct db_cfg_sys_para,interface))
#define DB_MAP_TABLE_ADJUST_VALUE(x)            DB_toString(x),(offsetof(struct db_cfg_sys_adjust_value_type,x)+offsetof(struct db_cfg_sys_para,adjustvalue))

#define DB_MAP_TABLE_ADJUST_AD_PARA(x)          DB_toString(x),(offsetof(struct db_cfg_sys_adjust_ad_para,x)+offsetof(struct db_cfg_sys_para,adjustAd))
#define DB_MAP_TABLE_ADJUST_AD_VALUE(x)         DB_toString(x),(offsetof(struct db_cfg_sys_adjust_ad_value_para,x)+offsetof(struct db_cfg_sys_para,adjustAdvalue))
#define DB_MAP_TABLE_ADJUST_PRTH(x)             DB_toString(x),(offsetof(struct db_cfg_sys_adjust_prth_para,x)+offsetof(struct db_cfg_sys_para,prthvalue))
#define DB_MAP_TABLE_ADJUST_I0(x)               DB_toString(x),(offsetof(struct db_cfg_sys_adjust_I0_para,x)+offsetof(struct db_cfg_sys_para,adjustI0value))


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
  SYS CFG MAP STRUCT START
*/
static const struct db_base_mapdef db_sys_adjust_map[] = 
{
    //DB_MAP_TABLE_ADJUST(HfConst1),  =>  {"HfConst1", 404}
    { DB_MAP_TABLE_ADJUST(HfConst1 ), sizeof(uint16_t)},
    { DB_MAP_TABLE_ADJUST(HfConst2 ), sizeof(uint16_t)},
    { DB_MAP_TABLE_ADJUST(IStart_PS), sizeof(uint16_t)},
    { DB_MAP_TABLE_ADJUST(IStart_Q ), sizeof(uint16_t)},
    { DB_MAP_TABLE_ADJUST(GsUa     ), sizeof(uint16_t)},
    { DB_MAP_TABLE_ADJUST(GsUb     ), sizeof(uint16_t)},
    { DB_MAP_TABLE_ADJUST(GsUc     ), sizeof(uint16_t)},
    { DB_MAP_TABLE_ADJUST(GsIa     ), sizeof(uint16_t)},
    { DB_MAP_TABLE_ADJUST(GsIb     ), sizeof(uint16_t)},
    { DB_MAP_TABLE_ADJUST(GsIc     ), sizeof(uint16_t)},
    { DB_MAP_TABLE_ADJUST(GsIn     ), sizeof(uint16_t)},

    { DB_MAP_TABLE_ADJUST(Phs_Ua   ), sizeof(uint32_t)},
    { DB_MAP_TABLE_ADJUST(Phs_Ub   ), sizeof(uint32_t)},
    { DB_MAP_TABLE_ADJUST(Phs_Uc   ), sizeof(uint32_t)},
    { DB_MAP_TABLE_ADJUST(Phs_Ia   ), sizeof(uint32_t)},
    { DB_MAP_TABLE_ADJUST(Phs_Ib   ), sizeof(uint32_t)},
    { DB_MAP_TABLE_ADJUST(Phs_Ic   ), sizeof(uint32_t)},
    { DB_MAP_TABLE_ADJUST(Phs_In   ), sizeof(uint32_t)},

    { DB_MAP_TABLE_ADJUST(Pa_PhsL  ), sizeof(uint16_t)},
    { DB_MAP_TABLE_ADJUST(Pb_PhsL  ), sizeof(uint16_t)},
    { DB_MAP_TABLE_ADJUST(Pc_PhsL  ), sizeof(uint16_t)},
    { DB_MAP_TABLE_ADJUST(Qa_PhsL  ), sizeof(uint16_t)},
    { DB_MAP_TABLE_ADJUST(Qb_PhsL  ), sizeof(uint16_t)},
    { DB_MAP_TABLE_ADJUST(Qc_PhsL  ), sizeof(uint16_t)},
    { DB_MAP_TABLE_ADJUST(ChkSum   ), sizeof(uint32_t)},
    { DB_MAP_TABLE_ADJUST(I0Adj    ), sizeof(uint16_t)},
    { DB_MAP_TABLE_ADJUST(T0Adj    ), sizeof(uint16_t)},
    { DB_MAP_TABLE_ADJUST(T1Adj    ), sizeof(uint16_t)},
    { DB_MAP_TABLE_ADJUST(T2Adj    ), sizeof(uint16_t)},
    { DB_MAP_TABLE_ADJUST(T3Adj    ), sizeof(uint16_t)},
    { DB_MAP_TABLE_ADJUST(Offset_Ia ), sizeof(uint16_t)},
    { DB_MAP_TABLE_ADJUST(Offset_Ib ), sizeof(uint16_t)},
    { DB_MAP_TABLE_ADJUST(Offset_Ic ), sizeof(uint16_t)},
};



static const struct db_base_mapdef db_sys_deadzone_map[] = 
{
    { DB_MAP_TABLE_DEADZONE(Ua), sizeof(uint16_t)},
    { DB_MAP_TABLE_DEADZONE(Ub), sizeof(uint16_t)},
    { DB_MAP_TABLE_DEADZONE(Uc), sizeof(uint16_t)},
    { DB_MAP_TABLE_DEADZONE(Ia), sizeof(uint16_t)},
    { DB_MAP_TABLE_DEADZONE(Ib), sizeof(uint16_t)},
    { DB_MAP_TABLE_DEADZONE(Ic), sizeof(uint16_t)},
    { DB_MAP_TABLE_DEADZONE(In), sizeof(uint16_t)},
    { DB_MAP_TABLE_DEADZONE(Pa), sizeof(uint16_t)},
    { DB_MAP_TABLE_DEADZONE(Pb), sizeof(uint16_t)},
    { DB_MAP_TABLE_DEADZONE(Pc), sizeof(uint16_t)},
    { DB_MAP_TABLE_DEADZONE(Qa), sizeof(uint16_t)},
    { DB_MAP_TABLE_DEADZONE(Qb), sizeof(uint16_t)},
    { DB_MAP_TABLE_DEADZONE(Qc), sizeof(uint16_t)},
    { DB_MAP_TABLE_DEADZONE(Sa), sizeof(uint16_t)},
    { DB_MAP_TABLE_DEADZONE(Sb), sizeof(uint16_t)},
    { DB_MAP_TABLE_DEADZONE(Sc), sizeof(uint16_t)},
    { DB_MAP_TABLE_DEADZONE(Uab), sizeof(uint16_t)},
    { DB_MAP_TABLE_DEADZONE(Ubc), sizeof(uint16_t)},
    { DB_MAP_TABLE_DEADZONE(Uca), sizeof(uint16_t)},
};


static const struct db_base_mapdef db_sys_interface_map[] = 
{
    { DB_MAP_TABLE_INTERFACE(RatedVoltage )    , sizeof(uint16_t)},
    { DB_MAP_TABLE_INTERFACE(RatedCurrent )    , sizeof(uint16_t)},
    { DB_MAP_TABLE_INTERFACE(RatedFrequency)   , sizeof(uint16_t)},
    { DB_MAP_TABLE_INTERFACE(EnergyMax)        , sizeof(uint64_t)},
};

static const struct db_base_mapdef db_sys_adjust_value_map[] = 
{
    { DB_MAP_TABLE_ADJUST_VALUE(Adjust_Va              ),  sizeof(uint16_t)},
    { DB_MAP_TABLE_ADJUST_VALUE(Adjust_Vb              ),  sizeof(uint16_t)},
    { DB_MAP_TABLE_ADJUST_VALUE(Adjust_Vc              ),  sizeof(uint16_t)},
    { DB_MAP_TABLE_ADJUST_VALUE(Adjust_Ia              ),  sizeof(uint16_t)},
    { DB_MAP_TABLE_ADJUST_VALUE(Adjust_Ib              ),  sizeof(uint16_t)},
    { DB_MAP_TABLE_ADJUST_VALUE(Adjust_Ic              ),  sizeof(uint16_t)},
    { DB_MAP_TABLE_ADJUST_VALUE(Adjust_Pfa             ),  sizeof(uint16_t)},
    { DB_MAP_TABLE_ADJUST_VALUE(Adjust_Pfb             ),  sizeof(uint16_t)},
    { DB_MAP_TABLE_ADJUST_VALUE(Adjust_Pfc             ),  sizeof(uint16_t)},
};

static const struct db_base_mapdef db_sys_adjust_ad_para_map[] = 
{
    { DB_MAP_TABLE_ADJUST_AD_PARA(Adjust_Gv              ),  sizeof(uint16_t)},
    { DB_MAP_TABLE_ADJUST_AD_PARA(Adjust_Gi              ),  sizeof(uint16_t)},
    { DB_MAP_TABLE_ADJUST_AD_PARA(Krms_V              ),  sizeof(uint16_t)},
    { DB_MAP_TABLE_ADJUST_AD_PARA(Krms_I              ),  sizeof(uint16_t)},
    { DB_MAP_TABLE_ADJUST_AD_PARA(Krms_P              ),  sizeof(uint16_t)},
    { DB_MAP_TABLE_ADJUST_AD_PARA(Krms_Pf              ),  sizeof(uint16_t)},
    { DB_MAP_TABLE_ADJUST_AD_PARA(Krms_Hz              ),  sizeof(uint16_t)},
    { DB_MAP_TABLE_ADJUST_AD_PARA(Krms_E              ),  sizeof(uint16_t)},
};

static const struct db_base_mapdef db_sys_adjust_ad_value_map[] = 
{
    { DB_MAP_TABLE_ADJUST_AD_VALUE(Adjust_V              ),  sizeof(uint16_t)},
    { DB_MAP_TABLE_ADJUST_AD_VALUE(Adjust_I              ),  sizeof(uint16_t)},
    { DB_MAP_TABLE_ADJUST_AD_VALUE(Adjust_P              ),  sizeof(uint16_t)},
    { DB_MAP_TABLE_ADJUST_AD_VALUE(Adjust_A              ),  sizeof(uint16_t)},
    { DB_MAP_TABLE_ADJUST_AD_VALUE(Adjust_Va             ),  sizeof(uint32_t)},
    { DB_MAP_TABLE_ADJUST_AD_VALUE(Adjust_Vb             ),  sizeof(uint32_t)},
    { DB_MAP_TABLE_ADJUST_AD_VALUE(Adjust_Vc             ),  sizeof(uint32_t)},
    { DB_MAP_TABLE_ADJUST_AD_VALUE(Adjust_Ia             ),  sizeof(uint32_t)},
    { DB_MAP_TABLE_ADJUST_AD_VALUE(Adjust_Ib             ),  sizeof(uint32_t)},
    { DB_MAP_TABLE_ADJUST_AD_VALUE(Adjust_Ic             ),  sizeof(uint32_t)},
    { DB_MAP_TABLE_ADJUST_AD_VALUE(Adjust_Pa             ),  sizeof(uint32_t)},
    { DB_MAP_TABLE_ADJUST_AD_VALUE(Adjust_Pb             ),  sizeof(uint32_t)},
    { DB_MAP_TABLE_ADJUST_AD_VALUE(Adjust_Pc             ),  sizeof(uint32_t)},
};

static const struct db_base_mapdef db_sys_adjust_prth_map[] = 
{
    { DB_MAP_TABLE_ADJUST_PRTH(Prth1L              ),  sizeof(uint16_t)},
    { DB_MAP_TABLE_ADJUST_PRTH(Prth1H              ),  sizeof(uint16_t)},
    { DB_MAP_TABLE_ADJUST_PRTH(Prth2L              ),  sizeof(uint16_t)},
    { DB_MAP_TABLE_ADJUST_PRTH(Prth2H              ),  sizeof(uint16_t)},
    { DB_MAP_TABLE_ADJUST_PRTH(PhsxR1              ),  sizeof(uint16_t)},
    { DB_MAP_TABLE_ADJUST_PRTH(PhsxR2              ),  sizeof(uint16_t)},
    { DB_MAP_TABLE_ADJUST_PRTH(PhsxR3              ),  sizeof(uint16_t)},
};

static const struct db_base_mapdef db_sys_adjust_I0_map[] = 
{
    { DB_MAP_TABLE_ADJUST_I0(RatedI0              ),  sizeof(uint16_t)},
    { DB_MAP_TABLE_ADJUST_I0(Adjust_Gi0              ),  sizeof(uint16_t)},
    { DB_MAP_TABLE_ADJUST_I0(TypeI0              ),  sizeof(uint16_t)},
    { DB_MAP_TABLE_ADJUST_I0(Adjust_I0              ),  sizeof(uint16_t)},
    { DB_MAP_TABLE_ADJUST_I0(Adjust_I0_value              ),  sizeof(uint32_t)},
};  


/**********************************************************************************************************************
    新增参数的默认值区。
    为了兼容旧的设备（旧设备的配置json没有这些参数），所以需要单独设置这些参数的默认值
    1. adjustAd： AD校准参数
    2. adjustAdvalue： AD校准值
    3. prthvalue： 相位补偿参数
    4. adjustI0value： 漏电电流校准值

    默认值设置如下：
    1. adjustAd： AD校准参数
    2. adjustAdvalue： AD校准值
    3. prthvalue： 相位补偿参数
    4. adjustI0value： 漏电电流校准值
*/

static const uint32_t phs_default[] = {
    0xA7000200, // Phs_Ua
    0xA7000200, // Phs_Ub
    0xA7000200, // Phs_Uc
    0x20080200, // Phs_Ia
    0x20080200, // Phs_Ib
    0x20080200, // Phs_Ic
    0x20080200, // Phs_In
};

/* adjustAd： AD校准参数 */
static const struct db_cfg_sys_adjust_ad_para db_cfg_sys_adjust_ad_para_default = {
    .Adjust_Gv              = {1},
    .Adjust_Gi              = {1},
    .Krms_V                 = {100},
    .Krms_I                 = {1000},
    .Krms_P                 = {10},    
    .Krms_Pf                = {1000},
    .Krms_Hz                = {10000},
    .Krms_E                 = {1000},  
};
/* adjustAdvalue： AD校准值 */
static const struct db_cfg_sys_adjust_ad_value_para db_cfg_sys_adjust_ad_value_para_default = {
    .Adjust_V               = {220},
    .Adjust_I               = {5},
    .Adjust_P               = {1100},
    .Adjust_A               = {60},
    .Adjust_Va              = {22000000},
    .Adjust_Vb              = {22000000}, 
    .Adjust_Vc              = {22000000},
    .Adjust_Ia              = {2500000},
    .Adjust_Ib              = {2500000},
    .Adjust_Ic              = {2500000},
    .Adjust_Pa              = {6556511},
    .Adjust_Pb              = {6556511},
    .Adjust_Pc              = {6556511},
};

/* prthvalue： 相位补偿参数 */
static const struct db_cfg_sys_adjust_prth_para db_cfg_sys_adjust_prth_para_default = {
    .Prth1L              = {25},
    .Prth1H              = {40},
    .Prth2L              = {80},
    .Prth2H              = {100},
    .PhsxR1              = {20},
    .PhsxR2              = {0}, 
    .PhsxR3              = {0xFFE2},
};

/* adjustI0value： 漏电电流校准值 */
static const struct db_cfg_sys_adjust_I0_para db_cfg_sys_adjust_I0_para_default = {
    .RatedI0             = {5},
    .Adjust_Gi0          = {2},
    .TypeI0              = {0},
    .Adjust_I0           = {5},
    .Adjust_I0_value     = {12000000},
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
    printf("read->pProp->value %d \n",pProp->value);
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
    printf("read->pProp->value %d \n",pProp->value);
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
    printf("read->pProp->value %d \n",pProp->value);
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
    printf("read->pProp->value %d \n",pProp->value);
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
/*************************************************************************/
/** Function Name: db_cfg_json_item_base_read_u64                      **/    
/** Purpose: 读取u64类型参数（如EnergyMax），cJSON数值经double中转      **/    
/*************************************************************************/
static int db_cfg_json_item_base_read_u64(struct db_cfg_CellProperties_u64 *pProp, cJSON *pObject)
{
    cJSON *pItem;

    /* Value */
    pItem = cJSON_GetObjectItem(pObject, "Val"); 
    if(pItem == NULL)
        return -1;
    pProp->value = (uint64_t)pItem->valuedouble;

    /* ValDef */
    pItem = cJSON_GetObjectItem(pObject, "ValDef"); 
    if(pItem == NULL)
        return -1;
    pProp->scope.ValDef = (uint64_t)pItem->valuedouble;

    /* Max */
    pItem = cJSON_GetObjectItem(pObject, "Max"); 
    if(pItem == NULL)
        return -1;
    pProp->scope.max = (uint64_t)pItem->valuedouble;

    /* Min */
    pItem = cJSON_GetObjectItem(pObject, "Min"); 
    if(pItem == NULL)
        return -1;
    pProp->scope.min = (uint64_t)pItem->valuedouble;

    return 0;
}

/*************************************************************************/
/** Function Name: db_cfg_json_item_base_write_u64                     **/    
/** Purpose: 回写u64类型参数（如EnergyMax）                             **/    
/*************************************************************************/
static int db_cfg_json_item_base_write_u64(struct db_cfg_CellProperties_u64 *pProp, cJSON *pObject)
{
    /* Value */
    cJSON_ReplaceItemInObject(pObject, "Val", cJSON_CreateNumber((double)pProp->value));
    /* ValDef */
    cJSON_ReplaceItemInObject(pObject, "ValDef", cJSON_CreateNumber((double)pProp->scope.ValDef));
    /* Max */
    cJSON_ReplaceItemInObject(pObject, "Max", cJSON_CreateNumber((double)pProp->scope.max));
    /* Min */
    cJSON_ReplaceItemInObject(pObject, "Min", cJSON_CreateNumber((double)pProp->scope.min));

    return 0;
}

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
//            printf("%s %s\n",pMap[j].pName ,pItem->valuestring);
            if(!strcmp(pMap[j].pName, pItem->valuestring))
            {
                if(pMap[j].size == sizeof(uint64_t))
                {
                    struct db_cfg_CellProperties_u64 *pProp_u64;
                    pProp_u64 = (struct db_cfg_CellProperties_u64 *)(((char*)pCfg) + pMap[j].offset);
                    db_cfg_json_item_base_read_u64(pProp_u64, pObject);

                    printf("name:%s, value:%llu\n", pMap[j].pName, (unsigned long long)pProp_u64->value);
                }
                else if(pMap[j].size == sizeof(uint32_t))
                {
                    pProp_u32 = (struct db_cfg_CellProperties_u32 *)(((char*)pCfg) + pMap[j].offset);
                    db_cfg_json_item_base_read_u32(pProp_u32, pObject);

                    printf("name:%s, value:%d\n", pMap[j].pName, pProp_u32->value);
                }
                else
                {
                    pProp = (struct db_cfg_CellProperties_u16 *)(((char*)pCfg) + pMap[j].offset);
                    db_cfg_json_item_base_read(pProp, pObject);

                    printf("name:%s, value:%d\n", pMap[j].pName, pProp->value);
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
static int db_cfg_json_group_base_read_deadzone(void *pCfg, const struct db_base_mapdef *pMap, int MapSize, cJSON *pArrayItem)
{
    int         i, j;
    int         size;
    cJSON       *pObject, *pItem;
    struct db_cfg_CellProperties_u32 *pProp_u32; 
    struct db_cfg_CellProperties_u16 *pProp;    
    char str[32];

    size = cJSON_GetArraySize(pArrayItem); 
    for(i=0; i<size; i++)
    {
        pObject = cJSON_GetArrayItem(pArrayItem, i); 
        pItem = cJSON_GetObjectItem(pObject, "name");
        if(pItem == NULL)
            return -1;

        /*为了解决电压死区配置文件有Va/Vb/Vc/Vab/Vbc/Vca,Ua/Ub/Uc/Uab/Ubc/Uca的情况，程序中统一使用Ua/Ub/Uc/Uab/Ubc/Uca*/
        strcpy(str,pItem->valuestring);
        if( pItem->valuestring[0] == 'V')
        {
            str[0] = 'U';
        }

        /*
            go through the map table , find the node that match the json cfg
        */

        for(j=0; j<MapSize; j++)
        {
//            printf("%s %s\n",pMap[j].pName ,pItem->valuestring);
            if(!strcmp(pMap[j].pName, str))
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
                if(pMap[j].size == sizeof(uint64_t))
                {
                    struct db_cfg_CellProperties_u64 *pProp_u64;
                    pProp_u64 = (struct db_cfg_CellProperties_u64 *)(((char*)pCfg) + pMap[j].offset);
                    db_cfg_json_item_base_write_u64(pProp_u64, pObject);
                }
                else if(pMap[j].size == sizeof(uint32_t))
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
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int db_cfg_json_group_base_write_deadzone(void *pCfg, const struct db_base_mapdef *pMap, int MapSize, cJSON *pArrayItem)
{
    int         i, j;
    int         size;
    cJSON       *pObject, *pItem;
    struct db_cfg_CellProperties_u32 *pProp_u32;
    struct db_cfg_CellProperties_u16 *pProp;
    char str[32];
    
    size = cJSON_GetArraySize(pArrayItem); 
    for(i=0; i<size; i++)
    {
        pObject = cJSON_GetArrayItem(pArrayItem, i); 
        pItem = cJSON_GetObjectItem(pObject, "name");
        if(pItem == NULL)
            return -1;
        
        /*为了解决电压死区配置文件有Va/Vb/Vc/Vab/Vbc/Vca,Ua/Ub/Uc/Uab/Ubc/Uca的情况，程序中统一使用Ua/Ub/Uc/Uab/Ubc/Uca*/
        strcpy(str,pItem->valuestring);
        if( pItem->valuestring[0] == 'V')
        {
            str[0] = 'U';
        }

        /*
            go through the map table , find the node that match the json cfg
        */
        for(j=0; j<MapSize; j++)
        {
            if(!strcmp(pMap[j].pName, str))
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
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int db_cfg_sys_load(struct db_database_measure_base_type *pDB, const char *file, uint16_t chnlno)
{
    char    *p;
    int     size;
    int     ret;
    cJSON   *pJson, *pjsClass, *pjsGroup;
    struct db_cfg_sys_para *pSysCfg = &pDB->db_cfg.sys;
 
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
  
    /*
        pJson("setting") --
            |-- pjsClass("adjust")
                |--pjsGroup(line)
    */

    pjsClass = cJSON_GetObjectItem(pJson, "setting"); 
    if(!pjsClass)
        goto __err_json_format;
 
    /* get adjust parameters from syscfg.json */
    pjsGroup=cJSON_GetObjectItem(pjsClass, "adjust");
    if(!pjsGroup)
        goto __err_json_format;

    ret = db_cfg_json_group_base_read(pSysCfg, &db_sys_adjust_map[0], sizeof(db_sys_adjust_map)/sizeof(struct db_base_mapdef), pjsGroup);
    if(ret < 0)
        goto __err_json_format;

    /* check Phs_Ua, Phs_Ub, Phs_Uc, Phs_Ia, Phs_Ib, Phs_Ic, Phs_In */
    /*
        add by chenzy 2025 11 07
        新增的相位校正功能如果在旧设备上没有配置，则需要使用默认值
    */
    // if(pSysCfg->adjust.Phs_Ua.value == 0)
    // {
    //     pSysCfg->adjust.Phs_Ua.value = phs_default[0];
    //     pSysCfg->adjust.Phs_Ub.value = phs_default[1];
    //     pSysCfg->adjust.Phs_Uc.value = phs_default[2];
    //     pSysCfg->adjust.Phs_Ia.value = phs_default[3];
    //     pSysCfg->adjust.Phs_Ib.value = phs_default[4];
    //     pSysCfg->adjust.Phs_Ic.value = phs_default[5];
    //     pSysCfg->adjust.Phs_In.value = phs_default[6];
    // }


    /* get DEADZONE parameters from syscfg.json */
    pjsGroup=cJSON_GetObjectItem(pjsClass, "deadzone");
    if(!pjsGroup)
        goto __err_json_format;

    ret = db_cfg_json_group_base_read_deadzone(pSysCfg, &db_sys_deadzone_map[0], sizeof(db_sys_deadzone_map)/sizeof(struct db_base_mapdef), pjsGroup);
    if(ret < 0)
        goto __err_json_format;

    /* get INTERFACE parameters from syscfg.json */
    pjsGroup=cJSON_GetObjectItem(pjsClass, "interface");
    if(!pjsGroup)
        goto __err_json_format;

    ret = db_cfg_json_group_base_read(pSysCfg, &db_sys_interface_map[0], sizeof(db_sys_interface_map)/sizeof(struct db_base_mapdef), pjsGroup);
    if(ret < 0)
        goto __err_json_format;

    /* get INTERFACE parameters from syscfg.json */
    pjsGroup=cJSON_GetObjectItem(pjsClass, "adjustvalue");
    if(!pjsGroup)
        goto __err_json_format;

    ret = db_cfg_json_group_base_read(pSysCfg, &db_sys_adjust_value_map[0], sizeof(db_sys_adjust_value_map)/sizeof(struct db_base_mapdef), pjsGroup);
    if(ret < 0)
        goto __err_json_format;


    /*
        以下为新增的参数，用于支持AD校准和相位补偿
        注意： 为了兼容旧的设备（旧设备的配置json没有这些参数），所以需要单独读取这些参数
        1. adjustAd： AD校准参数
        2. adjustAdvalue： AD校准值
        3. prthvalue： 相位补偿参数
        4. adjustI0value： 漏电电流校准值

        兼容处理时，需要处理以下情况：
        1. 旧设备没有这些参数，所以需要默认值
        2. 旧设备有这些参数，但是值为0，所以需要默认值
    */    
    pjsGroup=cJSON_GetObjectItem(pjsClass, "adjustAd");
    if(!pjsGroup)
        memcpy(&pSysCfg->adjustAd, &db_cfg_sys_adjust_ad_para_default, sizeof(db_cfg_sys_adjust_ad_para_default));
    else
    {
        ret = db_cfg_json_group_base_read(pSysCfg, &db_sys_adjust_ad_para_map[0], sizeof(db_sys_adjust_ad_para_map)/sizeof(struct db_base_mapdef), pjsGroup);
        if(ret < 0)
            goto __err_json_format;
    } 
        
    pjsGroup=cJSON_GetObjectItem(pjsClass, "adjustAdvalue");
    if(!pjsGroup)
        memcpy(&pSysCfg->adjustAdvalue, &db_cfg_sys_adjust_ad_value_para_default, sizeof(db_cfg_sys_adjust_ad_value_para_default));
    else
    {
        ret = db_cfg_json_group_base_read(pSysCfg, &db_sys_adjust_ad_value_map[0], sizeof(db_sys_adjust_ad_value_map)/sizeof(struct db_base_mapdef), pjsGroup);
        if(ret < 0)
            goto __err_json_format;
    }

    pjsGroup=cJSON_GetObjectItem(pjsClass, "prthvalue");
    if(!pjsGroup)
        memcpy(&pSysCfg->prthvalue, &db_cfg_sys_adjust_prth_para_default, sizeof(db_cfg_sys_adjust_prth_para_default));
    else
    {
        ret = db_cfg_json_group_base_read(pSysCfg, &db_sys_adjust_prth_map[0], sizeof(db_sys_adjust_prth_map)/sizeof(struct db_base_mapdef), pjsGroup);
        if(ret < 0)
            goto __err_json_format;
    }
    
    pjsGroup=cJSON_GetObjectItem(pjsClass, "adjustI0value");
    if(!pjsGroup)
        memcpy(&pSysCfg->adjustI0value, &db_cfg_sys_adjust_I0_para_default, sizeof(db_cfg_sys_adjust_I0_para_default));
    else
    {
        ret = db_cfg_json_group_base_read(pSysCfg, &db_sys_adjust_I0_map[0], sizeof(db_sys_adjust_I0_map)/sizeof(struct db_base_mapdef), pjsGroup);
        if(ret < 0)
            goto __err_json_format;
    }

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
/** Function Name:db_cfg_sys_default                                   **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
void db_cfg_sys_default(struct db_database_measure_base_type *pDB, uint16_t chnlno)
{
    const char *file = db_cfg_get_sys_file(chnlno);
    const char *file_backup = db_cfg_get_sys_file_backup(chnlno);
    const char *file_default = db_cfg_get_sys_file_default(chnlno);

    if(!file || !file_backup || !file_default)
    {
        printf("db_cfg_sys_default: invalid channel %d\n", chnlno);
        return;
    }

    unlink(file);
    unlink(file_backup);
    db_file_copy(file_default, file);
    db_file_copy(file_default, file_backup);

    db_cfg_sys_load(pDB, file, chnlno);
    return ;
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int __db_cfg_sys_update(struct db_database_measure_base_type *pDB, const char *file, const char *default_file)
{
    char    *p, *pPri;
    int     size;
    int     ret;
    cJSON   *pJson, *pjsClass, *pjsGroup;
    struct db_cfg_sys_para *pSysCfg = &pDB->db_cfg.sys;

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
 
    /* get adjust parameters from syscfg.json */
    pjsGroup=cJSON_GetObjectItem(pjsClass, "adjust");
    if(!pjsGroup)
        goto __err_json_format;

    ret = db_cfg_json_group_base_write(pSysCfg, &db_sys_adjust_map[0], sizeof(db_sys_adjust_map)/sizeof(struct db_base_mapdef), pjsGroup);
    if(ret < 0)
        goto __err_json_format;

    /* get DEADZONE parameters from syscfg.json */
    pjsGroup=cJSON_GetObjectItem(pjsClass, "deadzone");
    if(!pjsGroup)
        goto __err_json_format;

    ret = db_cfg_json_group_base_write_deadzone(pSysCfg, &db_sys_deadzone_map[0], sizeof(db_sys_deadzone_map)/sizeof(struct db_base_mapdef), pjsGroup);
    if(ret < 0)
        goto __err_json_format;

    /* get INTERFACE parameters from syscfg.json */
    pjsGroup=cJSON_GetObjectItem(pjsClass, "interface");
    if(!pjsGroup)
        goto __err_json_format;

    ret = db_cfg_json_group_base_write(pSysCfg, &db_sys_interface_map[0], sizeof(db_sys_interface_map)/sizeof(struct db_base_mapdef), pjsGroup);
    if(ret < 0)
        goto __err_json_format;


    /* get INTERFACE parameters from syscfg.json */
    pjsGroup=cJSON_GetObjectItem(pjsClass, "adjustvalue");
    if(!pjsGroup)
        goto __err_json_format;

    ret = db_cfg_json_group_base_write(pSysCfg, &db_sys_adjust_value_map[0], sizeof(db_sys_adjust_value_map)/sizeof(struct db_base_mapdef), pjsGroup);
    if(ret < 0)
        goto __err_json_format;

    /*
        新增的4组参数回写（AD校准参数/AD校准值/相位补偿/漏电电流校准）
        与load保持一致的兼容策略：旧模板中没有该组时跳过，不报错
    */
    pjsGroup=cJSON_GetObjectItem(pjsClass, "adjustAd");
    if(pjsGroup)
    {
        ret = db_cfg_json_group_base_write(pSysCfg, &db_sys_adjust_ad_para_map[0], sizeof(db_sys_adjust_ad_para_map)/sizeof(struct db_base_mapdef), pjsGroup);
        if(ret < 0)
            goto __err_json_format;
    }

    pjsGroup=cJSON_GetObjectItem(pjsClass, "adjustAdvalue");
    if(pjsGroup)
    {
        ret = db_cfg_json_group_base_write(pSysCfg, &db_sys_adjust_ad_value_map[0], sizeof(db_sys_adjust_ad_value_map)/sizeof(struct db_base_mapdef), pjsGroup);
        if(ret < 0)
            goto __err_json_format;
    }

    pjsGroup=cJSON_GetObjectItem(pjsClass, "prthvalue");
    if(pjsGroup)
    {
        ret = db_cfg_json_group_base_write(pSysCfg, &db_sys_adjust_prth_map[0], sizeof(db_sys_adjust_prth_map)/sizeof(struct db_base_mapdef), pjsGroup);
        if(ret < 0)
            goto __err_json_format;
    }

    pjsGroup=cJSON_GetObjectItem(pjsClass, "adjustI0value");
    if(pjsGroup)
    {
        ret = db_cfg_json_group_base_write(pSysCfg, &db_sys_adjust_I0_map[0], sizeof(db_sys_adjust_I0_map)/sizeof(struct db_base_mapdef), pjsGroup);
        if(ret < 0)
            goto __err_json_format;
    }

    /*  */
    pPri = cJSON_Print(pJson);
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
int db_cfg_sys_update(struct db_database_measure_base_type *pDB, uint16_t chnlno)
{
    int ret = 0;
    char file[256], file_backup[256], file_default[256];
    const char *file_ptr;
    
    /* 每次调用后立即复制，避免静态缓冲区被后续调用覆盖 */
    file_ptr = db_cfg_get_sys_file(chnlno);
    if(!file_ptr)
    {
        printf("db_cfg_sys_update: invalid channel %d\n", chnlno);
        return -1;
    }
    snprintf(file, sizeof(file), "%s", file_ptr);
    
    file_ptr = db_cfg_get_sys_file_backup(chnlno);
    if(!file_ptr)
    {
        printf("db_cfg_sys_update: invalid channel %d\n", chnlno);
        return -1;
    }
    snprintf(file_backup, sizeof(file_backup), "%s", file_ptr);
    
    file_ptr = db_cfg_get_sys_file_default(chnlno);
    if(!file_ptr)
    {
        printf("db_cfg_sys_update: invalid channel %d\n", chnlno);
        return -1;
    }
    snprintf(file_default, sizeof(file_default), "%s", file_ptr);

    ret = __db_cfg_sys_update(pDB, file, file_default);
    ret = __db_cfg_sys_update(pDB, file_backup, file_default);

    return ret;
}



/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int db_cfg_sys_init(struct db_database_measure_base_type *pDB, uint16_t chnlno)
{
    int ret;
    char file[256], file_backup[256];
    const char *file_ptr;
    
    /* 每次调用后立即复制，避免静态缓冲区被后续调用覆盖 */
    file_ptr = db_cfg_get_sys_file(chnlno);
    if(!file_ptr)
    {
        printf("db_cfg_sys_init: invalid channel %d\n", chnlno);
        return -1;
    }
    snprintf(file, sizeof(file), "%s", file_ptr);
    
    file_ptr = db_cfg_get_sys_file_backup(chnlno);
    if(!file_ptr)
    {
        printf("db_cfg_sys_init: invalid channel %d\n", chnlno);
        return -1;
    }
    snprintf(file_backup, sizeof(file_backup), "%s", file_ptr);

    ret = db_cfg_sys_load(pDB, file, chnlno);
    if(ret < 0)
        ret = db_cfg_sys_load(pDB, file_backup, chnlno);

    return ret;
}

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
__inline struct db_cfg_sys_para *db_cfg_get_sys(struct db_database_measure_base_type *pDB)
{
    return &pDB->db_cfg.sys;
}

