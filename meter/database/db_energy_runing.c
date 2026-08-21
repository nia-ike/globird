
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


#define DB_RUNING_ENERGY_NUM_PER_DB    (72)               // 72 hours

#define DB_RUNING_ENERGY_SQL_PATH      "/userdata/energy"
#define DB_RUNING_ENERGY_SQL_MAIN      DB_RUNING_ENERGY_SQL_PATH"/runing_energy_main.db"
#define DB_RUNING_ENERGY_SQL_BAKUP     DB_RUNING_ENERGY_SQL_PATH"/runing_energy_bakup.db"

#define DB_ENERGY_SQL_CREATE "CREATE TABLE IF NOT EXISTS [runing_energy]("                            \
                             "  [_Id] INT64, "                                                        \
                             "  [_Time] TIMESTAMP NOT NULL DEFAULT (DATETIME ('now', 'localtime')), " \
                             "  [_Range] INT PRIMARY KEY UNIQUE ON CONFLICT REPLACE, "                \
                             "  [kWhImp_A] INT64, "                                                   \
                             "  [kWhImp_B] INT64, "                                                   \
                             "  [kWhImp_C] INT64, "                                                   \
                             "  [kWhImp_Tot] INT64, "                                                 \
                             "  [kWhExp_A] INT64, "                                                   \
                             "  [kWhExp_B] INT64, "                                                   \
                             "  [kWhExp_C] INT64, "                                                   \
                             "  [kWhExp_Tot] INT64, "                                                 \
                             "  [kvarhImp_A] INT64, "                                                 \
                             "  [kvarhImp_B] INT64, "                                                 \
                             "  [kvarhImp_C] INT64, "                                                 \
                             "  [kvarhImp_Tot] INT64, "                                               \
                             "  [kvarhExp_A] INT64, "                                                 \
                             "  [kvarhExp_B] INT64, "                                                 \
                             "  [kvarhExp_C] INT64, "                                                 \
                             "  [kvarhExp_Tot] INT64, "                                               \
                             "  [kVAh_A] INT64, "                                                     \
                             "  [kVAh_B] INT64, "                                                     \
                             "  [kVAh_C] INT64, "                                                     \
                             "  [kVAh_Tot] INT64, "                                                   \
                             "  [kWhTot_A] INT64, "                                                     \
                             "  [kWhTot_B] INT64, "                                                     \
                             "  [kWhTot_C] INT64, "                                                     \
                             "  [kWhTot_Tot] INT64, "                                                     \
                             "  [kvarhTot_A] INT64, "                                                     \
                             "  [kvarhTot_B] INT64, "                                                     \
                             "  [kvarhTot_C] INT64, "                                                     \
                             "  [kvarhTot_Tot] INT64);"


/*
    ENERGY DATABASE manager
*/
struct db_runing_energy_sql_ctx
{
    uint64_t        id;
    pthread_mutex_t lock;
};


/* 动态上下文数组支持多通道 */
static struct db_runing_energy_sql_ctx *db_runing_energy_ctx_array = NULL;
static uint8_t db_runing_energy_ctx_max_channels = 0;

static struct db_runing_energy_sql_ctx* db_runing_energy_get_ctx(uint8_t channel)
{
    if(!db_runing_energy_ctx_array || channel >= db_runing_energy_ctx_max_channels)
        return NULL;
    
    return &db_runing_energy_ctx_array[channel];
}

static int db_runing_energy_ctx_init(uint8_t max_channels)
{
    if(db_runing_energy_ctx_array)
        return 0;
    
    db_runing_energy_ctx_max_channels = max_channels;
    db_runing_energy_ctx_array = (struct db_runing_energy_sql_ctx*)calloc(max_channels, 
                                                                           sizeof(struct db_runing_energy_sql_ctx));
    
    if(!db_runing_energy_ctx_array)
    {
        printf("Failed to allocate runing energy contexts for %d channels\n", max_channels);
        return -1;
    }
    
    for(int i = 0; i < max_channels; i++)
    {
        pthread_mutex_init(&db_runing_energy_ctx_array[i].lock, NULL);
        db_runing_energy_ctx_array[i].id = 0;
    }
    
    printf("Runing energy contexts initialized for %d channels\n", max_channels);
    return 0;
}

static void db_runing_energy_ctx_destroy(void)
{
    if(!db_runing_energy_ctx_array)
        return;
    
    for(int i = 0; i < db_runing_energy_ctx_max_channels; i++)
    {
        pthread_mutex_destroy(&db_runing_energy_ctx_array[i].lock);
    }
    
    free(db_runing_energy_ctx_array);
    db_runing_energy_ctx_array = NULL;
    db_runing_energy_ctx_max_channels = 0;
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
static struct db_runing_energy_type *db_runing_energy_get(struct db_database_measure_base_type *pDB)
{
    return &pDB->db_measure.energy.runing_hp;
}

/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
static void db_runing_energy_set(struct db_database_measure_base_type *pDB, struct db_runing_energy_type *pEnergy)
{
    pDB->db_measure.energy.runing_hp = *pEnergy;
}

/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
static int db_runing_energy_sql_create(struct db_runing_energy_sql_ctx *pCtx, char *db_name)
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
    rc = sqlite3_exec(db, DB_ENERGY_SQL_CREATE, NULL, NULL, &zerr);
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
static int db_runing_energy_sql_get_item(struct db_runing_energy_sql_ctx *pCtx, char *db_name, struct db_runing_energy_type *pEnergy)
{
#define DB_RUNING_ENERGY_SQL "select * from runing_energy order by _Id desc limit 1;"
#define DB_RUNING_ENERGY_SQL_LEN sizeof(DB_RUNING_ENERGY_SQL)
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
    rc = sqlite3_prepare_v2(db, DB_RUNING_ENERGY_SQL, DB_RUNING_ENERGY_SQL_LEN, &stmt, NULL);
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
            pCtx->id           = sqlite3_column_int64(stmt, 0);
            pEnergy->kWhImp[0] = (uint32_t)sqlite3_column_int64(stmt, 3);    //"  [kWhImp_A] INT64, "
            pEnergy->kWhImp[1] = (uint32_t)sqlite3_column_int64(stmt, 4);    //"  [kWhImp_B] INT64, "
            pEnergy->kWhImp[2] = (uint32_t)sqlite3_column_int64(stmt, 5);    //"  [kWhImp_C] INT64, "
            pEnergy->kWhImp[3] = (uint32_t)sqlite3_column_int64(stmt, 6);    //"  [kWhImp_Tot] INT64, "
            pEnergy->kWhExp[0] = (uint32_t)sqlite3_column_int64(stmt, 7);    //"  [kWhExp_A] INT64, "
            pEnergy->kWhExp[1] = (uint32_t)sqlite3_column_int64(stmt, 8);    //"  [kWhExp_B] INT64, "
            pEnergy->kWhExp[2] = (uint32_t)sqlite3_column_int64(stmt, 9);    //"  [kWhExp_C] INT64, "
            pEnergy->kWhExp[3] = (uint32_t)sqlite3_column_int64(stmt, 10);   //"  [kWhExp_Tot] INT64, "
            pEnergy->kvarhImp[0] = (uint32_t)sqlite3_column_int64(stmt, 11); //"  [kvarhImp_A] INT64, "
            pEnergy->kvarhImp[1] = (uint32_t)sqlite3_column_int64(stmt, 12); //"  [kvarhImp_B] INT64, "
            pEnergy->kvarhImp[2] = (uint32_t)sqlite3_column_int64(stmt, 13); //"  [kvarhImp_C] INT64, "
            pEnergy->kvarhImp[3] = (uint32_t)sqlite3_column_int64(stmt, 14); //"  [kvarhImp_Tot] INT64, "
            pEnergy->kvarhExp[0] = (uint32_t)sqlite3_column_int64(stmt, 15); //"  [kvarhExp_A] INT64, "
            pEnergy->kvarhExp[1] = (uint32_t)sqlite3_column_int64(stmt, 16); //"  [kvarhExp_B] INT64, "
            pEnergy->kvarhExp[2] = (uint32_t)sqlite3_column_int64(stmt, 17); //"  [kvarhExp_C] INT64, "
            pEnergy->kvarhExp[3] = (uint32_t)sqlite3_column_int64(stmt, 18); //"  [kvarhExp_Tot] INT64, "
            pEnergy->kVAh[0] = (uint32_t)sqlite3_column_int64(stmt, 19);     //"  [kVAh_A] INT64, "
            pEnergy->kVAh[1] = (uint32_t)sqlite3_column_int64(stmt, 20);     //"  [kVAh_B] INT64, "
            pEnergy->kVAh[2] = (uint32_t)sqlite3_column_int64(stmt, 21);     //"  [kVAh_C] INT64, "
            pEnergy->kVAh[3] = (uint32_t)sqlite3_column_int64(stmt, 22);     //"  [kVAh_Tot] INT64, "
            pEnergy->kWhTot[0] = (uint32_t)sqlite3_column_int64(stmt, 23);   //"  [kWhTot] INT64, "
            pEnergy->kWhTot[1] = (uint32_t)sqlite3_column_int64(stmt, 24);   //"  [kWhTot] INT64, "
            pEnergy->kWhTot[2] = (uint32_t)sqlite3_column_int64(stmt, 25);   //"  [kWhTot] INT64, "
            pEnergy->kWhTot[3] = (uint32_t)sqlite3_column_int64(stmt, 26);   //"  [kWhTot] INT64, "
            pEnergy->kvarhTot[0] = (uint32_t)sqlite3_column_int64(stmt, 27); //"  [kvarhTot] INT64);"
            pEnergy->kvarhTot[1] = (uint32_t)sqlite3_column_int64(stmt, 28); //"  [kvarhTot] INT64);"
            pEnergy->kvarhTot[2] = (uint32_t)sqlite3_column_int64(stmt, 29); //"  [kvarhTot] INT64);"
            pEnergy->kvarhTot[3] = (uint32_t)sqlite3_column_int64(stmt, 30); //"  [kvarhTot] INT64);"

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
static int db_runing_energy_sql_set_item(struct db_runing_energy_sql_ctx *pCtx, char *db_name, struct db_runing_energy_type *pEnergy)
{
    sqlite3 *db;
    char sql[2048] = {0};
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
    snprintf(sql, sizeof(sql), "replace into runing_energy(_Id,_Range," 
                                                "kWhImp_A,kWhImp_B,kWhImp_C,kWhImp_Tot,"
                                                "kWhExp_A,kWhExp_B,kWhExp_C,kWhExp_Tot,"
                                                "kvarhImp_A,kvarhImp_B,kvarhImp_C,kvarhImp_Tot,"
                                                "kvarhExp_A,kvarhExp_B,kvarhExp_C,kvarhExp_Tot,"
                                                "kVAh_A,kVAh_B,kVAh_C,kVAh_Tot,"
                                                "kWhTot_A,kWhTot_B,kWhTot_C,kWhTot_Tot,"
                                                "kvarhTot_A,kvarhTot_B,kvarhTot_C,kvarhTot_Tot)"
                                                "values("
                                                "%llu,%d,"
                                                "%llu,%llu,%llu,%llu,"
                                                "%llu,%llu,%llu,%llu,"
                                                "%llu,%llu,%llu,%llu,"
                                                "%llu,%llu,%llu,%llu,"
                                                "%llu,%llu,%llu,%llu,"
                                                "%llu,%llu,%llu,%llu,"
                                                "%llu,%llu,%llu,%llu);",
                                                pCtx->id, pCtx->id % DB_RUNING_ENERGY_NUM_PER_DB,
                                                (uint64_t)pEnergy->kWhImp[0],  (uint64_t)pEnergy->kWhImp[1], (uint64_t)pEnergy->kWhImp[2], (uint64_t)pEnergy->kWhImp[3],
                                                (uint64_t)pEnergy->kWhExp[0], (uint64_t)pEnergy->kWhExp[1], (uint64_t)pEnergy->kWhExp[2], (uint64_t)pEnergy->kWhExp[3],
                                                (uint64_t)pEnergy->kvarhImp[0], (uint64_t)pEnergy->kvarhImp[1], (uint64_t)pEnergy->kvarhImp[2], (uint64_t)pEnergy->kvarhImp[3],
                                                (uint64_t)pEnergy->kvarhExp[0], (uint64_t)pEnergy->kvarhExp[1], (uint64_t)pEnergy->kvarhExp[2], (uint64_t)pEnergy->kvarhExp[3],
                                                (uint64_t)pEnergy->kVAh[0], (uint64_t)pEnergy->kVAh[1], (uint64_t)pEnergy->kVAh[2], (uint64_t)pEnergy->kVAh[3],
                                                (uint64_t)pEnergy->kWhTot[0],(uint64_t)pEnergy->kWhTot[1],(uint64_t)pEnergy->kWhTot[2],(uint64_t)pEnergy->kWhTot[3],
                                                (uint64_t)pEnergy->kvarhTot[0],(uint64_t)pEnergy->kvarhTot[1],(uint64_t)pEnergy->kvarhTot[2],(uint64_t)pEnergy->kvarhTot[3]);

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
static int db_runing_energy_load(struct db_runing_energy_sql_ctx *pCtx, struct db_database_measure_base_type *pDB, uint8_t channel)
{ 
    int ret;
    struct db_runing_energy_type tmpEnergy;
    const char *filemain = db_energy_get_runing_main(channel);

    if(!pCtx)
        return -1;
    
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
        ret = db_runing_energy_sql_get_item(pCtx, (char*)filemain, &tmpEnergy);
        if(ret == 0)
        {
            /* load the GOOD setting */
            db_runing_energy_set(pDB, &tmpEnergy);
            return 0;
        }
    }

    /* if here, can not load setting from MAIN, we need bakup */
    if(access(DB_RUNING_ENERGY_SQL_BAKUP, F_OK) == 0)
    {
        ret = db_runing_energy_sql_get_item(pCtx, DB_RUNING_ENERGY_SQL_BAKUP, &tmpEnergy);
        if(ret == 0)
        {
            /* load the GOOD setting */
            db_runing_energy_set(pDB, &tmpEnergy);
            return 0;
        }
    }
    
    /* if here, both MAIN and BAKUP is fail, we clear the energy */
    memset(&tmpEnergy, 0 , sizeof(tmpEnergy));
    db_runing_energy_set(pDB, &tmpEnergy);

    return -1;
}

/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
int db_runing_energy_default(struct db_database_measure_base_type *pDB, uint8_t channel)
{
    struct db_runing_energy_sql_ctx *pCtx = db_runing_energy_get_ctx(channel);
    
    if(!pCtx)
    {
        printf("db_runing_energy_default: invalid channel %d\n", channel);
        return -1;
    }
    
    unlink(db_energy_get_runing_main(channel));

    db_runing_energy_load(pCtx, pDB, channel);

    return 0;
}



/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
void db_runing_energy_update(struct db_database_measure_base_type *pDB, uint8_t channel)
{
    int ret;
    struct db_runing_energy_sql_ctx *pCtx = db_runing_energy_get_ctx(channel);
    const char *filemain = db_energy_get_runing_main(channel);
    
    if(!pCtx)
    {
        printf("db_runing_energy_update: invalid channel %d\n", channel);
        return;
    }
     
    /* no main.db, create it */
    if(access(filemain, F_OK))
        db_runing_energy_sql_create(pCtx, (char*)filemain);

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
    ret = db_runing_energy_sql_set_item(pCtx, (char*)filemain, db_runing_energy_get(pDB));
    if(ret < 0)
    {
        /* file exist. but file is error, delete it and re-create it */
        unlink(filemain);
        ret = db_runing_energy_sql_create(pCtx, (char*)filemain);
        db_runing_energy_sql_set_item(pCtx, (char*)filemain, db_runing_energy_get(pDB));
    }
}




/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
int db_runing_energy_init(struct db_database_measure_base_type *pDB, uint8_t channel)
{
    int ret;
    struct db_runing_energy_sql_ctx *pCtx;
    
    if(!pDB)
        return -1;

    /* 首次调用时初始化上下文数组 */
    if(!db_runing_energy_ctx_array)
    {
        uint8_t max_channels = db_msr_msg_event_api_get_max_channels();
        if(db_runing_energy_ctx_init(max_channels) < 0)
            return -1;
    }
    
    pCtx = db_runing_energy_get_ctx(channel);
    if(!pCtx)
    {
        printf("db_runing_energy_init: invalid channel %d\n", channel);
        return -1;
    }
    
    /* load file from  */
    db_runing_energy_load(pCtx, pDB, channel);
    
}



/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
int db_runing_energy_destroy(struct db_database_measure_base_type *pDB, uint8_t channel)
{
    if(!pDB)
        return -1;

    /* 注意：不销毁单个上下文的 mutex，统一在 cleanup 中处理 */
    return 0;
}

void db_runing_energy_cleanup_all(void)
{
    db_runing_energy_ctx_destroy();
}