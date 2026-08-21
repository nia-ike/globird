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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <dirent.h>
#include <sqlite3.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>
#include <sys/stat.h>  //stat
#include <sys/types.h> 

#include "dc_sql_cache.h"





#define DC_ENERGY_SQL_CREATE "CREATE TABLE IF NOT EXISTS [cache]("                            \
                             "  [_Id] INT64, "                                                        \
                             "  [_Time] TIMESTAMP NOT NULL DEFAULT (DATETIME ('now', 'localtime')), " \
                             "  [_Range] INT PRIMARY KEY UNIQUE ON CONFLICT REPLACE, "                \
                             "  [_Bin] BLOB); "


/*
    缓存暂时放置于内存中，预计使用8MB-10MB空间缓存
    根据测点数量进行缓存条数自动调整
*/
struct dc_sql_cache_num_tbl_t
{
    uint16_t        item_num;
    uint16_t        rows;
};


/*
    data cache manager
*/
struct dc_sql_cache_ctx
{
    uint64_t        id;
    pthread_mutex_t lock;
};



static struct dc_sql_cache_num_tbl_t dc_sql_cache_num_tbl[] = {
    /* 本表格的 item_num 每行必须递减 */
    {8192,       256},
    {4096,       512},
    {2048,      1024},
    {1024,      2048},
    {512,       4096},
    {256,       8192},
};


/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
static struct dc_sql_cache_ctx *dc_sql_cache_obj_get()
{
    static struct dc_sql_cache_ctx dc_cache_ctx;

    return &dc_cache_ctx;
}


/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
static int dc_sql_cache_rows_max(int items_num)
{
    int i;

    /* 超出最大点数，按最大点数的数量 */
    if(items_num >= dc_sql_cache_num_tbl[0].item_num)
    {
        return dc_sql_cache_num_tbl[0].rows;
    }

    /**/
    for(i=0;i<sizeof(dc_sql_cache_num_tbl)/sizeof(struct dc_sql_cache_num_tbl_t) - 1;i++)
    {
        if((items_num < dc_sql_cache_num_tbl[i].item_num) && (items_num > dc_sql_cache_num_tbl[i+1].item_num))
        {
            return dc_sql_cache_num_tbl[i].rows;
        }
    }

    return dc_sql_cache_num_tbl[i].rows;
}



/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
static int mkdir_fullpath(const char *pcPath)
{
	if(pcPath == NULL)return -1;

	char acPathBuffer[128] = {0x00};
	snprintf(acPathBuffer,sizeof(acPathBuffer)-1,"%s",pcPath);

	int32_t nPathLength = strlen(acPathBuffer);

	if (acPathBuffer[nPathLength - 1] != '/')
	{
		strcat(acPathBuffer, "/");
	}

	nPathLength = strlen(acPathBuffer);

	char acDirBuffer[64] = {0x00};

	int32_t Index = 0;

	for (Index = 0; Index < nPathLength; Index++)
	{
		memset(acDirBuffer,0x00,sizeof(acDirBuffer));
		if (('/' == acPathBuffer[Index]) && (0 != Index))
		{
			strncpy(acDirBuffer,acPathBuffer,Index);
			int32_t nStatus = access(acDirBuffer, F_OK);

			if (0 != nStatus)
			{
				if ((ENOTDIR == errno) || (ENOENT == errno))
				{
					nStatus = mkdir(acDirBuffer, 0755);
					if (0 != nStatus)
					{
						return -1;
					}
				}
				else
				{
					return -1;
				}
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
unsigned long dc_sql_file_size(void)
{
    unsigned long filesize = 0;
    struct stat statbuff;

    if(stat(DC_SQL_CACHE_MAIN, &statbuff) != -1)
    {
        filesize = statbuff.st_size;
    }
    else
    {
        filesize = 0;
    }

    return filesize;
}

/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
int dc_sql_cache_create(void)
{
    sqlite3 *db;
    char *zerr = NULL;
    int rc;
    struct dc_sql_cache_ctx *pCtx = dc_sql_cache_obj_get();

    pthread_mutex_lock(&pCtx->lock);
    rc = sqlite3_open(DC_SQL_CACHE_MAIN, &db);
    if (rc)
    {
        sqlite3_close(db);
        pthread_mutex_unlock(&pCtx->lock);
        printf("data cache create sqlite fail\n");
        exit(-1);
    }

    // CREATE table
    rc = sqlite3_exec(db, DC_ENERGY_SQL_CREATE, NULL, NULL, &zerr);
    if (rc != SQLITE_OK)
    {
        printf("%s:%s\n", __FUNCTION__, zerr);
        sqlite3_free(zerr);

        sqlite3_close(db);
        pthread_mutex_unlock(&pCtx->lock);
        return -1;
    }

    sqlite3_close(db);
    pthread_mutex_unlock(&pCtx->lock);
    return 0;
}


/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
static unsigned long get_file_size(const char *path)
{
    unsigned long filesize = 0;
    struct stat statbuff;
    if(stat(path, &statbuff) != -1)
    {
        filesize = statbuff.st_size;
    }
    else
    {
        filesize = 0;
    }
    return filesize;
}


/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
static int dc_sql_check(void)
{
    sqlite3 *db;
    int rc;
    unsigned long size;
    bool integrityVerified = false;
    sqlite3_stmt *integrity = NULL;

    /*
        数据库文件为0，即文件损坏；  注意数据库文件为0，    PRAGMA integrity_check; 会错误检查为OK，所以需要先检查文件大小
    */
    size = get_file_size(DC_SQL_CACHE_MAIN);
    if(size == 0)
    {
        return integrityVerified;
    }

    rc = sqlite3_open(DC_SQL_CACHE_MAIN, &db);
    if (rc)
        return false;

    if ( sqlite3_prepare_v2( db, "PRAGMA integrity_check;", -1, &integrity, NULL ) == SQLITE_OK ) 
    {
        while ( sqlite3_step( integrity ) == SQLITE_ROW ) 
        {
            const unsigned char *result = sqlite3_column_text( integrity, 0 );
            if ( result && strcmp( ( const char * )result, (const char *)"ok" ) == 0 ) 
            {
                integrityVerified = true;
                break;
            }
        }

        sqlite3_finalize(integrity);
    }

    sqlite3_close(db);

    return integrityVerified;
}



/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
void dc_sql_cache_delete(void)
{
    unlink(DC_SQL_CACHE_MAIN);
}



/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
int dc_sql_cache_get_newest_item(struct dc_sql_cache_item_t *pItem)
{
#define DC_SQL_CACHE_QUERY "select * from cache order by _Id desc limit 1;"
#define DC_SQL_CACHE_QUERY_LEN sizeof(DC_SQL_CACHE_QUERY)
    sqlite3 *db;
    int rc, ret = -1;
    sqlite3_stmt *stmt;
    struct tm ltime;
    
    struct dc_sql_cache_ctx *pCtx = dc_sql_cache_obj_get();

    pthread_mutex_lock(&pCtx->lock);
    rc = sqlite3_open(DC_SQL_CACHE_MAIN, &db);
    if (rc)
    {
        goto __err;
    }

    /*
        max 10 line of 1 database.
        we just need the line of max(_Id). other line just use for check
    */
    rc = sqlite3_prepare_v2(db, DC_SQL_CACHE_QUERY, DC_SQL_CACHE_QUERY_LEN, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        goto __err;
    }

    do
    {
        rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW)
        {
            pCtx->id            = sqlite3_column_int64(stmt, 0);
            pItem->id           = pCtx->id;    
            
            strptime(sqlite3_column_text(stmt, 1), "%Y-%m-%d %H:%M:%S", &ltime);
            pItem->ts           = mktime(&ltime);

            pItem->blob_size    = sqlite3_column_bytes(stmt, 3);
            if(pItem->blob_size > 0)
                memcpy(pItem->blob, sqlite3_column_blob(stmt, 3), pItem->blob_size);

            ret = 0;
        }
        else if (rc == SQLITE_DONE)
        {
            if(ret != 0)
                ret = -2;
            break;
        }
        else
        {
            ret = -1;
            break;
        }
    } while (1);


__err:
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    pthread_mutex_unlock(&pCtx->lock);

    return ret;
}


/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            time_t t_ptr.                                   **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
int dc_sql_cache_get_item_by_time(struct dc_sql_cache_item_t *pItem, time_t t_ptr)
{
    sqlite3 *db;
    int rc, n, ret = -1;
    sqlite3_stmt *stmt;
    char sql[128] = {0};
    struct tm ltime;

    struct dc_sql_cache_ctx *pCtx = dc_sql_cache_obj_get();

    /*
        time_t t_ptr.  查询指定 > t_ptr时刻 的数据，
    */

    pthread_mutex_lock(&pCtx->lock);
    rc = sqlite3_open(DC_SQL_CACHE_MAIN, &db);
    if (rc)
    {
        goto __err;
    }

    /*
        max 10 line of 1 database.
        we just need the line of max(_Id). other line just use for check
    */
    localtime_r(&t_ptr, &ltime);
    n = snprintf(sql, sizeof(sql),  "select * from cache where _Time > '%04d-%02d-%02d %02d:%02d:%02d' limit 1;",
                        ltime.tm_year+1900, ltime.tm_mon+1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec);

    rc = sqlite3_prepare_v2(db, sql, n, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        goto __err;
    }

    do
    {
        rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW)
        {
            pCtx->id            = sqlite3_column_int64(stmt, 0);
            pItem->id           = pCtx->id;    
            
            strptime(sqlite3_column_text(stmt, 1), "%Y-%m-%d %H:%M:%S", &ltime);
            pItem->ts           = mktime(&ltime);

            pItem->blob_size    = sqlite3_column_bytes(stmt, 3);
            if(pItem->blob_size > 0)
                memcpy(pItem->blob, sqlite3_column_blob(stmt, 3), pItem->blob_size);

            ret = 0;
        }
        else if (rc == SQLITE_DONE)
        {
            if(ret != 0)
                ret = -2;
            break;
        }
        else
        {
            ret = -1;
            break;
        }
    } while (1);


__err:
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    pthread_mutex_unlock(&pCtx->lock);

    return ret;
}

/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
int dc_sql_cache_set_item(struct dc_sql_cache_item_t *pItem, uint16_t max_items)
{
    sqlite3 *db;
    char sql[128] = {0};
    int n, index, rc;
    uint32_t iErrCnt;
    sqlite3_stmt *stmt;
    struct dc_sql_cache_ctx *pCtx = dc_sql_cache_obj_get();

    /* 检查数据文件的完整性 */
    rc = dc_sql_check();
    if(rc == false)
    {
        rc = -1;
        return rc;
    }

    pCtx->id++;
    pItem->id = pCtx->id;

    pthread_mutex_lock(&pCtx->lock);
    rc = sqlite3_open(DC_SQL_CACHE_MAIN, &db);
    if (rc)
    {
        goto __err;
    }
    
    /*
        max 10 line of 1 database.
        we just need the line of max(_Id). other line just use for check
    */
    n = snprintf(sql, sizeof(sql), "replace into cache(_Id,_Range,_Bin)" 
                                                "values("
                                                "%llu,%d,:_Bin);",
                                                pCtx->id, (int)(pCtx->id % dc_sql_cache_rows_max(max_items)));

    iErrCnt = 3;
    do 
    {
        rc = sqlite3_exec(db, sql, NULL, NULL, NULL);
        if(SQLITE_OK != rc)
        {
            usleep(200000);
        }
        else
        {
            break;
        }
    }while(iErrCnt--);

    rc = sqlite3_prepare_v2(db, sql, n, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        goto __err;
    }

    index = sqlite3_bind_parameter_index(stmt, ":_Bin");
    rc = sqlite3_bind_blob(stmt, index, pItem->blob, pItem->blob_size, SQLITE_STATIC);
    if (rc != SQLITE_OK)
    {
        goto __err;
    }

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
    {
        goto __err;
    }


__err:
    if (stmt)
        sqlite3_finalize(stmt);
    sqlite3_close(db);
    pthread_mutex_unlock(&pCtx->lock);    

    return rc;
}



/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:           删除 id < nitem 的所有数据                       **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
int dc_sql_cache_rows_flush(uint32_t nitem)
{
    int     rc;
    sqlite3 *db;
    char sql[256] = {0};
    struct dc_sql_cache_ctx *pCtx = dc_sql_cache_obj_get();

    pthread_mutex_lock(&pCtx->lock);
    rc = sqlite3_open(DC_SQL_CACHE_MAIN, &db);
    if (rc)
        goto __err;

    /*
        max 10 line of 1 database.
        we just need the line of max(_Id). other line just use for check
    */
    snprintf(sql, sizeof(sql), "delete from cache where _Id IN (select _Id from cache limit %d);", nitem);
    rc = sqlite3_exec(db, sql, NULL, NULL, NULL);
    if (rc != SQLITE_OK)
    {
        goto __err;
    }

    /* flush the db file space */
    snprintf(sql, sizeof(sql), "VACUUM");
    rc = sqlite3_exec(db, sql, NULL, NULL, NULL);
    if (rc != SQLITE_OK)
    {
        goto __err;
    }

__err:
    sqlite3_close(db);
    pthread_mutex_unlock(&pCtx->lock);    

    return rc;
}


/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
int dc_sql_cache_init(struct db_spl_realtime_data_tbl_t *pDB)
{ 
    int rc;
    struct dc_sql_cache_item_t item;
    struct dc_sql_cache_ctx *pCtx = dc_sql_cache_obj_get();

    pCtx->id = 0;
    /*
        foreach MAIN and BAKUP file, read the db
    */
    /*
        1. check the PATH
    */
    if(access(DC_SQL_CACHE_PATH, F_OK))
        mkdir_fullpath(DC_SQL_CACHE_PATH);

    if(access(DC_SQL_CACHE_MAIN, F_OK))
        dc_sql_cache_create();
    
    /* create locker */
    pthread_mutex_init(&pCtx->lock, NULL);

    /* 读取最新的id */
    rc = dc_sql_cache_get_newest_item(&item);
    if(rc == 0)
        pCtx->id = item.id;

    return -1;
}


/*************************************************************************/
/** Function Name : db_history_multirate_destroy                       **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
void dc_sql_cache_destroy(void)
{
    struct dc_sql_cache_ctx *pCtx = dc_sql_cache_obj_get();

    pthread_mutex_destroy(&pCtx->lock);
}