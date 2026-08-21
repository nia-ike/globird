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
#ifndef _DC_SQL_CACHE_H_
#define _DC_SQL_CACHE_H_

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "stdint.h"
#include <stddef.h>
#include <dirent.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>
#include <sys/stat.h>  //stat
#include <sys/types.h> 

#include "db_struct.h"


#define DC_SQL_CACHE_PATH      "/var/run/cache/"
#define DC_SQL_CACHE_MAIN      DC_SQL_CACHE_PATH"/data.db"

#define DC_SQL_CACHE_FILE_MAXSIZE       (8*1024*1024)
#define DC_SQL_CACHE_BLOB_BUFSIZE       (192*1024)


struct dc_sql_cache_item_t 
{
    uint64_t id; //
    time_t   ts; // timestamp
    uint8_t  blob[DC_SQL_CACHE_BLOB_BUFSIZE];  // 每个节点16字节， 最大8192个节点， + 索引表300字节左右。 设定192KB留裕量
    uint32_t blob_size;
};



int dc_sql_cache_init(struct db_spl_realtime_data_tbl_t *pDB);
void dc_sql_cache_destroy(void);

unsigned long dc_sql_file_size(void);

int dc_sql_cache_create(void);
void dc_sql_cache_delete(void);
int dc_sql_cache_get_item(struct dc_sql_cache_item_t *pItem);
int dc_sql_cache_get_item_by_time(struct dc_sql_cache_item_t *pItem, time_t t_ptr);
int dc_sql_cache_set_item(struct dc_sql_cache_item_t *pItem, uint16_t max_items);


int dc_sql_cache_rows_flush(uint32_t nitem);
#endif
