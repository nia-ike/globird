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
#include <sys/stat.h>
#include "cJSON.h"
#include <stddef.h>
#include "cJSON_Utils.h"
#include <time.h>

#include "db_cp.h"
#include "db_struct.h"
#include "db_cfg_type.h"
#include "db_cfg_json.h"
#include "db_cfg_user_multirate.h"

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
__inline struct db_cfg_user_multirate_para *db_cfg_get_user_multirate(struct db_database_type *pDB)
{
    /* 多费率配置是全局共享的，在 db_database_type 顶层 */
    return &pDB->multirate;
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
static int db_multirate_unit_read16(struct db_cfg_multirate_unit_type *pcfg, cJSON *pObject)
{
    cJSON *pItem;

    /*
            |-- {"hm":0, "rate":0}
    */

    /********************************************************/
    /* 加载时段起始时间 */
    /********************************************************/
    pItem = cJSON_GetObjectItem(pObject, "hm");
    if (!pItem)
        return -1;
    pcfg->s_hm.value = pItem->valueint;

    /********************************************************/
    
    /********************************************************/
    /* 加载时段费率号 */
    /********************************************************/
    pItem = cJSON_GetObjectItem(pObject, "rate");
    if (!pItem)
        return -1;
    pcfg->rate.value = pItem->valueint;
    /********************************************************/

    return 0;
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int db_multirate_unit_write16(struct db_cfg_multirate_unit_type *pcfg, cJSON *pObject)
{
    cJSON *pItem;

    /*
            |-- {"hm":0, "rate":0}
    */
    cJSON_ReplaceItemInObject(pObject, 
                              "hm", 
                              cJSON_CreateNumber(pcfg->s_hm.value));
    /********************************************************/
    
    /********************************************************/
  
    /********************************************************/
    cJSON_ReplaceItemInObject(pObject, 
                              "rate", 
                              cJSON_CreateNumber(pcfg->rate.value));
    /********************************************************/


    return 0;
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int db_multirate_week_unit_read16(struct db_cfg_multirate_week_unit_type *pcfg, cJSON *pObject)
{
    cJSON *pItem;

    /*
            |-- {"s_week":1},
    */

    /********************************************************/
    /* 加载周(工作日/节假日)起始时间 */
    /********************************************************/
    pItem = cJSON_GetObjectItem(pObject, "s_week");
    if (!pItem)
        return -1;
    pcfg->s_week.value = pItem->valueint;

    return 0;
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int db_multirate_week_unit_write16(struct db_cfg_multirate_week_unit_type *pcfg, cJSON *pObject)
{
    cJSON *pItem;

    /*
            |-- {"hm":0, "rate":0}
    */
    cJSON_ReplaceItemInObject(pObject, 
                              "s_week", 
                              cJSON_CreateNumber(pcfg->s_week.value));
    /********************************************************/

    return 0;
}

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int db_multirate_unit_group_read(struct db_cfg_multirate_unit_group_type *pcfg, cJSON *pObject)
{
    int ret;
    cJSON *pArrayItem;

    /*
            "day":[{"hm":0, "rate":0}, {"hm":-1 , "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}]},
    */

    /********************************************************/
    /* 加载时段起始时间 */
    /********************************************************/
    pArrayItem = cJSON_GetObjectItem(pObject, "day");
    if (!pArrayItem)
        return -1;
    
    if (cJSON_GetArraySize(pArrayItem) != MULTRATE_NUM_DAY)
    {
        printf("multirate time unit != %d\n", MULTRATE_NUM_DAY);
        return -1;
    }
        
    /********************************************************/
    for (int i = 0; i < MULTRATE_NUM_DAY; i++)
    {
        pObject = cJSON_GetArrayItem(pArrayItem, i);
        if (!pObject)
            return -1;
        
        ret = db_multirate_unit_read16(&pcfg->day[i], pObject);
        if (ret < 0)
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
static int db_multirate_unit_group_write(struct db_cfg_multirate_unit_group_type *pcfg, cJSON *pObject)
{
    int ret;
    cJSON *pArrayItem;

    /*
            "day":[{"hm":0, "rate":0}, {"hm":-1 , "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}]},
    */

    /********************************************************/
    /* 加载时段起始时间 */
    /********************************************************/
    pArrayItem = cJSON_GetObjectItem(pObject, "day");
    if (!pArrayItem)
        return -1;
    
    if (cJSON_GetArraySize(pArrayItem) != MULTRATE_NUM_DAY)
    {
        printf("multirate time unit != %d\n", MULTRATE_NUM_DAY);
        return -1;
    }
        
    /********************************************************/
    for (int i = 0; i < MULTRATE_NUM_DAY; i++)
    {
        pObject = cJSON_GetArrayItem(pArrayItem, i);
        if (!pObject)
            return -1;
        
        ret = db_multirate_unit_write16(&pcfg->day[i], pObject);
        if (ret < 0)
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
static int db_multirate_day_table_read(struct db_cfg_multirate_day_table_type *pcfg, cJSON *pObject)
{
    int ret;
    cJSON *pArrayItem;

    /*
         "table_day":[
                        {"id":0 ,"day":[{"hm":0, "rate":0}, {"hm":-1 , "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}]},
       
    */

    /********************************************************/
    /* 加载时段起始时间 */
    /********************************************************/
    pArrayItem = cJSON_GetObjectItem(pObject, "table_day");
    if (!pArrayItem)
        return -1;
    
    if (cJSON_GetArraySize(pArrayItem) != MULTRATE_NUM_DAY_TABLE)
    {
        printf("multirate time table != %d\n", MULTRATE_NUM_DAY_TABLE);
        return -1;
    }
        
    /********************************************************/
    for (int i = 0; i < MULTRATE_NUM_DAY_TABLE; i++)
    {
        pObject = cJSON_GetArrayItem(pArrayItem, i);
        if (!pObject)
            return -1;
        
        ret = db_multirate_unit_group_read(&pcfg->table_day[i], pObject);
        if (ret < 0)
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
static int db_multirate_day_table_write(struct db_cfg_multirate_day_table_type *pcfg, cJSON *pObject)
{
    int     ret;
    cJSON   *pArrayItem;

    /*
        "table_day":[
                        {"id":0 ,"day":[{"hm":0, "rate":0}, {"hm":-1 , "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}]},
                        {"id":1 ,"day":[{"hm":-1, "rate":0}, {"hm":-1 , "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}]},
                        {"id":2 ,"da
    */

    pArrayItem = cJSON_GetObjectItem(pObject, "table_day");
    if (!pArrayItem)
        return -1;

    if (cJSON_GetArraySize(pArrayItem) != MULTRATE_NUM_DAY_TABLE)
        return -1;

    for (int i = 0; i < MULTRATE_NUM_DAY_TABLE; i++)
    {
        pObject = cJSON_GetArrayItem(pArrayItem, i);
        if (!pObject)
            return -1;
        
        ret = db_multirate_unit_group_write(&pcfg->table_day[i], pObject);
        if (ret < 0)
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
static int db_multirate_week_table_read(struct db_cfg_multirate_week_table_type *pcfg, cJSON *pObject)
{
    int ret;
    cJSON *pArrayItem;

    /*
        "table_week":[
                        {"s_week":1},
                        {"s_week":-1},
                        {"s_week":-1},
                        {"s_week":-1}
    ],
    */

    /********************************************************/
    /* 加载时段起始时间 */
    /********************************************************/
    pArrayItem = cJSON_GetObjectItem(pObject, "table_week");
    if (!pArrayItem)
        return -1;
    
    if (cJSON_GetArraySize(pArrayItem) != MULTRATE_NUM_WEEK)
    {
        printf("multirate time table != %d\n", MULTRATE_NUM_WEEK);
        return -1;
    }
        
    /********************************************************/
    for (int i = 0; i < MULTRATE_NUM_WEEK; i++)
    {
        pObject = cJSON_GetArrayItem(pArrayItem, i);
        if (!pObject)
            return -1;
        
        ret = db_multirate_week_unit_read16(&pcfg->week[i], pObject);
        if (ret < 0)
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
static int db_multirate_week_table_write(struct db_cfg_multirate_week_table_type *pcfg, cJSON *pObject)
{
    int ret;
    cJSON *pArrayItem;

    /*
        "table_week":[
                        {"s_week":1},
                        {"s_week":-1},
                        {"s_week":-1},
                        {"s_week":-1}
    ],
    */

    /********************************************************/
    /* 加载时段起始时间 */
    /********************************************************/
    pArrayItem = cJSON_GetObjectItem(pObject, "table_week");
    if (!pArrayItem)
        return -1;
    
    if (cJSON_GetArraySize(pArrayItem) != MULTRATE_NUM_WEEK)
    {
        printf("multirate time table != %d\n", MULTRATE_NUM_WEEK);
        return -1;
    }
        
    /********************************************************/
    for (int i = 0; i < MULTRATE_NUM_WEEK; i++)
    {
        pObject = cJSON_GetArrayItem(pArrayItem, i);
        if (!pObject)
            return -1;
        
        ret = db_multirate_week_unit_write16(&pcfg->week[i], pObject);
        if (ret < 0)
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
static int db_multirate_special_day_read(struct db_cfg_multirate_specialday_type *pcfg, cJSON *pObject)
{
    int     i, ret;
    cJSON  *pItem;

    /*
           "special":[
                        {
                            "md":-1,
                            "day":0
                        },
    */
    pItem = cJSON_GetObjectItem(pObject, "md");
    if (!pItem)
        return -1;

    pcfg->s_md.value = pItem->valueint;


    pItem = cJSON_GetObjectItem(pObject, "day");
    if (!pItem)
        return -1;

    pcfg->day.value = pItem->valueint;

    return 0;
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int db_multirate_special_day_write(struct db_cfg_multirate_specialday_type *pcfg, cJSON *pObject)
{
    int     i, ret;
    cJSON  *pItem;

    /*
            "special":[
                        {
                            "md":-1,
                            "day":0
                        },
    */
    cJSON_ReplaceItemInObject(pObject, 
                              "md", 
                              cJSON_CreateNumber(pcfg->s_md.value));

    cJSON_ReplaceItemInObject(pObject, 
                              "day", 
                              cJSON_CreateNumber(pcfg->day.value));

    return 0;
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int db_multirate_special_read(struct db_cfg_multirate_special_type *pcfg, cJSON *pObject)
{
    int     i, ret;
    cJSON  *pArrayItem;

    /*
        "special":[
            {
                "md":-1,
                "day":0
            },
    */

    pArrayItem = cJSON_GetObjectItem(pObject, "special");
    if (!pArrayItem)
        return -1;

    if (cJSON_GetArraySize(pArrayItem) != MULTRATE_NUM_DAY_SPEC)
        return -1;

    for (int i = 0; i < MULTRATE_NUM_DAY_SPEC; i++)
    {
        pObject = cJSON_GetArrayItem(pArrayItem, i);
        if (!pObject)
            return -1;
        
        ret = db_multirate_special_day_read(&pcfg->cfg[i], pObject);
        if (ret < 0)
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
static int db_multirate_special_write(struct db_cfg_multirate_special_type *pcfg, cJSON *pObject)
{
    int     i, ret;
    cJSON  *pArrayItem;

    /*
        "special":[
        {
            "md":0101,
            "day":[
                {"hm":0, "rate":0}, {"hm":-1 , "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}
            ]
    */

    pArrayItem = cJSON_GetObjectItem(pObject, "special");
    if (!pArrayItem)
        return -1;

    if (cJSON_GetArraySize(pArrayItem) != MULTRATE_NUM_DAY_SPEC)
        return -1;

    for (int i = 0; i < MULTRATE_NUM_DAY_SPEC; i++)
    {
        pObject = cJSON_GetArrayItem(pArrayItem, i);
        if (!pObject)
            return -1;
        
        ret = db_multirate_special_day_write(&pcfg->cfg[i], pObject);
        if (ret < 0)
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
static int db_multirate_general_day_read(struct db_cfg_multirate_day_type *pcfg, cJSON *pObject)
{
    int     ret;
    cJSON  *pItem;

    /*
             "day":0
    */
    pItem = cJSON_GetObjectItem(pObject, "day");
    if (!pItem)
        return -1;
    
    pcfg->day.value = pItem->valueint;

    return 0;
}



/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int db_multirate_general_day_write(struct db_cfg_multirate_day_type *pcfg, cJSON *pObject)
{
    int     i, ret;
    cJSON  *pItem, *pArrayItem;

    /*
            "day":0
    */
   cJSON_ReplaceItemInObject(pObject, 
                             "day", 
                             cJSON_CreateNumber(pcfg->day.value));
  
    return 0;
}




/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int db_multirate_general_week_read(struct db_cfg_multirate_week_type *pcfg, cJSON *pObject)
{
    int     i, ret;
    cJSON  *pItem, *pArrayItem;

    /*
            "md": -1,
            "week":[
                {"w":1,"day":[{"h.....
    */
    pItem = cJSON_GetObjectItem(pObject, "md");
    if (!pItem)
        return -1;
    /*
        周划分
    */
    pcfg->s_md.value = pItem->valueint;

    pArrayItem = cJSON_GetObjectItem(pObject, "week");
    if (!pArrayItem)
        return -1;

    if (cJSON_GetArraySize(pArrayItem) != MULTRATE_NUM_WEEK)
        return -1;

    for (int i = 0; i < MULTRATE_NUM_WEEK; i++)
    {
        pObject = cJSON_GetArrayItem(pArrayItem, i);
        if (!pObject)
            return -1;
        
        ret = db_multirate_general_day_read(&pcfg->week[i], pObject);
        if (ret < 0)
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
static int db_multirate_general_week_write(struct db_cfg_multirate_week_type *pcfg, cJSON *pObject)
{
    int     i, ret;
    cJSON  *pItem, *pArrayItem;

    /*
            "md": 101,
            "week":[
                {
                    "day":0
                },
                {"day":-1},
                {"day":-1},
                {"day":-1}
    */
    cJSON_ReplaceItemInObject(pObject, 
                              "md", 
                              cJSON_CreateNumber(pcfg->s_md.value));
    
    pArrayItem = cJSON_GetObjectItem(pObject, "week");
    if (!pArrayItem)
        return -1;

    if (cJSON_GetArraySize(pArrayItem) != MULTRATE_NUM_WEEK)
        return -1;

    for (int i = 0; i < MULTRATE_NUM_WEEK; i++)
    {
        pObject = cJSON_GetArrayItem(pArrayItem, i);
        if (!pObject)
            return -1;
        
        ret = db_multirate_general_day_write(&pcfg->week[i], pObject);
        if (ret < 0)
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
static int db_multirate_general_read(struct db_cfg_multirate_general_type *pcfg, cJSON *pObject)
{
    int     i, ret;
    cJSON  *pItem, *pArrayItem;

    /*
        "normal":{
            "season":[
                {
                    "md": 0101,
    */
    pItem = cJSON_GetObjectItem(pObject, "normal");
    if (!pItem)
        return -1;

    pArrayItem = cJSON_GetObjectItem(pItem, "season");
    if (!pArrayItem)
        return -1;

    if (cJSON_GetArraySize(pArrayItem) != MULTRATE_NUM_SEASON)
        return -1;

    for (int i = 0; i < MULTRATE_NUM_SEASON; i++)
    {
        pObject = cJSON_GetArrayItem(pArrayItem, i);
        if (!pObject)
            return -1;
        
        ret = db_multirate_general_week_read(&pcfg->season[i], pObject);
        if (ret < 0)
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
static int db_multirate_general_write(struct db_cfg_multirate_general_type *pcfg, cJSON *pObject)
{
    int     i, ret;
    cJSON  *pItem, *pArrayItem;

    /*
        "normal":{
            "season":[
                {
                    "md": 0101,
    */
    pItem = cJSON_GetObjectItem(pObject, "normal");
    if (!pItem)
        return -1;

    pArrayItem = cJSON_GetObjectItem(pItem, "season");
    if (!pArrayItem)
        return -1;

    if (cJSON_GetArraySize(pArrayItem) != MULTRATE_NUM_SEASON)
        return -1;

    for (int i = 0; i < MULTRATE_NUM_SEASON; i++)
    {
        pObject = cJSON_GetArrayItem(pArrayItem, i);
        if (!pObject)
            return -1;
        
        ret = db_multirate_general_week_write(&pcfg->season[i], pObject);
        if (ret < 0)
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
static int db_multirate_schema_load(struct db_cfg_multirate_schema_type *pCfg, char *file)
{
    char    *p;
    int     ret, size;
    cJSON   *pJson, *pjsClass;

    /*
        pJson("scheme.json")
            |-- pjsClass("special")
            |-- pjsClass("normal")
    */
   
    p = db_cfg_json_file_alloc(file, &size);
    if (NULL == p)
        return -1;

    p = db_cfg_json_file_read(file, p, size);
    if (NULL == p)
        goto __err_free;

    pJson = cJSON_Parse(p);
    if (!pJson){
        printf("Error before: [%s]\n", cJSON_GetErrorPtr());
        goto __err_free;
    }
    /********************************************************/
    /* 加载时段表 */
    /********************************************************/
    ret = db_multirate_day_table_read(&pCfg->table_day, pJson);
    if (ret < 0)
        goto __err_json_format;

    /********************************************************/
    /* 加载周段表参数 */
    /********************************************************/
    ret = db_multirate_week_table_read(&pCfg->table_week, pJson);
    if (ret < 0)
        goto __err_json_format;

    /********************************************************/
    /* 加载特殊日参数 */
    /********************************************************/
    ret = db_multirate_special_read(&pCfg->special, pJson);
    if (ret < 0)
        goto __err_json_format;
    /********************************************************/
    
    /********************************************************/
    /* 加载一般日参数 */
    /********************************************************/
    ret = db_multirate_general_read(&pCfg->general, pJson);
    if (ret < 0)
        goto __err_json_format;
    /********************************************************/

    //printf("%s \n",p);  

    cJSON_Delete(pJson);
    db_cfg_json_file_delete(p);
    return 0;

__err_json_format:
    cJSON_Delete(pJson);

__err_free:
    db_cfg_json_file_delete(p);
    return -1;
}



/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int db_multirate_cfg_read(struct db_database_type *pDB, const char *file)
{
    char    *p;
    int     ret, size;
    cJSON   *pVersion, *pJson, *pjsClass, *pItem;

    /*
        pJson("scheme.json")
            |-- pjsClass("special")
            |-- pjsClass("normal")
    */
    p = db_cfg_json_file_alloc(file, &size);
    if (NULL == p)
        return -1;

    p = db_cfg_json_file_read(file, p, size);
    if (NULL == p)
        goto __err_free;

    pJson = cJSON_Parse(p);
    if (!pJson)
        goto __err_free;

    /********************************************************/
    /*  */
    /********************************************************/

    /*
        "setting":{
            "switchtime":0,
    */


    /********************************************************/
    /* 加载切换时间 */
    /********************************************************/
    pjsClass = cJSON_GetObjectItem(pJson, "setting");
    if (!pjsClass)
        return -1;

    pItem = cJSON_GetObjectItem(pjsClass, "switchtime");
    if (!pItem)
        return -1;
    
    pDB->multirate.unix_switch.value = pItem->valueint;
    /********************************************************/
    
    cJSON_Delete(pJson);
    db_cfg_json_file_delete(p);
    return 0;

__err_json_format:
    cJSON_Delete(pJson);

__err_free:
    db_cfg_json_file_delete(p);
    return -1;
}




/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int __db_multirate_cfg_update(struct db_database_type *pDB, const char *file)
{
    char    *p, *pPri;
    int     size;
    int     ret;
    cJSON   *pJson, *pjsClass, *pjsGroup, *pItem;
 
    /* prepare the buffer for json */
    /*
        use the default file for template.
        because the default file not be write aften, it is more save.
        this function will change all the parameters that cunstomer set,
        
    */
    p = db_cfg_jsonfile_alloc(db_cfg_get_multirate_file(DB_CFG_TYPE_DEFAULT), &size);
    if(p == NULL)
        return -1;
   
    p = db_cfg_jsonfile_read(db_cfg_get_multirate_file(DB_CFG_TYPE_DEFAULT), p, size);
    if(p == NULL)
        goto __err_free;
 
    pJson = cJSON_Parse(p);  
    if(!pJson)
    {
        printf("Json file[%s] error\n", db_cfg_get_multirate_file(DB_CFG_TYPE_DEFAULT));
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
    pjsGroup=cJSON_GetObjectItem(pjsClass, "switchtime");
    if(!pjsGroup)
        goto __err_json_format;

    /* value */
    cJSON_ReplaceItemInObject(pjsClass, "switchtime", cJSON_CreateNumber(pDB->multirate.unix_switch.value));

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
static int __db_multirate_schema_update(struct db_cfg_multirate_schema_type *pCfg, const char *file)
{
    char    *p, *pPri;
    int     size;
    int     ret;
    cJSON   *pJson, *pjsClass, *pjsGroup, *pItem;

    /* prepare the buffer for json */
    /*
        use the default file for template.
        because the default file not be write aften, it is more save.
        this function will change all the parameters that cunstomer set,
        
    */
    p = db_cfg_jsonfile_alloc(db_cfg_get_multirate_schema1_file(DB_CFG_TYPE_DEFAULT), &size);
    if(p == NULL)
        return -1;
   
    p = db_cfg_jsonfile_read(db_cfg_get_multirate_schema1_file(DB_CFG_TYPE_DEFAULT), p, size);
    if(p == NULL)
        goto __err_free;
 
    pJson = cJSON_Parse(p);  
    if(!pJson)
    {
        printf("Json file[%s] error\n", file);
        goto __err_free;
    }
    
    /* 
        write day table
     */
    db_multirate_day_table_write(&pCfg->table_day, pJson);
    /*
        write week table
    */
    db_multirate_week_table_write(&pCfg->table_week, pJson);
    /*
        write special
    */
    db_multirate_special_write(&pCfg->special, pJson);
    /* write general */
    db_multirate_general_write(&pCfg->general, pJson);
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
int db_multirate_cfg_load(struct db_database_type *pDB)
{
    int ret;
    const char *file = db_cfg_get_multirate_file(DB_CFG_TYPE_NORMAL);
    const char *file_backup = db_cfg_get_multirate_file(DB_CFG_TYPE_BACKUP);

    /* load multirate CFG top */
    ret = db_multirate_cfg_read(pDB, file);
    if(ret < 0)
    {
        ret = db_multirate_cfg_read(pDB, file_backup);
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
int db_cfg_user_multirate_load(struct db_database_type *pDB)
{
    int ret = -1;

    ret = db_multirate_cfg_load(pDB);
    if(ret < 0)
    {
        //use the old multirate config
        printf("Can't load new multirate cfg\n");
        return -1;
    }

    /*
        read old schema. 
        if main error, read bakup
        if bakup error tooo, read default
    */
    ret = db_multirate_schema_load(&pDB->multirate.schema, db_cfg_get_multirate_schema1_file(DB_CFG_TYPE_NORMAL));
    if(ret < 0)
    {
        ret = db_multirate_schema_load(&pDB->multirate.schema, db_cfg_get_multirate_schema1_file(DB_CFG_TYPE_BACKUP));
        if(ret < 0)
        {
            ret = db_multirate_schema_load(&pDB->multirate.schema, db_cfg_get_multirate_schema1_file(DB_CFG_TYPE_DEFAULT));
        }
    }

    ret = db_multirate_schema_load(&pDB->multirate.schema_new, db_cfg_get_multirate_schema2_file(DB_CFG_TYPE_NORMAL));
    if(ret < 0)
    {
        ret = db_multirate_schema_load(&pDB->multirate.schema_new, db_cfg_get_multirate_schema2_file(DB_CFG_TYPE_BACKUP));
        if(ret < 0)
        {
            ret = db_multirate_schema_load(&pDB->multirate.schema_new, db_cfg_get_multirate_schema2_file(DB_CFG_TYPE_DEFAULT));
        }
    }
    return ret;
}






/*********************************************************************************************************
** Function name:       db_cfg_user_multirate_update
** Descriptions:        update the multirate's json file
** input parameters:    pDB     - database pointer
** output parameters:   none
** Returned value:      none
*********************************************************************************************************/
int db_cfg_user_multirate_update(struct db_database_type *pDB)
{
    int                                 ret;
    struct db_cfg_user_multirate_para *pCfg = &pDB->multirate;
    /*
        multirate
            |-- meter_multirate_config.json             (选择费率方案)
            |-- meter_multirate_scheme1.json            (加载费率方案参数)
    */
    const char *file = db_cfg_get_multirate_file(DB_CFG_TYPE_NORMAL);
    const char *file_backup = db_cfg_get_multirate_file(DB_CFG_TYPE_BACKUP);
    const char *schema1 = db_cfg_get_multirate_schema1_file(DB_CFG_TYPE_NORMAL);
    const char *schema1_backup = db_cfg_get_multirate_schema1_file(DB_CFG_TYPE_BACKUP);
    const char *schema2 = db_cfg_get_multirate_schema2_file(DB_CFG_TYPE_NORMAL);
    const char *schema2_backup = db_cfg_get_multirate_schema2_file(DB_CFG_TYPE_BACKUP);

    __db_multirate_cfg_update(pDB, file);
    db_file_copy(file, file_backup);
    
    /* 当支持多套费率时，应根据设置的切换时间来获取第n套费率的json文件名。 */
    __db_multirate_schema_update(&pCfg->schema, schema1);
    db_file_copy(schema1, schema1_backup);
    __db_multirate_schema_update(&pCfg->schema_new, schema2);
    db_file_copy(schema2, schema2_backup);

    return 0;
}

/*********************************************************************************************************
** Function name:       db_cfg_user_multirate_default
** Descriptions:        default the multirate's json file
** input parameters:    pDB     - database pointer
** output parameters:   none
** Returned value:      none
*********************************************************************************************************/
void db_cfg_user_multirate_default(struct db_database_type *pDB)
{
    const char *file = db_cfg_get_multirate_file(DB_CFG_TYPE_NORMAL);
    const char *file_backup = db_cfg_get_multirate_file(DB_CFG_TYPE_BACKUP);
    const char *file_default = db_cfg_get_multirate_file(DB_CFG_TYPE_DEFAULT);
    const char *schema1 = db_cfg_get_multirate_schema1_file(DB_CFG_TYPE_NORMAL);
    const char *schema1_backup = db_cfg_get_multirate_schema1_file(DB_CFG_TYPE_BACKUP);
    const char *schema1_default = db_cfg_get_multirate_schema1_file(DB_CFG_TYPE_DEFAULT);
    const char *schema2 = db_cfg_get_multirate_schema2_file(DB_CFG_TYPE_NORMAL);
    const char *schema2_backup = db_cfg_get_multirate_schema2_file(DB_CFG_TYPE_BACKUP);
    const char *schema2_default = db_cfg_get_multirate_schema2_file(DB_CFG_TYPE_DEFAULT);

    unlink(file);
    unlink(file_backup);
    unlink(schema1);
    unlink(schema1_backup);
    unlink(schema2);
    unlink(schema2_backup);

    db_file_copy(file_default, file);
    db_file_copy(file_default, file_backup);

    db_file_copy(schema1_default, schema1);
    db_file_copy(schema1_default, schema1_backup);

    db_file_copy(schema2_default, schema2);
    db_file_copy(schema2_default, schema2_backup);

    db_cfg_user_multirate_load(pDB);
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int db_cfg_user_multirate_init(struct db_database_type *pDB)
{
    return db_cfg_user_multirate_load(pDB);
}