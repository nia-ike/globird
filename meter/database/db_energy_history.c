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
#include <stdint.h>
#include <stddef.h>
#include <dirent.h>
#include <sqlite3.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>
#include <sys/stat.h>  //stat
#include <sys/types.h> 

#include "db_measure.h"
#include "db_struct.h"
#include "db_msr_msgevent_api.h"




#define DB_HISTORY_SQL_CREATE(__CLASS__) "CREATE TABLE IF NOT EXISTS [" \
                                    #__CLASS__ \
                                    "](" \
                                    " [_Id] INT64," \
                                    " [_Time] TIMESTAMP NOT NULL DEFAULT (DATETIME ('now', 'localtime'))," \
                                    " [_Range] INT PRIMARY KEY UNIQUE ON CONFLICT REPLACE," \
                                    " [kWhTot] INT64," \
                                    " [kvarhTot] INT64," \
                                    " [reserved] INT64," \
                                    " [kWhImp] INT64," \
                                    " [kWhExp] INT64," \
                                    " [kvarhImp] INT64," \
                                    " [kvarhExp] INT64);" 



/*
    ENERGY DATABASE manager
*/
struct db_history_energy_sql_ctx
{
    uint64_t        id_day;
    uint64_t        id_month;
    uint64_t        id_year;
    pthread_mutex_t lock;
};


/* 动态上下文数组支持多通道 */
static struct db_history_energy_sql_ctx *db_history_energy_ctx_array = NULL;
static uint8_t db_history_energy_ctx_max_channels = 0;

static struct db_history_energy_sql_ctx* db_history_energy_get_ctx(uint8_t channel)
{
    if(!db_history_energy_ctx_array || channel >= db_history_energy_ctx_max_channels)
        return NULL;
    
    return &db_history_energy_ctx_array[channel];
}

static int db_history_energy_ctx_init(uint8_t max_channels)
{
    if(db_history_energy_ctx_array)
        return 0;
    
    db_history_energy_ctx_max_channels = max_channels;
    db_history_energy_ctx_array = (struct db_history_energy_sql_ctx*)calloc(max_channels, 
                                                                             sizeof(struct db_history_energy_sql_ctx));
    
    if(!db_history_energy_ctx_array)
    {
        printf("Failed to allocate history energy contexts for %d channels\n", max_channels);
        return -1;
    }
    
    for(int i = 0; i < max_channels; i++)
    {
        pthread_mutex_init(&db_history_energy_ctx_array[i].lock, NULL);
        db_history_energy_ctx_array[i].id_day = 0;
        db_history_energy_ctx_array[i].id_month = 0;
        db_history_energy_ctx_array[i].id_year = 0;
    }
    
    printf("History energy contexts initialized for %d channels\n", max_channels);
    return 0;
}

static void db_history_energy_ctx_destroy(void)
{
    if(!db_history_energy_ctx_array)
        return;
    
    for(int i = 0; i < db_history_energy_ctx_max_channels; i++)
    {
        pthread_mutex_destroy(&db_history_energy_ctx_array[i].lock);
    }
    
    free(db_history_energy_ctx_array);
    db_history_energy_ctx_array = NULL;
    db_history_energy_ctx_max_channels = 0;
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
static struct db_energy_history_type *db_history_energy_get(struct db_database_measure_base_type *pDB)
{
    return &pDB->db_measure.energy.history;
}

/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
static void history_energy_set(struct db_database_measure_base_type *pDB, struct db_energy_history_type *pEnergy)
{
    pDB->db_measure.energy.history = *pEnergy;
}


/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
static int db_history_energy_sql_create(struct db_history_energy_sql_ctx *pCtx, const char *db_name)
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
    rc = sqlite3_exec(db, DB_HISTORY_SQL_CREATE(day), NULL, NULL, &zerr);
    if (rc != SQLITE_OK)
    {
        printf("%s:%s\n", __FUNCTION__, zerr);
        sqlite3_free(zerr);

        sqlite3_close(db);
        pthread_mutex_unlock(&pCtx->lock);
        return -1;
    }

    // CREATE table
    rc = sqlite3_exec(db, DB_HISTORY_SQL_CREATE(month), NULL, NULL, &zerr);
    if (rc != SQLITE_OK)
    {
        printf("%s:%s\n", __FUNCTION__, zerr);
        sqlite3_free(zerr);

        sqlite3_close(db);
        pthread_mutex_unlock(&pCtx->lock);
        return -1;
    }

    // CREATE table
    rc = sqlite3_exec(db, DB_HISTORY_SQL_CREATE(year), NULL, NULL, &zerr);
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
static int db_history_energy_sql_get_item(struct db_history_energy_sql_ctx *pCtx, const char *db_name, const char *db_tablename, uint32_t maxItem, struct db_energy_history_type *pEnergy)
{
    sqlite3 *db;
    char sql[2048];
    char *zerr;
    int rc, ret = -1;
    sqlite3_stmt *stmt;
    struct tm tm;
    struct db_history_energy_commond_t *p;
    uint64_t *id;

    if(maxItem > DB_HISTORY_ENERGY_DAY)
        return -1;

    switch (maxItem)
    {
    case DB_HISTORY_ENERGY_DAY:
        p = (struct db_history_energy_commond_t *)&pEnergy->day;
        id = &pCtx->id_day;
        break;
    case DB_HISTORY_ENERGY_MONTH:
        p = (struct db_history_energy_commond_t *)&pEnergy->month;
        id = &pCtx->id_month;
        break;
    case DB_HISTORY_ENERGY_YEAR:
        p = (struct db_history_energy_commond_t *)&pEnergy->year;
        id = &pCtx->id_year;
        break;
    default:
        return -1;
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
        max [maxItem] line of 1 database.
        we just need the line of max(_Id). other line just use for check
    */
    snprintf(sql, sizeof(sql), "select * from %s order by _Time desc limit %d;" ,
                                                db_tablename, maxItem);
    /*
        max 10 line of 1 database.
        we just need the line of max(_Id). other line just use for check
    */
    rc = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        if (stmt)
            sqlite3_finalize(stmt);

        sqlite3_close(db);
        pthread_mutex_unlock(&pCtx->lock);
        return -1;
    }

    p->nItem = 0;

    do
    {
        rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW)
        {
            if(p->nItem > maxItem)
                break;

            if(p->nItem == 0)
                *id                       = sqlite3_column_int64(stmt, 0);
            strptime(sqlite3_column_text(stmt, 1), "%Y-%m-%d %H:%M:%S", &tm);
            p->record[p->nItem].time  = mktime(&tm);
            p->record[p->nItem].kWh   = sqlite3_column_int64(stmt, 3);
            p->record[p->nItem].kvarh = sqlite3_column_int64(stmt, 4);
            p->nItem++;
            
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
static int db_history_energy_sql_set_item(struct db_history_energy_sql_ctx *pCtx, const char *db_name, const char *db_tablename, uint32_t maxItem, struct db_database_measure_base_type *pDB)
{
    sqlite3 *db;
    char sql[2048];
    char *zerr;
    int rc, ret = -1;
    uint32_t iErrCnt;
    uint64_t *id;
    sqlite3_stmt *stmt;
    struct db_realtime_energy_type *pEnergy = &pDB->db_measure.energy.realtime;

    switch (maxItem)
    {
    case DB_HISTORY_ENERGY_DAY:
        id = &pCtx->id_day;
        break;
    case DB_HISTORY_ENERGY_MONTH:
        id = &pCtx->id_month;
        break;
    case DB_HISTORY_ENERGY_YEAR:
        id = &pCtx->id_year;
        break;
    default:
        return -1;
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
    /*
        max [maxItem] line of 1 database.
        we just need the line of max(_Id). other line just use for check
    */
    snprintf(sql, sizeof(sql), "replace into %s(_Id,_Range," 
                                                "kWhTot,kvarhTot,reserved,"
                                                "kWhImp,kWhExp,"
                                                "kvarhImp,kvarhExp)"
                                                "values("
                                                "%llu,%d,"
                                                "%llu,%llu,%llu,"
                                                "%llu,%llu,"
                                                "%llu,%llu);",
                                                db_tablename, 
                                                *id, *id % maxItem,
                                                (uint64_t)pEnergy->kWhTot[3], (uint64_t)pEnergy->kvarhTot[3], (uint64_t)pEnergy->kVAh[3],
                                                (uint64_t)pEnergy->kWhImp[3], (uint64_t)pEnergy->kWhExp[3],
                                                (uint64_t)pEnergy->kvarhImp[3], (uint64_t)pEnergy->kvarhExp[3]);

//    printf("%s\n", sql); 
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


/*********************************************************************************************************
** Function name:       
** Descriptions:        兼容性处理。 老版本数据库字段没有kWh_imp，kWhExp，karh_imp,kvarhExp， 新版本需要kWh_imp，kWhExp，karh_imp,kvarhExp。
                        这里会导致升级后，删除掉PQ事件所有记录，重新生成数据库文件
** input parameters:    none
** output parameters:   none
** Returned value:      none
*********************************************************************************************************/
static int db_energy_compatible(struct db_database_measure_base_type *pDB, struct db_history_energy_sql_ctx *pCtx, const char *db_name, const char *db_tablename)
{
    sqlite3         *db;
    char            sql[256];
    char            *zerr;
    int             rc, iscolumn = 0;
    sqlite3_stmt    *stmt;
    uint32_t iErrCnt;

    #define DB_ENERGY_KWH_IMP_TABLE_COLUMN    "select * from sqlite_master where name='day' and sql like '\%kWhImp\%';"
    #define DB_ENERGY_KWH_EXP_TABLE_COLUMN    "select * from sqlite_master where name='day' and sql like '\%kWhExp\%';"
    #define DB_ENERGY_KVARH_IMP_TABLE_COLUMN  "select * from sqlite_master where name='day' and sql like '\%kvarhImp\%';"
    #define DB_ENERGY_KVARH_EXP_TABLE_COLUMN  "select * from sqlite_master where name='day' and sql like '\%kvarhExp\%';"


    #define DB_ENERGY_KWH_IMP_TABLE_ADD    "alter table day add  kWhImp;"
    #define DB_ENERGY_KWH_EXP_TABLE_ADD    "alter table day add  kWhExp;"
    #define DB_ENERGY_KVARH_IMP_TABLE_ADD    "alter table day add  kvarhImp;"
    #define DB_ENERGY_KVARH_EXP_TABLE_ADD    "alter table day add  kvarhExp;"

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
    snprintf(sql, sizeof(sql), "select * from sqlite_master where name=\'%s\' and sql like \'%ckWhImp%c\';",db_tablename,'%','%');

    rc = sqlite3_prepare_v2(db, sql, sizeof(sql), &stmt, NULL);

    if (SQLITE_OK != rc)
    {
        if (stmt)
        {
            sqlite3_finalize(stmt);
        }

        sqlite3_close(db);
        pthread_mutex_unlock(&pCtx->lock);
        return -1;
    }

    do {
        rc = sqlite3_step(stmt);
       if (SQLITE_ROW == rc)
        {
            iscolumn++;
            break;
        }
        else if (SQLITE_DONE == rc)
        {
            break;
        }
        else
        {
            break;
        }
    }while(1);


    /* 找不到该列，说明数据库是旧的，插入新列*/
    if(iscolumn == 0)
    {
        iErrCnt = 3;
        do 
        {
            snprintf(sql, sizeof(sql), "ALTER TABLE %s ADD kWhImp INT64 DEFAULT 0;",
                                                db_tablename);
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

        iErrCnt = 3;
        do 
        {
            snprintf(sql, sizeof(sql), "ALTER TABLE %s ADD kWhExp INT64 DEFAULT 0;",
                                                db_tablename);
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

        iErrCnt = 3;
        do 
        {
            snprintf(sql, sizeof(sql), "ALTER TABLE %s ADD kvarhImp INT64 DEFAULT 0;",
                                                db_tablename);
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

        iErrCnt = 3;
        do 
        {
            snprintf(sql, sizeof(sql), "ALTER TABLE %s ADD kvarhExp INT64 DEFAULT 0;",
                                                db_tablename);
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


        sqlite3_finalize(stmt);
        sqlite3_close(db);
        pthread_mutex_unlock(&pCtx->lock);

        return 1;
    }

    sqlite3_finalize(stmt);
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
int db_history_energy_load(struct db_database_measure_base_type *pDB, uint8_t channel, const char *filemain)
{ 
    int ret;
    struct db_history_energy_sql_ctx *pCtx = db_history_energy_get_ctx(channel);
    
    if(!pCtx)
    {
        printf("db_history_energy_load: invalid channel %d\n", channel);
        return -1;
    }

    pCtx->id_day = 0;
    pCtx->id_month = 0;
    pCtx->id_year = 0;

    /*
        foreach MAIN and BAKUP file, read the db
    */
    /*
        1. check the PATH
    */
    if(access(DB_ENERGY_SQL_PATH, F_OK))
        mkdir_fullpath(DB_ENERGY_SQL_PATH);


    if(access(filemain, F_OK) == 0)
    {
        /*
        兼容旧数据库，数据库中新添加kWhImp，kWhExp，kvarhImp，kvarhExp 字段
        */
        db_energy_compatible(pDB, pCtx, filemain, "day"); 
        db_energy_compatible(pDB, pCtx, filemain, "month"); 
        db_energy_compatible(pDB, pCtx, filemain, "year"); 
    }
    /*
        2. check the main.db

    */
    if(access(filemain, F_OK) == 0)
    {
        ret = db_history_energy_sql_get_item(pCtx, filemain, "day", DB_HISTORY_ENERGY_DAY, db_history_energy_get(pDB));
//        printf("DB_HISTORY_ENERGY_DAY:%d \n",ret);
        if(ret < 0)
        {
            memset(&pDB->db_measure.energy.history.day, 0, sizeof(pDB->db_measure.energy.history.day));
        }

        ret = db_history_energy_sql_get_item(pCtx, filemain, "month", DB_HISTORY_ENERGY_MONTH, db_history_energy_get(pDB));
//        printf("DB_HISTORY_ENERGY_MONTH:%d \n",ret);
        if(ret < 0)
        {
            memset(&pDB->db_measure.energy.history.month, 0, sizeof(pDB->db_measure.energy.history.month));
        }

        ret = db_history_energy_sql_get_item(pCtx, filemain, "year", DB_HISTORY_ENERGY_YEAR, db_history_energy_get(pDB));
//        printf("DB_HISTORY_ENERGY_YEAR:%d \n",ret);
        if(ret < 0)
        {
            memset(&pDB->db_measure.energy.history.year, 0, sizeof(pDB->db_measure.energy.history.year));
        }


        return 0;
    }

    /* No database. clear all the var */
    memset(&pDB->db_measure.energy.history.day,   0, sizeof(pDB->db_measure.energy.history.day));
    memset(&pDB->db_measure.energy.history.month, 0, sizeof(pDB->db_measure.energy.history.month));
    memset(&pDB->db_measure.energy.history.year,  0, sizeof(pDB->db_measure.energy.history.year));
    /* err */

    return -1;
}



/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
void db_history_energy_process(struct db_database_measure_base_type *pDB, uint8_t channel, const char *filemain)
{
    static time_t   last[32] = {0};  // 每个通道独立的 last 时间
    int             ret; 
    time_t          now;
    struct tm       last_tm, now_tm;
    struct db_history_energy_sql_ctx *pCtx = db_history_energy_get_ctx(channel);
    
    if(!pCtx)
    {
        printf("db_history_energy_process: invalid channel %d\n", channel);
        return;
    }
    
    now     = time(NULL);
    localtime_r(&now, &now_tm);

    /*
        new day.   hour = 0, minute = 0
    */
    if((now_tm.tm_hour == 0) && (now_tm.tm_min == 0))
    {
        /* 
            just only "now"'s hour and minute is different with "last", it will record the history 
         */
        localtime_r(&last[channel], &last_tm);
        if((last_tm.tm_hour != 0) || (last_tm.tm_min != 0))
        {
            /* 
                record new DAY history
            */

            /* no main.db, create it */
            if(access(filemain, F_OK))
                db_history_energy_sql_create(pCtx, filemain); 

            /* add one record to main */
            ret = db_history_energy_sql_set_item(pCtx, filemain,  "day", DB_HISTORY_ENERGY_DAY, pDB);
            if(ret < 0)
            {
                /* file exist. but file is error, delete it and re-create it */
                unlink(filemain);
                ret = db_history_energy_sql_create(pCtx, filemain);
                db_history_energy_sql_set_item(pCtx, filemain,  "day", DB_HISTORY_ENERGY_DAY, pDB);
            }


            /*
                new month
            */
            if((now_tm.tm_mday == 1))
            {
                /* add one record to main */
                ret = db_history_energy_sql_set_item(pCtx, filemain,  "month", DB_HISTORY_ENERGY_MONTH, pDB);
                if(ret < 0)
                {
                    /* file exist. but file is error, delete it and re-create it */
                    unlink(filemain);
                    ret = db_history_energy_sql_create(pCtx, filemain);
                    db_history_energy_sql_set_item(pCtx, filemain,  "month", DB_HISTORY_ENERGY_MONTH, pDB);
                }
                /*
                    new month
                */
                if((now_tm.tm_mon == 0))
                {
                    /* add one record to main */
                    ret = db_history_energy_sql_set_item(pCtx, filemain,  "year", DB_HISTORY_ENERGY_YEAR, pDB);
                    if(ret < 0)
                    {
                        /* file exist. but file is error, delete it and re-create it */
                        unlink(filemain);
                        ret = db_history_energy_sql_create(pCtx, filemain);
                        db_history_energy_sql_set_item(pCtx, filemain,  "year", DB_HISTORY_ENERGY_YEAR, pDB);
                    }
                }
            }

            /* re-initial */
            db_history_energy_load(pDB, channel, filemain);

        }

    }

    /* update the "last" time */
    last[channel] = now;

}


/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
int db_history_energy_default(struct db_database_measure_base_type *pDB, uint8_t channel, const char *filemain)
{
    unlink(filemain);

   /* re-initial */
    db_history_energy_load(pDB, channel, filemain);

    return 0;
}

/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
int db_history_energy_init(struct db_database_measure_base_type *pDB, uint8_t channel, const char *filemain)
{
    int ret;
    struct db_history_energy_sql_ctx *pCtx;
    
    if(!pDB)
        return -1;

    /* 首次调用时初始化上下文数组 */
    if(!db_history_energy_ctx_array)
    {
        uint8_t max_channels = db_msr_msg_event_api_get_max_channels();
        if(db_history_energy_ctx_init(max_channels) < 0)
            return -1;
    }
    
    pCtx = db_history_energy_get_ctx(channel);
    if(!pCtx)
    {
        printf("db_history_energy_init: invalid channel %d\n", channel);
        return -1;
    }
    
    /* load file from  */
    db_history_energy_load(pDB, channel, filemain);
    
    return 0;
}


/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
void db_history_energy_destroy(struct db_database_measure_base_type *pDB, uint8_t channel)
{
    if(!pDB)
        return;

    /* 注意：不销毁单个上下文的 mutex，统一在 cleanup 中处理 */
}

void db_history_energy_cleanup_all(void)
{
    db_history_energy_ctx_destroy();
}