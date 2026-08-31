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
#define DB_MAP_TABLE_RELAY(x)                DB_toString(x),(offsetof(struct db_cfg_user_relay_para,x)+offsetof(struct db_database_expand_type,db_relay))

static const struct db_base_mapdef db_user_relay_map[] = 
{
    { DB_MAP_TABLE_RELAY(relay[0].mode            ),  sizeof(uint16_t)},
    { DB_MAP_TABLE_RELAY(relay[0].reset_time      ),  sizeof(uint16_t)},
    { DB_MAP_TABLE_RELAY(relay[1].mode            ),  sizeof(uint16_t)},
    { DB_MAP_TABLE_RELAY(relay[1].reset_time      ),  sizeof(uint16_t)},
};




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

static int db_cfg_json_group_base_write(void *pCfg, const struct db_base_mapdef *pMap, int MapSize, cJSON *pArrayItem);


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int user2_json_object_tree_write(cJSON* pjsGroup, char * newstring, struct db_database_expand_type *pCfg, const struct db_base_mapdef *pMap, int MapSize)
{
    int ret = -1;

    cJSON *pItem;    

    struct json_new_object_u32 json_new_object_list[] =
    {
        {
            .object =  NULL,
            .name = "di1_mode",
            .Val = 0,
            .ValDef = 0,
            .Max = 9,
            .Min = 0,
        },
        {
            .object =  NULL,
            .name = "di1_factor",
            .Val = 100,
            .ValDef = 100,
            .Max = 9999,
            .Min = 1,
        },
        {
            .object =  NULL,
            .name = "di2_mode",
            .Val = 0,
            .ValDef = 0,
            .Max = 9,
            .Min = 0,
        },
        {
            .object =  NULL,
            .name = "di2_factor",
            .Val = 100,
            .ValDef = 100,
            .Max = 9999,
            .Min = 1,
        },    
    };

    struct json_new_object_u32 json_new_object_list2[] =
    {
        {
            .object =  NULL,
            .name = "setPtot",
            .Val = 0,
            .ValDef = 0,
            .Max = 99999999,
            .Min = 0,
        },
        {
            .object =  NULL,
            .name = "setQtot",
            .Val = 100,
            .ValDef = 100,
            .Max = 99999999,
            .Min = 1,
        },
        {
            .object =  NULL,
            .name = "setStot",
            .Val = 0,
            .ValDef = 0,
            .Max = 99999999,
            .Min = 0,
        },  
    };

    pItem = cJSON_GetObjectItem(pjsGroup, newstring);
    if ( NULL  == pItem)
    {
        cJSON  *array   =  cJSON_CreateArray();
        cJSON_AddItemToObject(pjsGroup,newstring, array);
        if(strcmp(newstring,"di_pluse") == 0)
        {
            for (size_t i = 0; i < sizeof(json_new_object_list)/sizeof(json_new_object_list[0]); i++)
            {
                json_new_object_list[i].object =  cJSON_CreateObject();  
                if(NULL != json_new_object_list[i].object)
                {
                    cJSON_AddItemToArray(array,json_new_object_list[i].object);
                    cJSON_AddStringToObject(json_new_object_list[i].object, "name",   json_new_object_list[i].name);        
                    cJSON_AddNumberToObject(json_new_object_list[i].object, "Val",    json_new_object_list[i].Val);
                    cJSON_AddNumberToObject(json_new_object_list[i].object, "ValDef", json_new_object_list[i].ValDef);
                    cJSON_AddNumberToObject(json_new_object_list[i].object, "Max",    json_new_object_list[i].Max);
                    cJSON_AddNumberToObject(json_new_object_list[i].object, "Min",    json_new_object_list[i].Min);
                }
            }   
        }
        if(strcmp(newstring,"runing_energy") == 0)
        {
            for (size_t i = 0; i < sizeof(json_new_object_list2)/sizeof(json_new_object_list2[0]); i++)
            {
                json_new_object_list2[i].object =  cJSON_CreateObject(); 
                if(NULL != json_new_object_list2[i].object)
                {
                    cJSON_AddItemToArray(array,json_new_object_list2[i].object);
                    cJSON_AddStringToObject(json_new_object_list2[i].object, "name",   json_new_object_list2[i].name);        
                    cJSON_AddNumberToObject(json_new_object_list2[i].object, "Val",    json_new_object_list2[i].Val);
                    cJSON_AddNumberToObject(json_new_object_list2[i].object, "ValDef", json_new_object_list2[i].ValDef);
                    cJSON_AddNumberToObject(json_new_object_list2[i].object, "Max",    json_new_object_list2[i].Max);
                    cJSON_AddNumberToObject(json_new_object_list2[i].object, "Min",    json_new_object_list2[i].Min);
                }
            }   
        }

        pItem = cJSON_GetObjectItem(pjsGroup, newstring);
        
        ret = db_cfg_json_group_base_write(pCfg, pMap, MapSize, pItem);

        if(ret < 0)
            return -1;
    }
    else
    {
        ret = db_cfg_json_group_base_write(pCfg, pMap, MapSize, pItem);
        if(ret < 0)
            return -1;
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
    printf("read->pProp->scope.Max %d \n",pProp->scope.Max);
    printf("read->pProp->scope.Min %d \n",pProp->scope.Min);
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
    printf("read->pProp->scope.Max %d \n",pProp->scope.Max);
    printf("read->pProp->scope.Min %d \n",pProp->scope.Min);
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
    printf("read->pProp->scope.Max %d \n",pProp->scope.Max);
    printf("read->pProp->scope.Min %d \n",pProp->scope.Min);
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
    printf("read->pProp->scope.Max %d \n",pProp->scope.Max);
    printf("read->pProp->scope.Min %d \n",pProp->scope.Min);
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
//            printf("%s %s\n",pMap[j].pName ,pItem->valuestring);
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
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int db_cfg_user_expand_load(struct db_database_type *pDB, const char *file)
{
    char    *p;
    int     size;
    int     ret;
    cJSON   *pJson, *pjsClass, *pjsGroup;
    struct db_database_expand_type *pCfg = &(pDB->db_expand);
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
    pjsGroup=cJSON_GetObjectItem(pjsClass, "relay");
    if(!pjsGroup)
        goto __err_json_format;
    ret = db_cfg_json_group_base_read(pCfg, &db_user_relay_map[0],
        sizeof(db_user_relay_map) / sizeof(struct db_base_mapdef), pjsGroup);
    if(ret < 0)
        goto __err_json_format;

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
void db_cfg_user_expand_default(struct db_database_type *pDB)
{
    const char *file = db_cfg_get_digi_file(DB_CFG_TYPE_NORMAL);
    const char *file_backup = db_cfg_get_digi_file(DB_CFG_TYPE_BACKUP);
    const char *file_default = db_cfg_get_digi_file(DB_CFG_TYPE_DEFAULT);

    unlink(file);
    unlink(file_backup);
    db_file_copy(file_default, file);
    db_file_copy(file_default, file_backup);

    db_cfg_user_expand_load(pDB, file);
}



/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int __db_cfg_user_expand_update(struct db_database_type *pDB, const char *file)
{
    char    *p, *pPri;
    int     size;
    int     ret;
    cJSON   *pJson, *pjsClass, *pjsGroup;
    struct db_database_expand_type *pCfg = &(pDB->db_expand);
    const char *default_file = db_cfg_get_digi_file(DB_CFG_TYPE_DEFAULT);
    
    /* prepare the buffer for json */
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
    pjsGroup=cJSON_GetObjectItem(pjsClass, "relay");
    if(!pjsGroup)
        goto __err_json_format;

    ret = db_cfg_json_group_base_write(pCfg, &db_user_relay_map[0], sizeof(db_user_relay_map)/sizeof(struct db_base_mapdef), pjsGroup);
    if(ret < 0)
        goto __err_json_format;


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
int db_cfg_user_expand_update(struct db_database_type *pDB)
{
    int ret;
    const char *file = db_cfg_get_digi_file(DB_CFG_TYPE_NORMAL);
    const char *file_backup = db_cfg_get_digi_file(DB_CFG_TYPE_BACKUP);
    
    ret = __db_cfg_user_expand_update(pDB, file);
    ret = __db_cfg_user_expand_update(pDB, file_backup);

    system("/ego/etc/monit.d/program/net_setting.sh");
    return ret;
}

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int db_cfg_user_expand_init(struct db_database_type *pDB)
{
    int ret;
    const char *file = db_cfg_get_digi_file(DB_CFG_TYPE_NORMAL);

    ret = db_cfg_user_expand_load(pDB, file);
    if(ret < 0)
    {
        const char *file_backup = db_cfg_get_digi_file(DB_CFG_TYPE_BACKUP);
        ret = db_cfg_user_expand_load(pDB, file_backup);
    }
    
    return ret;
}



/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
__inline struct db_database_expand_type *db_cfg_get_user_digi(struct db_database_type *pDB)
{    
    return &pDB->db_expand;
}

