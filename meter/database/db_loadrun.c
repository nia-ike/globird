/*
* Copyright (c) 2013,珠海派诺电子有限公司
* All rights reserved.
* 
* 文件名称：db_realtime.c
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
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stddef.h>
#include <dirent.h>
#include <sqlite3.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>
#include <sys/stat.h>  //stat
#include <sys/types.h> 

#include "db_struct.h"
#include "db_msr_msgevent_api.h"


#define DB_LOADRUN_NUM_PER_DB    (0x64)               // x % 4  = x & 0x3.  save 4 loadrun record

#define DB_LOADRUN_SQL_PATH      "/userdata/loadrun"
#define DB_LOADRUN_SQL_MAIN      DB_LOADRUN_SQL_PATH"/rt_loadrun_main.db"
#define DB_LOADRUN_SQL_BAKUP     DB_LOADRUN_SQL_PATH"/rt_loadrun_bakup.db"

#define DB_LOADRUN_SQL_CREATE "CREATE TABLE IF NOT EXISTS [realtime_loadrun]("                          \
                             "  [_Id] INT64, "                                                        \
                             "  [_Time] TIMESTAMP NOT NULL DEFAULT (DATETIME ('now', 'localtime')), " \
                             "  [_Range] INT PRIMARY KEY UNIQUE ON CONFLICT REPLACE, "                \
                             "  [DOH] INT64, "                                                   \
                             "  [LRH] INT64, "                                                   \
                             "  [LOH] INT64, "                                                   \
                             "  [LRC] INT64);"


/*
    LOADRUN DATABASE manager
*/
struct db_rt_loadrun_sql_ctx
{
    uint64_t        id;
    pthread_mutex_t lock;
};


/* 动态上下文数组支持多通道 */
static struct db_rt_loadrun_sql_ctx *db_loadrun_ctx_array = NULL;
static uint8_t db_loadrun_ctx_max_channels = 0;

static struct db_rt_loadrun_sql_ctx* db_loadrun_get_ctx(uint8_t channel)
{
    if(!db_loadrun_ctx_array || channel >= db_loadrun_ctx_max_channels)
        return NULL;
    
    return &db_loadrun_ctx_array[channel];
}

static int db_loadrun_ctx_init(uint8_t max_channels)
{
    if(db_loadrun_ctx_array)
        return 0;
    
    db_loadrun_ctx_max_channels = max_channels;
    db_loadrun_ctx_array = (struct db_rt_loadrun_sql_ctx*)calloc(max_channels, 
                                                                  sizeof(struct db_rt_loadrun_sql_ctx));
    
    if(!db_loadrun_ctx_array)
    {
        printf("Failed to allocate loadrun contexts for %d channels\n", max_channels);
        return -1;
    }
    
    for(int i = 0; i < max_channels; i++)
    {
        pthread_mutex_init(&db_loadrun_ctx_array[i].lock, NULL);
        db_loadrun_ctx_array[i].id = 0;
    }
    
    printf("Loadrun contexts initialized for %d channels\n", max_channels);
    return 0;
}

static void db_loadrun_ctx_destroy(void)
{
    if(!db_loadrun_ctx_array)
        return;
    
    for(int i = 0; i < db_loadrun_ctx_max_channels; i++)
    {
        pthread_mutex_destroy(&db_loadrun_ctx_array[i].lock);
    }
    
    free(db_loadrun_ctx_array);
    db_loadrun_ctx_array = NULL;
    db_loadrun_ctx_max_channels = 0;
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
static struct db_statistical_loadrun_type *db_loadrun_rt_get(struct db_database_measure_base_type *pDB)
{
    return &pDB->db_measure.statistical.loadrun;
}

/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
static void db_loadrun_rt_set(struct db_database_measure_base_type *pDB, struct db_statistical_loadrun_type *pLoadrun)
{
    pDB->db_measure.statistical.loadrun = *pLoadrun;
}

/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
static int db_loadrun_sql_create(struct db_rt_loadrun_sql_ctx *pCtx, char *db_name)
{
    sqlite3 *db;
    char *zerr = NULL;
    int rc;

    pthread_mutex_lock(&pCtx->lock);
    rc = sqlite3_open(db_name, &db);
    if (rc)
    {
        sqlite3_close(db);
        pthread_mutex_unlock(&pCtx->lock);
        return -1;
    }

    // CREATE table
    rc = sqlite3_exec(db, DB_LOADRUN_SQL_CREATE, NULL, NULL, &zerr);
    if (rc != SQLITE_OK)
    {
        printf("%s:%s\n", __FUNCTION__, zerr);
        sqlite3_free(zerr);

        sqlite3_close(db);
        pthread_mutex_unlock(&pCtx->lock);
        return -1;
    }

    if(zerr) sqlite3_free(zerr);

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
static int db_loadrun_sql_get_item(struct db_rt_loadrun_sql_ctx *pCtx, char *db_name, struct db_statistical_loadrun_type *pLoadrun)
{
#define DB_LOADRUN_REALTIME_SQL "select * from realtime_loadrun order by _Id desc limit 1;"
#define DB_LOADRUN_REALTIME_SQL_LEN sizeof(DB_LOADRUN_REALTIME_SQL)
    sqlite3 *db;
    char *zerr;
    int rc, ret = -1;
    sqlite3_stmt *stmt;

    pthread_mutex_lock(&pCtx->lock);
    rc = sqlite3_open(db_name, &db);
    if (rc)
    {
        sqlite3_close(db);
        pthread_mutex_unlock(&pCtx->lock);
        return -1;
    }

    /*
        max 10 line of 1 database.
        we just need the line of max(_Id). other line just use for check
    */
    rc = sqlite3_prepare_v2(db, DB_LOADRUN_REALTIME_SQL, DB_LOADRUN_REALTIME_SQL_LEN, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        if (stmt)
            sqlite3_finalize(stmt);

        sqlite3_close(db);
        pthread_mutex_unlock(&pCtx->lock);
        return -1;
    }

    do
    {
        rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW)
        {
            pCtx->id        = sqlite3_column_int64(stmt, 0);
            // 兼容性：从64位截断到32位
            pLoadrun->doh   = (uint32_t)sqlite3_column_int64(stmt, 3);   //" DOH "
            pLoadrun->lrh   = (uint32_t)sqlite3_column_int64(stmt, 4);   //" LRH "
            pLoadrun->loh   = (uint32_t)sqlite3_column_int64(stmt, 5);   //" LOH "
            pLoadrun->lrc   = (uint32_t)sqlite3_column_int64(stmt, 6);   //" LRC "
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
static int db_loadrun_sql_set_item(struct db_rt_loadrun_sql_ctx *pCtx, char *db_name, struct db_statistical_loadrun_type *pLoadrun)
{
    sqlite3 *db;
    char sql[2048];
    char *zerr;
    int rc, ret = -1;
    uint32_t iErrCnt;
    sqlite3_stmt *stmt;

    pthread_mutex_lock(&pCtx->lock);
    rc = sqlite3_open(db_name, &db);
    if (rc)
    {
        sqlite3_close(db);
        pthread_mutex_unlock(&pCtx->lock);
        return -1;
    }

    /*
        max 10 line of 1 database.
        we just need the line of max(_Id). other line just use for check
    */
    snprintf(sql, sizeof(sql), "replace into realtime_loadrun(_Id,_Range," 
                                                "DOH,LRH,LOH,LRC)"
                                                "values("
                                                "%llu,%d,"
                                                "%lld,%lld,%lld,%lld);",
                                                pCtx->id, (uint16_t)(pCtx->id % DB_LOADRUN_NUM_PER_DB),
                                                (uint64_t)pLoadrun->doh, (uint64_t)pLoadrun->lrh, 
                                                (uint64_t)pLoadrun->loh, (uint64_t)pLoadrun->lrc);

    //printf("%s\n", sql); 
    iErrCnt = 3;
    do 
    {
        rc = sqlite3_exec(db, sql, NULL, NULL, &zerr);
        if(SQLITE_OK != rc)
        {
            if(!strstr(zerr, "database is locked"))
                break;
            
            usleep(200000);
        }
        else
        {
            break;
        }
    }while(iErrCnt--);

    if(zerr)
        sqlite3_free(zerr);

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
static int db_loadrun_load(struct db_rt_loadrun_sql_ctx *pCtx, struct db_database_measure_base_type *pDB, uint8_t channel)
{ 
    int ret;
    struct db_statistical_loadrun_type tmpLoadrun;
    const char *filemain = db_loadrun_get_main(channel);

    if(!pCtx)
        return -1;
    
    pCtx->id = 0;
    /*
        foreach MAIN and BAKUP file, read the db
    */
    /*
        1. check the PATH
    */
    if(access(DB_LOADRUN_SQL_DIR, F_OK))
        mkdir_fullpath(DB_LOADRUN_SQL_DIR);

    /*
        2. check the main.db and bakup.db
           A. main is OK?  main OK -> finish loading
           B. main is fail, read the bakup. 
           C. bakup is OK?  bakup OK -> finish loading
           D. bakup is fail too, default 0. 

    */
    if(access(filemain, F_OK) == 0)
    {
        ret = db_loadrun_sql_get_item(pCtx, (char*)filemain, &tmpLoadrun);
        if(ret == 0)
        {
            /* load the GOOD setting */
            db_loadrun_rt_set(pDB, &tmpLoadrun);
            return 0;
        }
    }

    /* if here, can not load setting from MAIN, we need bakup */
    if(access(DB_LOADRUN_SQL_BAKUP, F_OK) == 0)
    {
        ret = db_loadrun_sql_get_item(pCtx, DB_LOADRUN_SQL_BAKUP, &tmpLoadrun);
        if(ret == 0)
        {
            /* load the GOOD setting */
            db_loadrun_rt_set(pDB, &tmpLoadrun);
            return 0;
        }
    }
    
    /* if here, both MAIN and BAKUP is fail, we clear the loadrun */
    memset(&tmpLoadrun, 0 , sizeof(tmpLoadrun));
    db_loadrun_rt_set(pDB, &tmpLoadrun);

    return -1;
}

/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
int db_loadrun_default(struct db_database_measure_base_type *pDB, uint8_t channel)
{
    struct db_rt_loadrun_sql_ctx *pCtx = db_loadrun_get_ctx(channel);
    
    if(!pCtx)
    {
        printf("db_loadrun_default: invalid channel %d\n", channel);
        return -1;
    }
    
    unlink(db_loadrun_get_main(channel));

    db_loadrun_load(pCtx, pDB, channel);

    return 0;
}



/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
void db_loadrun_update(struct db_database_measure_base_type *pDB, uint8_t channel)
{
    int ret;
    struct db_rt_loadrun_sql_ctx *pCtx = db_loadrun_get_ctx(channel);
    const char *filemain = db_loadrun_get_main(channel);
    
    if(!pCtx)
    {
        printf("db_loadrun_update: invalid channel %d\n", channel);
        return;
    }
     
    /* no main.db, create it */
    if(access(filemain, F_OK))
        db_loadrun_sql_create(pCtx, (char*)filemain);

    pCtx->id++;
    if(pCtx->id > 0xffffffffffffff00)
    {
        /*
            generally speaking, never here in this device's life cycle.
            this just use for repair the loadrun if some issue.
        */
        unlink(filemain);
        pCtx->id = 0;
    }

    /* add one record to main */
    ret = db_loadrun_sql_set_item(pCtx, (char*)filemain, db_loadrun_rt_get(pDB));
    if(ret < 0)
    {
        /* file exist. but file is error, delete it and re-create it */
        unlink(filemain);
        ret = db_loadrun_sql_create(pCtx, (char*)filemain);
        db_loadrun_sql_set_item(pCtx, (char*)filemain, db_loadrun_rt_get(pDB));
    }
}




/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
int db_loadrun_init(struct db_database_measure_base_type *pDB, uint8_t channel)
{
    int ret;
    struct db_rt_loadrun_sql_ctx *pCtx;
    
    if(!pDB)
        return -1;

    /* 首次调用时初始化上下文数组 */
    if(!db_loadrun_ctx_array)
    {
        uint8_t max_channels = db_msr_msg_event_api_get_max_channels();
        if(db_loadrun_ctx_init(max_channels) < 0)
            return -1;
    }
    
    pCtx = db_loadrun_get_ctx(channel);
    if(!pCtx)
    {
        printf("db_loadrun_init: invalid channel %d\n", channel);
        return -1;
    }
    
    /* load file from  */
    db_loadrun_load(pCtx, pDB, channel);

    
}



/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
int db_loadrun_destroy(struct db_database_measure_base_type *pDB, uint8_t channel)
{
    if(!pDB)
        return -1;

    /* 注意：不销毁单个上下文的 mutex，统一在 cleanup 中处理 */
    return 0;
}

void db_loadrun_cleanup_all(void)
{
    db_loadrun_ctx_destroy();
}