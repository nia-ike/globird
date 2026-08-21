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

#include "db_cfg.h"
#include "db_struct.h"
#include "db_device.h"  /* for rt_db_getVmem */ 


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
void db_cfg_user_default(struct db_database_measure_base_type *pMeasureBase, uint16_t flg)
{
    /* 获取顶层数据库指针（多费率配置在顶层） */
    struct db_database_type *pDB = (struct db_database_type*)rt_db_getVmem();
    
    db_cfg_user_base_default(pMeasureBase, flg);
    db_cfg_user_multirate_default(pDB);
    return ;
}

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
void db_cfg_default(struct db_database_measure_base_type *pMeasureBase, uint16_t flg)
{
    db_cfg_sys_default(pMeasureBase, flg);
    db_cfg_user_default(pMeasureBase, flg);
    return ;
}

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int db_cfg_user_update(struct db_database_measure_base_type *pMeasureBase, uint16_t flg)
{
    /* 获取顶层数据库指针（多费率配置在顶层） */
    struct db_database_type *pDB = (struct db_database_type*)rt_db_getVmem();
    
    db_cfg_user_base_update(pMeasureBase, flg);
    db_cfg_user_multirate_update(pDB);
    return 0;
}

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
void db_cfg_update(struct db_database_measure_base_type *pMeasureBase, uint16_t flg)
{
    db_cfg_sys_update(pMeasureBase, flg);
    db_cfg_user_update(pMeasureBase, flg);
    return ;
}

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int db_cfg_user_load(struct db_database_measure_base_type *pMeasureBase, const char *file, uint16_t flg)
{
    /* 获取顶层数据库指针（多费率配置在顶层） */
    struct db_database_type *pDB = (struct db_database_type*)rt_db_getVmem();
    
    db_cfg_user_base_init(pMeasureBase, flg);
    db_cfg_user_multirate_init(pDB);

    return 0;
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int db_cfg_user_init(struct db_database_measure_base_type *pMeasureBase, uint16_t flg)
{
    /* 获取顶层数据库指针（多费率配置在顶层） */
    struct db_database_type *pDB = (struct db_database_type*)rt_db_getVmem();
    
    db_cfg_user_base_init(pMeasureBase, flg);
    db_cfg_user_multirate_init(pDB);

    return 0;
}

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int db_cfg_init(struct db_database_measure_base_type *pMeasureBase, uint16_t flg)
{
    db_cfg_sys_init(pMeasureBase, flg);
    db_cfg_user_init(pMeasureBase, flg);
    return 0;
}

