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

#include "db_struct.h"
#include "db_msr_msgevent_api.h"


#define DB_MULTIRATE_ENERGY_NUM_PER_DB    (72)               // x % 4  = x & 0x3.  save 4 energy record



#define DB_MULTIRATE_ENERGY_SQL_CREATE "CREATE TABLE IF NOT EXISTS [multirate_energy]("                     \
                                    " [_Id] INT64,"                                                         \
                                    " [_Time] TIMESTAMP NOT NULL DEFAULT (DATETIME ('now', 'localtime')),"  \
                                    " [_Range] INT PRIMARY KEY UNIQUE ON CONFLICT REPLACE,"                 \
                                    " [KwhImp_sharp0] INT64,"                                                \
                                    " [KwhExp_sharp0] INT64,"                                                \
                                    " [KvarhImp_sharp0] INT64,"                                              \
                                    " [KvarhExp_sharp0] INT64,"                                              \
                                    " [KwhImp_peak0] INT64,"                                                 \
                                    " [KwhExp_peak0] INT64,"                                                 \
                                    " [KvarhImp_peak0] INT64,"                                               \
                                    " [KvarhExp_peak0] INT64,"                                               \
                                    " [KwhImp_flat0] INT64,"                                                 \
                                    " [KwhExp_flat0] INT64,"                                                 \
                                    " [KvarhImp_flat0] INT64,"                                               \
                                    " [KvarhExp_flat0] INT64,"                                               \
                                    " [KwhImp_valley0] INT64,"                                               \
                                    " [KwhExp_valley0] INT64,"                                               \
                                    " [KvarhImp_valley0] INT64,"                                             \
                                    " [KvarhExp_valley0] INT64,"                                             \
                                    " [KwhImp_sharp1] INT64,"                                                \
                                    " [KwhExp_sharp1] INT64,"                                                \
                                    " [KvarhImp_sharp1] INT64,"                                              \
                                    " [KvarhExp_sharp1] INT64,"                                              \
                                    " [KwhImp_peak1] INT64,"                                                 \
                                    " [KwhExp_peak1] INT64,"                                                 \
                                    " [KvarhImp_peak1] INT64,"                                               \
                                    " [KvarhExp_peak1] INT64,"                                               \
                                    " [KwhImp_flat1] INT64,"                                                 \
                                    " [KwhExp_flat1] INT64,"                                                 \
                                    " [KvarhImp_flat1] INT64,"                                               \
                                    " [KvarhExp_flat1] INT64,"                                               \
                                    " [KwhImp_valley1] INT64,"                                               \
                                    " [KwhExp_valley1] INT64,"                                               \
                                    " [KvarhImp_valley1] INT64,"                                             \
                                    " [KvarhExp_valley1] INT64);" 


/*
    ENERGY DATABASE manager
*/
struct db_multirate_energy_sql_ctx
{
    uint64_t        id;
    pthread_mutex_t lock;
};


/* 动态上下文数组支持多通道 */
static struct db_multirate_energy_sql_ctx *db_multirate_energy_ctx_array = NULL;
static uint8_t db_multirate_energy_ctx_max_channels = 0;

static struct db_multirate_energy_sql_ctx* db_multirate_energy_get_ctx(uint8_t channel)
{
    if(!db_multirate_energy_ctx_array || channel >= db_multirate_energy_ctx_max_channels)
        return NULL;
    
    return &db_multirate_energy_ctx_array[channel];
}

static int db_multirate_energy_ctx_init(uint8_t max_channels)
{
    if(db_multirate_energy_ctx_array)
        return 0;
    
    db_multirate_energy_ctx_max_channels = max_channels;
    db_multirate_energy_ctx_array = (struct db_multirate_energy_sql_ctx*)calloc(max_channels, 
                                                                                  sizeof(struct db_multirate_energy_sql_ctx));
    
    if(!db_multirate_energy_ctx_array)
    {
        printf("Failed to allocate multirate energy contexts for %d channels\n", max_channels);
        return -1;
    }
    
    for(int i = 0; i < max_channels; i++)
    {
        pthread_mutex_init(&db_multirate_energy_ctx_array[i].lock, NULL);
        db_multirate_energy_ctx_array[i].id = 0;
    }
    
    printf("Multirate energy contexts initialized for %d channels\n", max_channels);
    return 0;
}

static void db_multirate_energy_ctx_destroy(void)
{
    if(!db_multirate_energy_ctx_array)
        return;
    
    for(int i = 0; i < db_multirate_energy_ctx_max_channels; i++)
    {
        pthread_mutex_destroy(&db_multirate_energy_ctx_array[i].lock);
    }
    
    free(db_multirate_energy_ctx_array);
    db_multirate_energy_ctx_array = NULL;
    db_multirate_energy_ctx_max_channels = 0;
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
static struct db_energy_multirate_type *db_multirate_get(struct db_database_measure_base_type *pDB)
{
    return &pDB->db_measure.energy.multirate_hp;
}

/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
static void db_multirate_set(struct db_database_measure_base_type *pDB, struct db_energy_multirate_type *pEnergy)
{
    pDB->db_measure.energy.multirate_hp = *pEnergy;
}

/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
static int db_multirate_energy_sql_create(struct db_multirate_energy_sql_ctx *pCtx, const char *db_name)
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
    rc = sqlite3_exec(db, DB_MULTIRATE_ENERGY_SQL_CREATE, NULL, NULL, &zerr);
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
static int db_multirate_energy_sql_get_item(struct db_multirate_energy_sql_ctx *pCtx, const char *db_name, struct db_energy_multirate_type *pEnergy)
{
    #define DB_ENERGY_MULTIRATE_SQL "select * from multirate_energy order by _Id desc limit 1;"
    #define DB_ENERGY_MULTIRATE_SQL_LEN sizeof(DB_ENERGY_MULTIRATE_SQL)
    sqlite3 *db;
    char sql[2048];
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
    rc = sqlite3_prepare_v2(db, DB_ENERGY_MULTIRATE_SQL, DB_ENERGY_MULTIRATE_SQL_LEN, &stmt, NULL);
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
            pCtx->id                  = sqlite3_column_int64(stmt, 0);
            pEnergy->rate[0].kWhImp   = (uint64_t)sqlite3_column_int64(stmt, 3);    //"  [Kwh_sharp] INT64 → UINT64"
            pEnergy->rate[0].kWhExp   = (uint64_t)sqlite3_column_int64(stmt, 4);    //"  [Kwh_sharp] INT64 → UINT64"
            pEnergy->rate[0].kvarhImp = (uint64_t)sqlite3_column_int64(stmt, 5);    //"  [Kvarh_sharp] INT64 → UINT64"
            pEnergy->rate[0].kvarhExp = (uint64_t)sqlite3_column_int64(stmt, 6);    //"  [Kvarh_sharp] INT64 → UINT64"
            pEnergy->rate[1].kWhImp   = (uint64_t)sqlite3_column_int64(stmt, 7);    //"  [Kwh_peak] INT64 → UINT64"
            pEnergy->rate[1].kWhExp   = (uint64_t)sqlite3_column_int64(stmt, 8);    //"  [Kwh_peak] INT64 → UINT64"
            pEnergy->rate[1].kvarhImp = (uint64_t)sqlite3_column_int64(stmt, 9);    //"  [Kvarh_peak] INT64 → UINT64"
            pEnergy->rate[1].kvarhExp = (uint64_t)sqlite3_column_int64(stmt, 10);   //"  [Kvarh_peak] INT64 → UINT64"
            pEnergy->rate[2].kWhImp   = (uint64_t)sqlite3_column_int64(stmt, 11);   //"  [Kwh_flat] INT64 → UINT64"
            pEnergy->rate[2].kWhExp   = (uint64_t)sqlite3_column_int64(stmt, 12);   //"  [Kwh_flat] INT64 → UINT64"
            pEnergy->rate[2].kvarhImp = (uint64_t)sqlite3_column_int64(stmt, 13);   //"  [Kvarh_flat] INT64 → UINT64"
            pEnergy->rate[2].kvarhExp = (uint64_t)sqlite3_column_int64(stmt, 14);   //"  [Kvarh_flat] INT64 → UINT64"
            pEnergy->rate[3].kWhImp   = (uint64_t)sqlite3_column_int64(stmt, 15);   //"  [Kwh_valley] INT64 → UINT64"
            pEnergy->rate[3].kWhExp   = (uint64_t)sqlite3_column_int64(stmt, 16);   //"  [Kwh_valley] INT64 → UINT64"
            pEnergy->rate[3].kvarhImp = (uint64_t)sqlite3_column_int64(stmt, 17);   //"  [Kvarh_valley] INT64 → UINT64"
            pEnergy->rate[3].kvarhExp = (uint64_t)sqlite3_column_int64(stmt, 18);   //"  [Kvarh_valley] INT64 → UINT64"

            pEnergy->rate[4].kWhImp   = (uint64_t)sqlite3_column_int64(stmt, 19);   //"  [Kwh_sharp] INT64 → UINT64"
            pEnergy->rate[4].kWhExp   = (uint64_t)sqlite3_column_int64(stmt, 20);   //"  [Kwh_sharp] INT64 → UINT64"
            pEnergy->rate[4].kvarhImp = (uint64_t)sqlite3_column_int64(stmt, 21);   //"  [Kvarh_sharp] INT64 → UINT64"
            pEnergy->rate[4].kvarhExp = (uint64_t)sqlite3_column_int64(stmt, 22);   //"  [Kvarh_sharp] INT64 → UINT64"
            pEnergy->rate[5].kWhImp   = (uint64_t)sqlite3_column_int64(stmt, 23);   //"  [Kwh_peak] INT64 → UINT64"
            pEnergy->rate[5].kWhExp   = (uint64_t)sqlite3_column_int64(stmt, 24);   //"  [Kwh_peak] INT64 → UINT64"
            pEnergy->rate[5].kvarhImp = (uint64_t)sqlite3_column_int64(stmt, 25);   //"  [Kvarh_peak] INT64 → UINT64"
            pEnergy->rate[5].kvarhExp = (uint64_t)sqlite3_column_int64(stmt, 26);   //"  [Kvarh_peak] INT64 → UINT64"
            pEnergy->rate[6].kWhImp   = (uint64_t)sqlite3_column_int64(stmt, 27);   //"  [Kwh_flat] INT64 → UINT64"
            pEnergy->rate[6].kWhExp   = (uint64_t)sqlite3_column_int64(stmt, 28);   //"  [Kwh_flat] INT64 → UINT64"
            pEnergy->rate[6].kvarhImp = (uint64_t)sqlite3_column_int64(stmt, 29);   //"  [Kvarh_flat] INT64 → UINT64"
            pEnergy->rate[6].kvarhExp = (uint64_t)sqlite3_column_int64(stmt, 30);   //"  [Kvarh_flat] INT64 → UINT64"
            pEnergy->rate[7].kWhImp   = (uint64_t)sqlite3_column_int64(stmt, 31);   //"  [Kwh_valley] INT64 → UINT64"
            pEnergy->rate[7].kWhExp   = (uint64_t)sqlite3_column_int64(stmt, 32);   //"  [Kwh_valley] INT64 → UINT64"
            pEnergy->rate[7].kvarhImp = (uint64_t)sqlite3_column_int64(stmt, 33);   //"  [Kvarh_valley] INT64 → UINT64"
            pEnergy->rate[7].kvarhExp = (uint64_t)sqlite3_column_int64(stmt, 34);   //"  [Kvarh_valley] INT64 → UINT64"
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
static int db_multirate_energy_sql_set_item(struct db_multirate_energy_sql_ctx *pCtx, const char *db_name, struct db_energy_multirate_type *pEnergy)
{
    sqlite3 *db;
    char sql[2048];
    char *zerr;
    int rc, ret = -1;
    uint32_t iErrCnt;
    uint64_t *id;
    sqlite3_stmt *stmt;


    pthread_mutex_lock(&pCtx->lock);
    rc = sqlite3_open(db_name, &db);
    if (rc)
    {
        sqlite3_close(db);
        pthread_mutex_unlock(&pCtx->lock);
        return -1;
    }

//    *id = *id + 1;
    /*
        max [maxItem] line of 1 database.
        we just need the line of max(_Id). other line just use for check
    */
    snprintf(sql, sizeof(sql), "replace into multirate_energy(_Id,_Range," 
                                                "KwhImp_sharp0,KwhExp_sharp0,KvarhImp_sharp0,KvarhExp_sharp0,"
                                                "KwhImp_peak0,KwhExp_peak0,KvarhImp_peak0,KvarhExp_peak0,"
                                                "KwhImp_flat0,KwhExp_flat0,KvarhImp_flat0,KvarhExp_flat0,"
                                                "KwhImp_valley0,KwhExp_valley0,KvarhImp_valley0,KvarhExp_valley0,"
                                                "KwhImp_sharp1,KwhExp_sharp1,KvarhImp_sharp1,KvarhExp_sharp1,"
                                                "KwhImp_peak1,KwhExp_peak1,KvarhImp_peak1,KvarhExp_peak1,"
                                                "KwhImp_flat1,KwhExp_flat1,KvarhImp_flat1,KvarhExp_flat1,"
                                                "KwhImp_valley1,KwhExp_valley1,KvarhImp_valley1,KvarhExp_valley1)"
                                                "values("
                                                "%llu,%d,"
                                                "%llu,%llu,%llu,%llu,"
                                                "%llu,%llu,%llu,%llu,"
                                                "%llu,%llu,%llu,%llu,"
                                                "%llu,%llu,%llu,%llu,"
                                                "%llu,%llu,%llu,%llu,"
                                                "%llu,%llu,%llu,%llu,"
                                                "%llu,%llu,%llu,%llu,"
                                                "%llu,%llu,%llu,%llu);",
                                                pCtx->id, pCtx->id % DB_MULTIRATE_ENERGY_NUM_PER_DB,
                                                (uint64_t)pEnergy->rate[0].kWhImp, (uint64_t)pEnergy->rate[0].kWhExp, (uint64_t)pEnergy->rate[0].kvarhImp, (uint64_t)pEnergy->rate[0].kvarhExp,
                                                (uint64_t)pEnergy->rate[1].kWhImp, (uint64_t)pEnergy->rate[1].kWhExp, (uint64_t)pEnergy->rate[1].kvarhImp, (uint64_t)pEnergy->rate[1].kvarhExp,
                                                (uint64_t)pEnergy->rate[2].kWhImp, (uint64_t)pEnergy->rate[2].kWhExp, (uint64_t)pEnergy->rate[2].kvarhImp, (uint64_t)pEnergy->rate[2].kvarhExp,
                                                (uint64_t)pEnergy->rate[3].kWhImp, (uint64_t)pEnergy->rate[3].kWhExp, (uint64_t)pEnergy->rate[3].kvarhImp, (uint64_t)pEnergy->rate[3].kvarhExp,
                                                (uint64_t)pEnergy->rate[4].kWhImp, (uint64_t)pEnergy->rate[4].kWhExp, (uint64_t)pEnergy->rate[4].kvarhImp, (uint64_t)pEnergy->rate[4].kvarhExp,
                                                (uint64_t)pEnergy->rate[5].kWhImp, (uint64_t)pEnergy->rate[5].kWhExp, (uint64_t)pEnergy->rate[5].kvarhImp, (uint64_t)pEnergy->rate[5].kvarhExp,
                                                (uint64_t)pEnergy->rate[6].kWhImp, (uint64_t)pEnergy->rate[6].kWhExp, (uint64_t)pEnergy->rate[6].kvarhImp, (uint64_t)pEnergy->rate[6].kvarhExp,
                                                (uint64_t)pEnergy->rate[7].kWhImp, (uint64_t)pEnergy->rate[7].kWhExp, (uint64_t)pEnergy->rate[7].kvarhImp, (uint64_t)pEnergy->rate[7].kvarhExp);

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
int db_multirate_energy_load(struct db_multirate_energy_sql_ctx *pCtx, struct db_database_measure_base_type *pDB, const char *filemain, const char *filebakup)
{ 
    int ret;
    struct db_energy_multirate_type tmpMultirateEnergy;

    pCtx->id = 0;

    /*
        foreach MAIN and BAKUP file, read the db
    */
    /*
        1. check the PATH
    */
    if(access(DB_ENERGY_SQL_DIR, F_OK))
        mkdir_fullpath(DB_ENERGY_SQL_DIR);

  /*
        2. check the main.db and bakup.db
           A. main is OK?  main OK -> finish loading
           B. main is fail, read the bakup. 
           C. bakup is OK?  bakup OK -> finish loading
           D. bakup is fail too, default 0. 

    */
    if(access(filemain, F_OK) == 0)
    {
        ret = db_multirate_energy_sql_get_item(pCtx, filemain, &tmpMultirateEnergy);
        if(ret == 0)
        {
            /* load the GOOD setting */
            db_multirate_set(pDB, &tmpMultirateEnergy);
            return 0;
        }
    }

    /* if here, can not load setting from MAIN, we need bakup */
    if(access(filebakup, F_OK) == 0)
    {
        ret = db_multirate_energy_sql_get_item(pCtx, filebakup, &tmpMultirateEnergy);
        if(ret == 0)
        {
            /* load the GOOD setting */
            db_multirate_set(pDB, &tmpMultirateEnergy);
            return 0;
        }
    }
    
    /* if here, both MAIN and BAKUP is fail, we clear the energy */
    memset(&tmpMultirateEnergy, 0 , sizeof(tmpMultirateEnergy));
    db_multirate_set(pDB, &tmpMultirateEnergy);


    return -1;
}



/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
int db_multirate_energy_default(struct db_database_measure_base_type *pDB, uint8_t channel, const char *filemain, const char *filebakup)
{
    struct db_multirate_energy_sql_ctx *pCtx = db_multirate_energy_get_ctx(channel);
    
    if(!pCtx)
    {
        printf("db_multirate_energy_default: invalid channel %d\n", channel);
        return -1;
    }
    
    unlink(filemain);
    unlink(filebakup);

    db_multirate_energy_load(pCtx, pDB, filemain, filebakup);

    return 0;
}



/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
void db_multirate_energy_update(struct db_database_measure_base_type *pDB, uint8_t channel, const char *filemain, const char *filebakup)
{
    db_multirate_energy_update_main(pDB, channel);
    db_multirate_energy_update_backup(pDB, channel);
}

void db_multirate_energy_update_main(struct db_database_measure_base_type *pDB, uint8_t channel)
{
    int ret;
    const char *filemain = db_energy_get_multirate_main(channel);  // 生成路径后立即使用
    struct db_multirate_energy_sql_ctx *pCtx = db_multirate_energy_get_ctx(channel);
    
    if(!pCtx)
    {
        printf("db_multirate_energy_update_main: invalid channel %d\n", channel);
        return;
    }
     
    /* no main.db, create it */
    if(access(filemain, F_OK))
        db_multirate_energy_sql_create(pCtx, filemain);

    pCtx->id++;
    if(pCtx->id > 0xffffffffffffff00)
    {
        /*
            generally speaking, never here in this device's life cycle.
            this just use for repair the energy if some issue.
        */
        unlink(filemain);
        pCtx->id = 0;
    }

    /* add one record to main */
    ret = db_multirate_energy_sql_set_item(pCtx, filemain, db_multirate_get(pDB));
    if(ret < 0)
    {
        /* file exist. but file is error, delete it and re-create it */
        unlink(filemain);
        ret = db_multirate_energy_sql_create(pCtx, filemain);
        db_multirate_energy_sql_set_item(pCtx, filemain, db_multirate_get(pDB));
    }
}

void db_multirate_energy_update_backup(struct db_database_measure_base_type *pDB, uint8_t channel)
{
    int ret;
    const char *filebakup = db_energy_get_multirate_backup(channel);  // 生成路径后立即使用
    struct db_multirate_energy_sql_ctx *pCtx = db_multirate_energy_get_ctx(channel);
    
    if(!pCtx)
    {
        printf("db_multirate_energy_update_backup: invalid channel %d\n", channel);
        return;
    }
    
    /* no bakup.db, create it */
    if(access(filebakup, F_OK))
        db_multirate_energy_sql_create(pCtx, filebakup);
    
    /* add one record to bakup */
    ret = db_multirate_energy_sql_set_item(pCtx, filebakup, db_multirate_get(pDB));
    if(ret < 0)
    {
        /* file exist. but file is error, delete it and re-create it */
        unlink(filebakup);
        ret = db_multirate_energy_sql_create(pCtx, filebakup);
        db_multirate_energy_sql_set_item(pCtx, filebakup, db_multirate_get(pDB));
    }
}


/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
int db_multirate_energy_init(struct db_database_measure_base_type *pDB, uint8_t channel, const char *filemain, const char *filebakup)
{
    int ret;
    struct db_multirate_energy_sql_ctx *pCtx;
    
    if(!pDB)
        return -1;

    /* 首次调用时初始化上下文数组 */
    if(!db_multirate_energy_ctx_array)
    {
        uint8_t max_channels = db_msr_msg_event_api_get_max_channels();
        if(db_multirate_energy_ctx_init(max_channels) < 0)
            return -1;
    }
    
    pCtx = db_multirate_energy_get_ctx(channel);
    if(!pCtx)
    {
        printf("db_multirate_energy_init: invalid channel %d\n", channel);
        return -1;
    }
    
    /* load file from  */
    db_multirate_energy_load(pCtx, pDB, filemain, filebakup);

    return 0;
}



/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
int db_multirate_energy_destroy(struct db_database_measure_base_type *pDB, uint8_t channel)
{
    if(!pDB)
        return -1;
    
    /* 注意：不销毁单个上下文的 mutex，统一在 cleanup 中处理 */
    return 0;
}

void db_multirate_energy_cleanup_all(void)
{
    db_multirate_energy_ctx_destroy();
}
