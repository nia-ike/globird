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


#define DB_MAXDMD_NUM_PER_DB    (72)               // x % 4  = x & 0x3.  save 4 energy record

#define DB_REALTIME_MAXDMD_SQL_PATH      "/userdata/maxdmd"
#define DB_REALTIME_MAXDMD_SQL_MAIN      DB_REALTIME_MAXDMD_SQL_PATH"/demand_realtime_max.db"
#define DB_REALTIME_MAXDMD_SQL_BAKUP     DB_REALTIME_MAXDMD_SQL_PATH"/demand_realtime_max_bakup.db"

#define DB_MAXDMD_SQL_CREATE(__CLASS__) "CREATE TABLE IF NOT EXISTS [" \
                                    #__CLASS__ \
                                    "](" \
                                    " [_Id] INT64," \
                                    " [_Time] INTERGER NOT NULL DEFAULT (DATETIME ('now', 'localtime'))," \
                                    " [_Range] INT PRIMARY KEY UNIQUE ON CONFLICT REPLACE," \
                                    " [Ia_Time] INTERGER, 	[Ia] INT64," \
                                    " [Ib_Time] INTERGER, 	[Ib] INT64," \
                                    " [Ic_Time] INTERGER, 	[Ic] INT64," \
                                    " [IAvg_Time] INTERGER, [IAvg] INT64," \
                                    " [Pa_Time] INTERGER, 	[Pa] INT64," \
                                    " [Pb_Time] INTERGER, 	[Pb] INT64," \
                                    " [Pc_Time] INTERGER, 	[Pc] INT64," \
                                    " [Ptot_Time] INTERGER, [Ptot] INT64," \
                                    " [Qa_Time] INTERGER, 	[Qa] INT64," \
                                    " [Qb_Time] INTERGER, 	[Qb] INT64," \
                                    " [Qc_Time] INTERGER, 	[Qc] INT64," \
                                    " [Qtot_Time] INTERGER, [Qtot] INT64," \
                                    " [Sa_Time] INTERGER, 	[Sa] UINT64," \
                                    " [Sb_Time] INTERGER, 	[Sb] UINT64," \
                                    " [Sc_Time] INTERGER, 	[Sc] UINT64," \
                                    " [Stot_Time] INTERGER, [Stot] UINT64," \
                                    " [Pa_neg_Time] INTERGER, [Pa_neg] INT64," \
                                    " [Pb_neg_Time] INTERGER, [Pb_neg] INT64," \
                                    " [Pc_neg_Time] INTERGER, [Pc_neg] INT64," \
                                    " [Ptot_neg_Time] INTERGER, [Ptot_neg] INT64," \
                                    " [Qa_neg_Time] INTERGER, [Qa_neg] INT64," \
                                    " [Qb_neg_Time] INTERGER, [Qb_neg] INT64," \
                                    " [Qc_neg_Time] INTERGER, [Qc_neg] INT64," \
                                    " [Qtot_neg_Time] INTERGER, [Qtot_neg] INT64);" 


/*
    ENERGY DATABASE manager
*/
struct db_max_dmd_sql_ctx
{
    uint64_t        id_day;
    uint64_t        id_month;
    pthread_mutex_t lock;
};


/* 动态上下文数组支持多通道 */
static struct db_max_dmd_sql_ctx *db_max_dmd_ctx_array = NULL;
static uint8_t db_max_dmd_ctx_max_channels = 0;

static struct db_max_dmd_sql_ctx* db_max_dmd_get_ctx(uint8_t channel)
{
    if(!db_max_dmd_ctx_array || channel >= db_max_dmd_ctx_max_channels)
        return NULL;
    
    return &db_max_dmd_ctx_array[channel];
}

static int db_max_dmd_ctx_init(uint8_t max_channels)
{
    if(db_max_dmd_ctx_array)
        return 0;
    
    db_max_dmd_ctx_max_channels = max_channels;
    db_max_dmd_ctx_array = (struct db_max_dmd_sql_ctx*)calloc(max_channels, 
                                                               sizeof(struct db_max_dmd_sql_ctx));
    
    if(!db_max_dmd_ctx_array)
    {
        printf("Failed to allocate maxdmd contexts for %d channels\n", max_channels);
        return -1;
    }
    
    for(int i = 0; i < max_channels; i++)
    {
        pthread_mutex_init(&db_max_dmd_ctx_array[i].lock, NULL);
        db_max_dmd_ctx_array[i].id_day = 0;
        db_max_dmd_ctx_array[i].id_month = 0;
    }
    
    printf("MaxDMD contexts initialized for %d channels\n", max_channels);
    return 0;
}

static void db_max_dmd_ctx_destroy(void)
{
    if(!db_max_dmd_ctx_array)
        return;
    
    for(int i = 0; i < db_max_dmd_ctx_max_channels; i++)
    {
        pthread_mutex_destroy(&db_max_dmd_ctx_array[i].lock);
    }
    
    free(db_max_dmd_ctx_array);
    db_max_dmd_ctx_array = NULL;
    db_max_dmd_ctx_max_channels = 0;
}

typedef struct{
        char *new_column_new;
        char *new_column_datatype;
        char *new_column_default;
}new_column_cfg_type;



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
static int db_maxdmd_sql_create(struct db_max_dmd_sql_ctx *pCtx, char *db_name)
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
    rc = sqlite3_exec(db, DB_MAXDMD_SQL_CREATE(day), NULL, NULL, &zerr);
    if (rc != SQLITE_OK)
    {
        printf("%s:%s\n", __FUNCTION__, zerr);
        sqlite3_free(zerr);

        sqlite3_close(db);
        pthread_mutex_unlock(&pCtx->lock);
        return -1;
    }

    // CREATE table
    rc = sqlite3_exec(db, DB_MAXDMD_SQL_CREATE(month), NULL, NULL, &zerr);
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
/** Function Name: db_maxdmd_compatible                                **/
/** Purpose: 兼容旧数据库，添加反向需量字段                              **/
/** Params: pCtx - 上下文, db_name - 数据库文件, db_tablename - 表名   **/
/** Return: 0 成功, -1 失败                                             **/
/** Notice: 检查并添加 Pa_neg, Pb_neg 等8个反向需量最值字段              **/
/*************************************************************************/
static int db_maxdmd_compatible(struct db_max_dmd_sql_ctx *pCtx, const char *db_name, const char *db_tablename)
{
    sqlite3         *db;
    char            sql[512];
    char            *zerr;
    int             rc, iscolumn = 0;
    sqlite3_stmt    *stmt;
    uint32_t        iErrCnt;
    
    // 要添加的新列配置
    new_column_cfg_type new_columns[] = {
        {"Pa_neg_Time",  "INTERGER", "0"},
        {"Pa_neg",       "INT64",    "0"},
        {"Pb_neg_Time",  "INTERGER", "0"},
        {"Pb_neg",       "INT64",    "0"},
        {"Pc_neg_Time",  "INTERGER", "0"},
        {"Pc_neg",       "INT64",    "0"},
        {"Ptot_neg_Time","INTERGER", "0"},
        {"Ptot_neg",     "INT64",    "0"},
        {"Qa_neg_Time",  "INTERGER", "0"},
        {"Qa_neg",       "INT64",    "0"},
        {"Qb_neg_Time",  "INTERGER", "0"},
        {"Qb_neg",       "INT64",    "0"},
        {"Qc_neg_Time",  "INTERGER", "0"},
        {"Qc_neg",       "INT64",    "0"},
        {"Qtot_neg_Time","INTERGER", "0"},
        {"Qtot_neg",     "INT64",    "0"},
        {NULL, NULL, NULL}  // 结束标记
    };

    pthread_mutex_lock(&pCtx->lock);
    rc = sqlite3_open(db_name, &db);
    if (rc)
    {
        sqlite3_close(db);
        pthread_mutex_unlock(&pCtx->lock);
        return -1;
    }

    // 遍历所有新列，检查并添加
    for(int i = 0; new_columns[i].new_column_new != NULL; i++)
    {
        // 检查列是否存在
        snprintf(sql, sizeof(sql), "select * from sqlite_master where name='%s' and sql like '%%%s%%';",
                 db_tablename, new_columns[i].new_column_new);
        
        rc = sqlite3_prepare_v2(db, sql, sizeof(sql), &stmt, NULL);
        if (SQLITE_OK != rc)
        {
            if (stmt)
                sqlite3_finalize(stmt);
            continue;
        }

        iscolumn = 0;
        rc = sqlite3_step(stmt);
        if (SQLITE_ROW == rc)
        {
            iscolumn = 1;  // 列已存在
        }
        sqlite3_finalize(stmt);

        // 找不到该列，说明数据库是旧的，添加新列
        if(iscolumn == 0)
        {
            iErrCnt = 3;
            do 
            {
                snprintf(sql, sizeof(sql), "ALTER TABLE %s ADD %s %s DEFAULT %s;",
                         db_tablename, 
                         new_columns[i].new_column_new,
                         new_columns[i].new_column_datatype,
                         new_columns[i].new_column_default);
                
                rc = sqlite3_exec(db, sql, NULL, NULL, &zerr);
                if(SQLITE_OK != rc)
                {
                    if(!strstr(zerr, "database is locked"))
                        break;
                    
                    usleep(200000);
                }
                else
                {
                    printf("Added column %s to table %s\n", new_columns[i].new_column_new, db_tablename);
                    break;
                }
            } while(iErrCnt--);
            
            if(zerr)
                sqlite3_free(zerr);
        }
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
static int db_maxdmd_sql_get_item(struct db_max_dmd_sql_ctx *pCtx, char *db_name, char *db_tablename, struct db_demand_max_type *pDmd)
{
    sqlite3 *db;
    char sql[2048];
    char *zerr;
    int rc, ret = -1;
    struct tm tm;
    sqlite3_stmt *stmt;
    uint64_t *id;
    struct db_demand_max_base_type *p;

    if(!strcmp(db_tablename, "day"))
    {
        id = &pCtx->id_day;
        p = &pDmd->day;
    }
    else 
    {
        id = &pCtx->id_month;
        p = &pDmd->month;
    }

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
    snprintf(sql, sizeof(sql), "select * from %s order by _Id desc limit 1;" ,
                                                db_tablename);
    rc = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
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
            *id           = sqlite3_column_int64(stmt, 0);
            /*
                current
            */
            p->current.Ia.time = sqlite3_column_int64(stmt, 3);
            p->current.Ia.value = sqlite3_column_int64(stmt, 4);

            p->current.Ib.time = sqlite3_column_int64(stmt, 5);
            p->current.Ib.value = sqlite3_column_int64(stmt, 6);

            p->current.Ic.time = sqlite3_column_int64(stmt, 7);
            p->current.Ic.value = sqlite3_column_int64(stmt, 8);
            
            p->current.IAvg.time = sqlite3_column_int64(stmt, 9);
            p->current.IAvg.value = sqlite3_column_int64(stmt, 10);

            /* power - 兼容性注意：值类型从64位改为32位，自动截断 */
            p->power.Pa.time = sqlite3_column_int64(stmt, 11);
            p->power.Pa.value = (int32_t)sqlite3_column_int64(stmt, 12);

            p->power.Pb.time = sqlite3_column_int64(stmt, 13);
            p->power.Pb.value = (int32_t)sqlite3_column_int64(stmt, 14);

            p->power.Pc.time = sqlite3_column_int64(stmt, 15);
            p->power.Pc.value = (int32_t)sqlite3_column_int64(stmt, 16);

            p->power.Ptot.time = sqlite3_column_int64(stmt, 17);
            p->power.Ptot.value = (int32_t)sqlite3_column_int64(stmt, 18);
            
            p->power.Qa.time = sqlite3_column_int64(stmt, 19);
            p->power.Qa.value = (int32_t)sqlite3_column_int64(stmt, 20);

            p->power.Qb.time = sqlite3_column_int64(stmt, 21);
            p->power.Qb.value = (int32_t)sqlite3_column_int64(stmt, 22);

            p->power.Qc.time = sqlite3_column_int64(stmt, 23);
            p->power.Qc.value = (int32_t)sqlite3_column_int64(stmt, 24);

            p->power.Qtot.time = sqlite3_column_int64(stmt, 25);
            p->power.Qtot.value = (int32_t)sqlite3_column_int64(stmt, 26);
            
            p->power.Sa.time = sqlite3_column_int64(stmt, 27);
            p->power.Sa.value = (uint32_t)sqlite3_column_int64(stmt, 28);

            p->power.Sb.time = sqlite3_column_int64(stmt, 29);
            p->power.Sb.value = (uint32_t)sqlite3_column_int64(stmt, 30);

            p->power.Sc.time = sqlite3_column_int64(stmt, 31);
            p->power.Sc.value = (uint32_t)sqlite3_column_int64(stmt, 32);

            p->power.Stot.time = sqlite3_column_int64(stmt, 33);
            p->power.Stot.value = (uint32_t)sqlite3_column_int64(stmt, 34);
            
            // 反向有功需量最值（2025-10-29 添加）- 兼容性：64位截断到32位
            p->power.Pa_neg.time = sqlite3_column_int64(stmt, 35);
            p->power.Pa_neg.value = (int32_t)sqlite3_column_int64(stmt, 36);
            
            p->power.Pb_neg.time = sqlite3_column_int64(stmt, 37);
            p->power.Pb_neg.value = (int32_t)sqlite3_column_int64(stmt, 38);
            
            p->power.Pc_neg.time = sqlite3_column_int64(stmt, 39);
            p->power.Pc_neg.value = (int32_t)sqlite3_column_int64(stmt, 40);
            
            p->power.Ptot_neg.time = sqlite3_column_int64(stmt, 41);
            p->power.Ptot_neg.value = (int32_t)sqlite3_column_int64(stmt, 42);
            
            // 反向无功需量最值（2025-10-29 添加）- 兼容性：64位截断到32位
            p->power.Qa_neg.time = sqlite3_column_int64(stmt, 43);
            p->power.Qa_neg.value = (int32_t)sqlite3_column_int64(stmt, 44);
            
            p->power.Qb_neg.time = sqlite3_column_int64(stmt, 45);
            p->power.Qb_neg.value = (int32_t)sqlite3_column_int64(stmt, 46);
            
            p->power.Qc_neg.time = sqlite3_column_int64(stmt, 47);
            p->power.Qc_neg.value = (int32_t)sqlite3_column_int64(stmt, 48);
            
            p->power.Qtot_neg.time = sqlite3_column_int64(stmt, 49);
            p->power.Qtot_neg.value = (int32_t)sqlite3_column_int64(stmt, 50);
            
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
static int db_maxdmd_sql_set_item(struct db_max_dmd_sql_ctx *pCtx, char *db_name, char *db_tablename, struct db_database_measure_base_type *pDB)
{
    sqlite3 *db;
    char sql[2048] = {0};
    char *zerr;
    int rc, ret = -1;
    uint32_t iErrCnt;
    sqlite3_stmt *stmt;
    uint64_t *id;
    struct db_demand_max_base_type *p;
    

    if(!strcmp(db_tablename, "day"))
    {
        id = &pCtx->id_day;
        p = &pDB->db_measure.statistical.demand.max.day;
    }
    else 
    {
        id = &pCtx->id_month;
        p = &pDB->db_measure.statistical.demand.max.month;
    }

    pthread_mutex_lock(&pCtx->lock);
    rc = sqlite3_open(db_name, &db);
    if (rc)
    {
        sqlite3_close(db);
        pthread_mutex_unlock(&pCtx->lock);
        return -1;
    }

    *id = *id + 1;
    if(*id > 0xffffffffffffff00)
    {
        /*
            generally speaking, never here in this device's life cycle.
            this just use for repair the energy if some issue.
        */
        unlink(DB_REALTIME_MAXDMD_SQL_MAIN);
        *id = 0;
    }

    /*
        max 10 line of 1 database.
        we just need the line of max(_Id). other line just use for check
    */
    snprintf(sql, sizeof(sql), "replace into %s(_Id,_Range," 
                                                "Ia_Time,Ia,"
                                                "Ib_Time,Ib,"
                                                "Ic_Time,Ic,"
                                                "IAvg_Time,IAvg,"
                                                "Pa_Time,Pa,"
                                                "Pb_Time,Pb,"
                                                "Pc_Time,Pc,"
                                                "PTot_Time,PTot,"
                                                "Qa_Time,Qa,"
                                                "Qb_Time,Qb,"
                                                "Qc_Time,Qc,"
                                                "QTot_Time,QTot,"
                                                "Sa_Time,Sa,"
                                                "Sb_Time,Sb,"
                                                "Sc_Time,Sc,"
                                                "STot_Time,STot,"
                                                "Pa_neg_Time,Pa_neg,"
                                                "Pb_neg_Time,Pb_neg,"
                                                "Pc_neg_Time,Pc_neg,"
                                                "PTot_neg_Time,PTot_neg,"
                                                "Qa_neg_Time,Qa_neg,"
                                                "Qb_neg_Time,Qb_neg,"
                                                "Qc_neg_Time,Qc_neg,"
                                                "QTot_neg_Time,QTot_neg)"
                                                "values("
                                                "%llu,%d,"
                                                "datetime(%u,'unixepoch','localtime'),%u,"
                                                "datetime(%u,'unixepoch','localtime'),%u,"
                                                "datetime(%u,'unixepoch','localtime'),%u,"
                                                "datetime(%u,'unixepoch','localtime'),%u,"
                                                "datetime(%u,'unixepoch','localtime'),%lld,"
                                                "datetime(%u,'unixepoch','localtime'),%lld,"
                                                "datetime(%u,'unixepoch','localtime'),%lld,"
                                                "datetime(%u,'unixepoch','localtime'),%lld,"
                                                "datetime(%u,'unixepoch','localtime'),%lld,"
                                                "datetime(%u,'unixepoch','localtime'),%lld,"
                                                "datetime(%u,'unixepoch','localtime'),%lld,"
                                                "datetime(%u,'unixepoch','localtime'),%lld,"
                                                "datetime(%u,'unixepoch','localtime'),%lld,"
                                                "datetime(%u,'unixepoch','localtime'),%lld,"
                                                "datetime(%u,'unixepoch','localtime'),%lld,"
                                                "datetime(%u,'unixepoch','localtime'),%lld,"
                                                "datetime(%u,'unixepoch','localtime'),%lld,"
                                                "datetime(%u,'unixepoch','localtime'),%lld,"
                                                "datetime(%u,'unixepoch','localtime'),%lld,"
                                                "datetime(%u,'unixepoch','localtime'),%lld,"
                                                "datetime(%u,'unixepoch','localtime'),%lld,"
                                                "datetime(%u,'unixepoch','localtime'),%lld,"
                                                "datetime(%u,'unixepoch','localtime'),%lld,"
                                                "datetime(%u,'unixepoch','localtime'),%lld);",
                                                db_tablename,
                                                *id, (uint16_t)(*id % DB_MAXDMD_NUM_PER_DB),
                                                p->current.Ia.time, p->current.Ia.value,
                                                p->current.Ib.time, p->current.Ib.value,
                                                p->current.Ic.time, p->current.Ic.value,
                                                p->current.IAvg.time, p->current.IAvg.value,
                                                p->power.Pa.time, (int64_t)p->power.Pa.value,
                                                p->power.Pb.time, (int64_t)p->power.Pb.value,
                                                p->power.Pc.time, (int64_t)p->power.Pc.value,
                                                p->power.Ptot.time, (int64_t)p->power.Ptot.value,
                                                p->power.Qa.time, (int64_t)p->power.Qa.value,
                                                p->power.Qb.time, (int64_t)p->power.Qb.value,
                                                p->power.Qc.time, (int64_t)p->power.Qc.value,
                                                p->power.Qtot.time, (int64_t)p->power.Qtot.value,
                                                p->power.Sa.time, (uint64_t)p->power.Sa.value,
                                                p->power.Sb.time, (uint64_t)p->power.Sb.value,
                                                p->power.Sc.time, (uint64_t)p->power.Sc.value,
                                                p->power.Stot.time, (uint64_t)p->power.Stot.value,
                                                p->power.Pa_neg.time, (int64_t)p->power.Pa_neg.value,
                                                p->power.Pb_neg.time, (int64_t)p->power.Pb_neg.value,
                                                p->power.Pc_neg.time, (int64_t)p->power.Pc_neg.value,
                                                p->power.Ptot_neg.time, (int64_t)p->power.Ptot_neg.value,
                                                p->power.Qa_neg.time, (int64_t)p->power.Qa_neg.value,
                                                p->power.Qb_neg.time, (int64_t)p->power.Qb_neg.value,
                                                p->power.Qc_neg.time, (int64_t)p->power.Qc_neg.value,
                                                p->power.Qtot_neg.time, (int64_t)p->power.Qtot_neg.value
                                                );
    printf("%s\n", sql);
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
static int db_maxdmd_load(struct db_max_dmd_sql_ctx *pCtx, struct db_database_measure_base_type *pDB, uint8_t channel)
{ 
    int ret;
    const char *filemain = db_maxdmd_get_main(channel);
    
    if(!pCtx)
        return -1;
    
    pCtx->id_day = 0;
    pCtx->id_month = 0;
    /*
        foreach MAIN and BAKUP file, read the db
    */
    /*
        1. check the PATH
    */
    if(access(DB_MAXDMD_SQL_DIR, F_OK))
        mkdir_fullpath(DB_MAXDMD_SQL_DIR);

    /*
        2. check the main.db and bakup.db
           A. main is OK?  main OK -> finish loading
           B. main is fail, read the bakup. 
           C. bakup is OK?  bakup OK -> finish loading
           D. bakup is fail too, default 0. 

    */

    if(access(filemain, F_OK) == 0)
    {
        // 兼容旧数据库，添加反向需量字段
        db_maxdmd_compatible(pCtx, filemain, "day");
        db_maxdmd_compatible(pCtx, filemain, "month");
        
        ret = db_maxdmd_sql_get_item(pCtx, (char*)filemain, "day", &pDB->db_measure.statistical.demand.max);
        if(ret < 0)
        {
            memset(&pDB->db_measure.statistical.demand.max.day, 0, sizeof(pDB->db_measure.statistical.demand.max.day));
        }

        ret = db_maxdmd_sql_get_item(pCtx, (char*)filemain, "month", &pDB->db_measure.statistical.demand.max);
        if(ret < 0)
        {
            memset(&pDB->db_measure.statistical.demand.max.month, 0, sizeof(pDB->db_measure.statistical.demand.max.month));
        }

        return 0;
    }


    /* No database. clear all the var */
    memset(&pDB->db_measure.statistical.demand.max.day, 0, sizeof(pDB->db_measure.statistical.demand.max.day));
    memset(&pDB->db_measure.statistical.demand.max.month, 0, sizeof(pDB->db_measure.statistical.demand.max.month));

    return -1;
}

/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
int db_maxdmd_default(struct db_database_measure_base_type *pDB, uint8_t channel)
{
    struct db_max_dmd_sql_ctx *pCtx = db_max_dmd_get_ctx(channel);
    
    if(!pCtx)
    {
        printf("db_maxdmd_default: invalid channel %d\n", channel);
        return -1;
    }
    
    unlink(db_maxdmd_get_main(channel));
    db_maxdmd_load(pCtx, pDB, channel);

    return 0;
}



/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
void db_maxdmd_update(struct db_database_measure_base_type *pDB, uint8_t channel)
{
    int ret;
    struct db_max_dmd_sql_ctx *pCtx = db_max_dmd_get_ctx(channel);
    const char *filemain = db_maxdmd_get_main(channel);
    
    if(!pCtx)
    {
        printf("db_maxdmd_update: invalid channel %d\n", channel);
        return;
    }
     
    /* no main.db, create it */
    if(access(filemain, F_OK))
        db_maxdmd_sql_create(pCtx, (char*)filemain);

    /* add one record to main */
    ret = db_maxdmd_sql_set_item(pCtx, (char*)filemain, "day", pDB);
    if(ret < 0)
    {
        /* file exist. but file is error, delete it and re-create it */
        unlink(filemain);
        ret = db_maxdmd_sql_create(pCtx, (char*)filemain);
        db_maxdmd_sql_set_item(pCtx, (char*)filemain, "day", pDB);
    }

    ret = db_maxdmd_sql_set_item(pCtx, (char*)filemain, "month", pDB);
    if(ret < 0)
    {
        /* file exist. but file is error, delete it and re-create it */
        unlink(filemain);
        ret = db_maxdmd_sql_create(pCtx, (char*)filemain);
        db_maxdmd_sql_set_item(pCtx, (char*)filemain, "month", pDB);
    }
}




/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
int db_maxdmd_init(struct db_database_measure_base_type *pDB, uint8_t channel)
{
    int ret;
    struct db_max_dmd_sql_ctx *pCtx;
    
    if(!pDB)
        return -1;

    /* 首次调用时初始化上下文数组 */
    if(!db_max_dmd_ctx_array)
    {
        uint8_t max_channels = db_msr_msg_event_api_get_max_channels();
        if(db_max_dmd_ctx_init(max_channels) < 0)
            return -1;
    }
    
    pCtx = db_max_dmd_get_ctx(channel);
    if(!pCtx)
    {
        printf("db_maxdmd_init: invalid channel %d\n", channel);
        return -1;
    }
    
    /* load file from  */
    db_maxdmd_load(pCtx, pDB, channel);

    return 0;
}



/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
void db_maxdmd_destroy(struct db_database_measure_base_type *pDB, uint8_t channel)
{
    if(!pDB)
        return;

    /* 注意：不销毁单个上下文的 mutex，统一在 cleanup 中处理 */
}

void db_maxdmd_cleanup_all(void)
{
    db_max_dmd_ctx_destroy();
}