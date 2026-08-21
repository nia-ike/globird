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

#define _GNU_SOURCE

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





/*
    新库以 _Id 为主键追加写入；旧库仍以 _Range 为主键时，写入时 _Range=_Id，
    保证每条记录唯一，由文件容量水位滚动删除最旧数据。
*/
#define DC_ENERGY_SQL_CREATE "CREATE TABLE IF NOT EXISTS [cache]("                            \
                             "  [_Id] INTEGER PRIMARY KEY ON CONFLICT REPLACE, "              \
                             "  [_Time] TIMESTAMP NOT NULL DEFAULT (DATETIME ('now', 'localtime')), " \
                             "  [_Range] INT, "                                               \
                             "  [_Bin] BLOB); "


/*
    data cache manager
*/
struct dc_sql_cache_ctx
{
    uint64_t        id;
    uint32_t        last_blob_size;
    uint8_t         mode;
    unsigned long   high_size;
    unsigned long   low_size;
    pthread_mutex_t lock;
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
    static struct dc_sql_cache_ctx dc_cache_ctx = {
        .mode = DC_CFG_LEGACY_MODE,
        .high_size = DC_CFG_LEGACY_HIGH_SIZE,
        .low_size = DC_CFG_LEGACY_LOW_SIZE,
    };

    return &dc_cache_ctx;
}


/*************************************************************************/
/** Function Name:     dc_sql_cache_apply_policy                       **/
/** Purpose:           应用配置中的模式与高低水位                       **/
/** Params:            attr - 配置属性                                 **/
/** Return:            None.                                           **/
/** Notice:            应在 dc_cfg_load 之后调用                       **/
/*************************************************************************/
void dc_sql_cache_apply_policy(const struct datacache_attr_t *attr)
{
    struct dc_sql_cache_ctx *pCtx = dc_sql_cache_obj_get();

    if (attr == NULL)
        return;

    pCtx->mode = attr->mode;
    pCtx->high_size = attr->high_size ? attr->high_size : DC_CFG_LEGACY_HIGH_SIZE;
    pCtx->low_size = attr->low_size ? attr->low_size : DC_CFG_LEGACY_LOW_SIZE;

    if (pCtx->low_size >= pCtx->high_size)
    {
        if (pCtx->high_size > (1u * 1024u * 1024u))
            pCtx->low_size = pCtx->high_size - (1u * 1024u * 1024u);
        else
            pCtx->low_size = pCtx->high_size / 2;
    }
}


/*************************************************************************/
/** Function Name:     dc_sql_cache_high_size                          **/
/** Purpose:           获取当前高水位字节数                             **/
/** Params:            None.                                           **/
/** Return:            高水位大小                                      **/
/** Notice:            None.                                           **/
/*************************************************************************/
unsigned long dc_sql_cache_high_size(void)
{
    return dc_sql_cache_obj_get()->high_size;
}


/*************************************************************************/
/** Function Name:     dc_sql_cache_mode                               **/
/** Purpose:           获取当前缓存模式                                 **/
/** Params:            None.                                           **/
/** Return:            DC_CACHE_MODE_ROLL / DAILY                      **/
/** Notice:            None.                                           **/
/*************************************************************************/
uint8_t dc_sql_cache_mode(void)
{
    return dc_sql_cache_obj_get()->mode;
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

    /* 新库尽量自动回收页；已有库该 PRAGMA 可能无效，不影响使用 */
    sqlite3_exec(db, "PRAGMA auto_vacuum=FULL;", NULL, NULL, NULL);

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
    sqlite3 *db = NULL;
    int rc, ret = -1;
    sqlite3_stmt *stmt = NULL;
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


int dc_sql_cache_get_item(struct dc_sql_cache_item_t *pItem)
{
    return dc_sql_cache_get_newest_item(pItem);
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
/** Function Name:     dc_sql_cache_trim_by_size_unlocked              **/
/** Purpose:           按容量水位删除最旧记录并 VACUUM（调用方已持锁） **/
/** Params:            per_row_bytes - 单条粗估大小，用于算删除条数     **/
/** Return:            0 成功；非0 SQLite 错误码                        **/
/** Notice:            仅在文件超过高水位时执行，删到低水位后停止       **/
/*************************************************************************/
static int dc_sql_cache_trim_by_size_unlocked(sqlite3 *db, uint32_t per_row_bytes)
{
    char sql[256];
    int rc = 0;
    int round;
    unsigned long size;
    unsigned long excess;
    uint32_t per;
    uint32_t nitem;
    struct dc_sql_cache_ctx *pCtx = dc_sql_cache_obj_get();

    per = per_row_bytes + 128;
    if (per < 256)
        per = 256;

    for (round = 0; round < 4; round++)
    {
        size = get_file_size(DC_SQL_CACHE_MAIN);
        if (size <= pCtx->high_size)
            return 0;

        if (size > pCtx->low_size)
            excess = size - pCtx->low_size;
        else
            excess = per;

        nitem = (uint32_t)(excess / per) + 32;
        if (nitem < 16)
            nitem = 16;
        if (nitem > 4096)
            nitem = 4096;

        snprintf(sql, sizeof(sql),
                 "delete from cache where _Id in (select _Id from cache order by _Id asc limit %u);",
                 nitem);
        rc = sqlite3_exec(db, sql, NULL, NULL, NULL);
        if (rc != SQLITE_OK)
            return rc;

        /* 必须 VACUUM，否则 stat 文件大小不下降，下载体积也不降 */
        rc = sqlite3_exec(db, "VACUUM", NULL, NULL, NULL);
        if (rc != SQLITE_OK)
            return rc;
    }

    return rc;
}


/*************************************************************************/
/** Function Name:     dc_sql_cache_set_item                           **/
/** Purpose:           追加写入一条压缩缓存                             **/
/** Params:            max_items 已忽略，保留调用兼容                   **/
/** Return:            0 成功；1 日模式已满跳过；<0 失败               **/
/** Notice:            roll=超高水位裁到低水位；daily=达高水位停止写入 **/
/*************************************************************************/
int dc_sql_cache_set_item(struct dc_sql_cache_item_t *pItem, uint16_t max_items)
{
    sqlite3 *db = NULL;
    char sql[160] = {0};
    int n, index, rc = -1;
    uint32_t iErrCnt;
    sqlite3_stmt *stmt = NULL;
    struct dc_sql_cache_ctx *pCtx = dc_sql_cache_obj_get();

    (void)max_items;

    if (pItem == NULL || pItem->blob_size == 0)
        return -1;

    /* 检查数据文件的完整性 */
    if (dc_sql_check() == false)
        return -1;

    pthread_mutex_lock(&pCtx->lock);

    /* 日模式：达到高水位后当天不再写入（不滚动删旧） */
    if (pCtx->mode == DC_CACHE_MODE_DAILY)
    {
        if (get_file_size(DC_SQL_CACHE_MAIN) >= pCtx->high_size)
        {
            pthread_mutex_unlock(&pCtx->lock);
            return DC_SQL_CACHE_RET_FULL;
        }
    }

    pCtx->id++;
    pItem->id = pCtx->id;
    pCtx->last_blob_size = pItem->blob_size;

    rc = sqlite3_open(DC_SQL_CACHE_MAIN, &db);
    if (rc != SQLITE_OK)
        goto __err;

    if (pCtx->mode == DC_CACHE_MODE_ROLL)
    {
        /* 写前先腾空间，避免库文件先顶破上限 */
        rc = dc_sql_cache_trim_by_size_unlocked(db, pItem->blob_size);
        if (rc != SQLITE_OK)
            goto __err;
    }

    /*
        _Range 写入与 _Id 相同：兼容旧表（_Range 主键）与新表（_Id 主键），
        每条记录占用独立槽位，不再按测点分档取模覆盖。
    */
    n = snprintf(sql, sizeof(sql),
                 "insert or replace into cache(_Id,_Range,_Bin) values(%llu,%llu,:_Bin);",
                 (unsigned long long)pCtx->id,
                 (unsigned long long)pCtx->id);

    iErrCnt = 3;
    do
    {
        rc = sqlite3_prepare_v2(db, sql, n, &stmt, NULL);
        if (rc == SQLITE_OK)
            break;

        sqlite3_finalize(stmt);
        stmt = NULL;
        usleep(200000);
    } while (iErrCnt--);

    if (rc != SQLITE_OK)
        goto __err;

    index = sqlite3_bind_parameter_index(stmt, ":_Bin");
    rc = sqlite3_bind_blob(stmt, index, pItem->blob, pItem->blob_size, SQLITE_STATIC);
    if (rc != SQLITE_OK)
        goto __err;

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
        goto __err;

    sqlite3_finalize(stmt);
    stmt = NULL;

    if (pCtx->mode == DC_CACHE_MODE_ROLL)
    {
        /* 写入后若仍超高水位（大 blob / 估算偏差），再裁一轮 */
        rc = dc_sql_cache_trim_by_size_unlocked(db, pItem->blob_size);
        if (rc != SQLITE_OK)
            goto __err;
    }

    rc = 0;

__err:
    if (stmt)
        sqlite3_finalize(stmt);
    if (db)
        sqlite3_close(db);
    pthread_mutex_unlock(&pCtx->lock);

    return rc;
}



/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:           删除最旧 nitem 条并 VACUUM                       **/
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

    if (nitem == 0)
        return 0;

    pthread_mutex_lock(&pCtx->lock);
    rc = sqlite3_open(DC_SQL_CACHE_MAIN, &db);
    if (rc)
        goto __err;

    snprintf(sql, sizeof(sql),
             "delete from cache where _Id IN (select _Id from cache order by _Id asc limit %u);",
             nitem);
    rc = sqlite3_exec(db, sql, NULL, NULL, NULL);
    if (rc != SQLITE_OK)
    {
        goto __err;
    }

    /* flush the db file space */
    rc = sqlite3_exec(db, "VACUUM", NULL, NULL, NULL);
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

    (void)pDB;

    pCtx->id = 0;
    pCtx->last_blob_size = 0;
    pthread_mutex_init(&pCtx->lock, NULL);

    if(access(DC_SQL_CACHE_PATH, F_OK))
    {
        if(mkdir_fullpath(DC_SQL_CACHE_PATH) != 0)
            return -1;
    }

    if(access(DC_SQL_CACHE_MAIN, F_OK))
    {
        if(dc_sql_cache_create() != 0)
            return -1;
    }

    /* 读取最新的id */
    rc = dc_sql_cache_get_newest_item(&item);
    if(rc == 0)
        pCtx->id = item.id;

    return 0;
}


/*************************************************************************/
/** Function Name:     dc_sql_cache_file_copy                          **/
/** Purpose:           拷贝文件（日模式归档用）                         **/
/** Params:            src - 源路径；dst - 目标路径                     **/
/** Return:            0 成功；-1 失败                                 **/
/** Notice:            None.                                           **/
/*************************************************************************/
static int dc_sql_cache_file_copy(const char *src, const char *dst)
{
    FILE *in, *out;
    char buf[8192];
    size_t n;

    in = fopen(src, "rb");
    if (in == NULL)
        return -1;

    out = fopen(dst, "wb");
    if (out == NULL)
    {
        fclose(in);
        return -1;
    }

    while ((n = fread(buf, 1, sizeof(buf), in)) > 0)
    {
        if (fwrite(buf, 1, n, out) != n)
        {
            fclose(in);
            fclose(out);
            unlink(dst);
            return -1;
        }
    }

    fclose(in);
    fclose(out);
    return 0;
}


/*************************************************************************/
/** Function Name:     dc_sql_cache_gzip_file                          **/
/** Purpose:           gzip 单文件；成功后删除明文                      **/
/** Params:            plain - 源；gz - 目标；fast - 非0 使用 gzip -1   **/
/** Return:            0 成功；-1 失败（保留明文）                      **/
/** Notice:            失败时删除不完整 .gz                             **/
/*************************************************************************/
static int dc_sql_cache_gzip_file(const char *plain, const char *gz, int fast)
{
    char cmd[512];
    struct stat st;

    if (plain == NULL || gz == NULL)
        return -1;

    unlink(gz);
    if (fast)
        snprintf(cmd, sizeof(cmd), "gzip -1 -c \"%s\" > \"%s\"", plain, gz);
    else
        snprintf(cmd, sizeof(cmd), "gzip -c \"%s\" > \"%s\"", plain, gz);

    if (system(cmd) != 0)
    {
        unlink(gz);
        return -1;
    }

    if (stat(gz, &st) != 0 || !S_ISREG(st.st_mode) || st.st_size < 10)
    {
        unlink(gz);
        return -1;
    }

    unlink(plain);
    return 0;
}


/*************************************************************************/
/** Function Name:     dc_sql_cache_history_ymd                        **/
/** Purpose:           解析 YYYYMMDD.db.gz / YYYYMMDD.db 中的日期       **/
/** Params:            name - 文件名；y/m/d - 输出年月日                **/
/** Return:            0 成功；-1 失败                                 **/
/** Notice:            明文 .db 仅用于 gzip 失败后的残留清理            **/
/*************************************************************************/
static int dc_sql_cache_history_ymd(const char *name, int *y, int *m, int *d)
{
    int yy, mm, dd;
    size_t len;

    if (name == NULL)
        return -1;
    len = strlen(name);
    if (len == 14 && strcmp(name + 8, ".db.gz") == 0)
    {
        if (sscanf(name, "%04d%02d%02d.db.gz", &yy, &mm, &dd) != 3)
            return -1;
    }
    else if (len == 11 && strcmp(name + 8, ".db") == 0)
    {
        if (sscanf(name, "%04d%02d%02d.db", &yy, &mm, &dd) != 3)
            return -1;
    }
    else
        return -1;

    if (yy < 2000 || mm < 1 || mm > 12 || dd < 1 || dd > 31)
        return -1;

    *y = yy;
    *m = mm;
    *d = dd;
    return 0;
}


/*************************************************************************/
/** Function Name:     dc_sql_cache_history_purge                      **/
/** Purpose:           按保留天数删除最旧的历史归档文件                 **/
/** Params:            history_path - 目录；keep_days - 保留天数        **/
/** Return:            None.                                           **/
/** Notice:            文件名需为 YYYYMMDD.db.gz（或 gzip 失败残留的 .db） **/
/*************************************************************************/
static void dc_sql_cache_history_purge(const char *history_path, uint16_t keep_days)
{
    DIR *dir;
    struct dirent *ent;
    char path[256];
    int y, m, d;
    int count = 0;
    int i, j;

    struct {
        int ymd;
        char name[32];
    } list[64];

    if (history_path == NULL || history_path[0] == '\0' || keep_days == 0)
        return;

    dir = opendir(history_path);
    if (dir == NULL)
        return;

    memset(list, 0, sizeof(list));
    while ((ent = readdir(dir)) != NULL)
    {
        if (dc_sql_cache_history_ymd(ent->d_name, &y, &m, &d) != 0)
            continue;
        if (count >= (int)(sizeof(list) / sizeof(list[0])))
            break;
        list[count].ymd = y * 10000 + m * 100 + d;
        strncpy(list[count].name, ent->d_name, sizeof(list[count].name) - 1);
        count++;
    }
    closedir(dir);

    /* 升序：最旧在前 */
    for (i = 0; i < count; i++)
    {
        for (j = i + 1; j < count; j++)
        {
            if (list[j].ymd < list[i].ymd)
            {
                int ty = list[i].ymd;
                char tname[32];
                memcpy(tname, list[i].name, sizeof(tname));
                list[i].ymd = list[j].ymd;
                memcpy(list[i].name, list[j].name, sizeof(list[i].name));
                list[j].ymd = ty;
                memcpy(list[j].name, tname, sizeof(list[j].name));
            }
        }
    }

    while (count > (int)keep_days)
    {
        snprintf(path, sizeof(path), "%s/%s", history_path, list[0].name);
        unlink(path);
        memmove(&list[0], &list[1], (size_t)(count - 1) * sizeof(list[0]));
        count--;
    }
}


/*************************************************************************/
/** Function Name:     dc_sql_cache_daily_rollover                     **/
/** Purpose:           日模式切日归档并重建当日空库                     **/
/** Params:            attr - 配置；day - 要归档的日期（一般为昨天）    **/
/** Return:            0 成功；-1 失败；roll 模式直接返回 0            **/
/** Notice:            先重建空库再 gzip，归档为 YYYYMMDD.db.gz        **/
/*************************************************************************/
int dc_sql_cache_daily_rollover(const struct datacache_attr_t *attr, const struct tm *day)
{
    char plain[256];
    char gz[256];
    struct dc_sql_cache_ctx *pCtx = dc_sql_cache_obj_get();

    if (attr == NULL || day == NULL)
        return -1;

    if (pCtx->mode != DC_CACHE_MODE_DAILY)
        return 0;

    if (attr->history_path[0] == '\0')
        return -1;

    if (mkdir_fullpath(attr->history_path) != 0)
        return -1;

    /* 归档“刚结束的那天”：传入的 day 应为昨天 */
    snprintf(plain, sizeof(plain), "%s/%04d%02d%02d.db",
             attr->history_path,
             day->tm_year + 1900, day->tm_mon + 1, day->tm_mday);
    snprintf(gz, sizeof(gz), "%s/%04d%02d%02d.db.gz",
             attr->history_path,
             day->tm_year + 1900, day->tm_mon + 1, day->tm_mday);

    if (access(DC_SQL_CACHE_MAIN, F_OK) == 0)
    {
        unlink(plain);
        if (dc_sql_cache_file_copy(DC_SQL_CACHE_MAIN, plain) != 0)
            return -1;
    }

    /* 立刻重建空库，gzip 不挡住后续写入 */
    dc_sql_cache_delete();
    if (dc_sql_cache_create() != 0)
        return -1;

    pCtx->id = 0;

    if (access(plain, F_OK) == 0)
    {
        /* gzip 失败保留明文，避免丢档（列表只公告 .db.gz） */
        (void)dc_sql_cache_gzip_file(plain, gz, 0);
    }

    dc_sql_cache_history_purge(attr->history_path, attr->history_days);
    return 0;
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