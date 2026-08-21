/*
* Copyright (c) 2013,珠海一构物联有限公司
* All rights reserved.
* 
* 文件名称：
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
#include <sys/types.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <sys/stat.h>  //stat
#include <time.h>
#include <stdbool.h>

#include "dc_cfg_attr.h"
#include "dc_sql_cache.h"
#include "db_device.h"
#include "db_node_fops.h"
#include "dc_lz4.h"

#include "lz4.h"
#include "sqlite3.h"

#define DC_CFG_FILE "/userdata/config/project/data_cache.json"


// shows how to use this program
/*************************************************************************/
/** Function Name:                                     **/
/** Purpose:                                         **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
static void usage(char *_name)
{
	fprintf(stderr,"\nUsage: %s [options] \n",_name);
    fprintf(stderr,"  -i 300              : the interval for record.\n");
	fprintf(stderr,"  -d                  : debug\n");
	fprintf(stderr,"  -h                  : Show this help message\n");
}


/*************************************************************************/
/** Function Name:                                     **/
/** Purpose:                                         **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
static int dc_cache_record_process(struct db_spl_realtime_data_tbl_t *pDB, int debug)
{
    int ret = -1;
    int origin_size;
    struct dc_sql_cache_item_t cache_item;

    origin_size = DataMgmtFops.map_active_node_size();
    if(origin_size <= 0)
        return -1;

    memset(&cache_item, 0, sizeof(cache_item));
    cache_item.blob_size = dc_lz4_compress((uint8_t *)pDB, origin_size, cache_item.blob, sizeof(cache_item.blob));
    if(cache_item.blob_size <= 0)
        return -1;

    ret = dc_sql_cache_set_item(&cache_item, DataMgmtFops.map_node_num());
    if(ret == DC_SQL_CACHE_RET_FULL)
    {
        /* 日模式已达高水位：当天停止继续写入 */
        if(debug)
            printf("daily cache full (>= high watermark), skip write\n");
        return 0;
    }
    if(ret < 0)
    {
        dc_sql_cache_delete();
        if(dc_sql_cache_create() == 0)
            ret = dc_sql_cache_set_item(&cache_item, DataMgmtFops.map_node_num());
    }

    if(debug)
    {
        time_t    t_now;
        struct tm ltime;
        float ratio;

        /* get the time */
        t_now = time(&t_now);
        localtime_r(&t_now, &ltime);
        ratio = (float)cache_item.blob_size * 100.0f / origin_size;

        printf("%04d-%02d-%02d %02d:%02d:%02d => id:%lld  Node NUM:%5d  compressed:%d  orgin:%d  ratio: %.02f %%  db:%lu mode:%s\n", 
                ltime.tm_year+1900, ltime.tm_mon+1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec,
                (long long)cache_item.id,
                DataMgmtFops.map_node_num(), cache_item.blob_size, origin_size, ratio,
                dc_sql_file_size(),
                (dc_sql_cache_mode() == DC_CACHE_MODE_DAILY) ? "daily" : "roll");
    }

    return ret;
}


/*************************************************************************/
/** Function Name:     dc_cache_check_daily_rollover                   **/
/** Purpose:           日模式检测日期变化并触发归档                     **/
/** Params:            ctx - 上下文；ltime - 当前时间；p_ymd - 日戳缓存 **/
/** Return:            None.                                           **/
/** Notice:            roll 模式直接返回                               **/
/*************************************************************************/
static void dc_cache_check_daily_rollover(struct datacache_ctx_t *ctx, struct tm *ltime, int *p_ymd)
{
    int ymd;
    struct tm yesterday;

    if (ctx->attr.mode != DC_CACHE_MODE_DAILY)
        return;

    ymd = (ltime->tm_year + 1900) * 10000 + (ltime->tm_mon + 1) * 100 + ltime->tm_mday;
    if (*p_ymd < 0)
    {
        *p_ymd = ymd;
        return;
    }

    if (ymd == *p_ymd)
        return;

    /* 日期变化：归档前一天，并清空当日 RAM 库（跨天续传不保留） */
    yesterday = *ltime;
    yesterday.tm_mday -= 1;
    mktime(&yesterday); /* 规范到真实昨天 */

    if (ctx->debug)
        printf("daily rollover: archive %04d%02d%02d -> %s\n",
               yesterday.tm_year + 1900, yesterday.tm_mon + 1, yesterday.tm_mday,
               ctx->attr.history_path);

    if (dc_sql_cache_daily_rollover(&ctx->attr, &yesterday) != 0)
        printf("daily rollover failed\n");

    *p_ymd = ymd;
}


/*************************************************************************/
/** Function Name:                                     **/
/** Purpose:                                         **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
int main(int argc, char **argv)
{
    void     *vmem;
    int       opt, inv_time = -1;
    time_t    t_now;
    int       t_lastmin = -1;
    int       t_lastsec = -1;
    int       day_ymd = -1;
    struct tm ltime;
    
    struct datacache_ctx_t ctx = {
        .debug = false,
        .attr = {
            .checked = 0,
            .inv_time = 300,
            .max_alarm_nums = 256,
            .max_items = 8192,
            .max_rows = 2016,
            /* 启动默认=旧设计；cfg_load 会再铺一遍 LEGACY，有新字段再覆盖 */
            .mode = DC_CFG_LEGACY_MODE,
            .high_size = DC_CFG_LEGACY_HIGH_SIZE,
            .low_size = DC_CFG_LEGACY_LOW_SIZE,
            .history_days = DC_CFG_LEGACY_HISTORY_DAYS,
        }
    };

    strncpy(ctx.attr.history_path, DC_CFG_LEGACY_HISTORY_PATH, sizeof(ctx.attr.history_path) - 1);

    if(argc >= 2)
    {
        while((opt=getopt(argc, argv, "i:dh")) != -1)
        {
            switch(opt)
            {
                case 'd':
                    ctx.debug = true;
                break;
                case 'i':
                    inv_time = atoi(optarg);
                break;
                case 'h':
                    usage(argv[0]);
                    exit(0);
                break;
                
            }
        }
    }

    /*
        database attach:
            it just get the vmem pointer from /dev/vmem  (mmap)
    */
    vmem = rt_database_attach();
    if(vmem == NULL)
    {
        printf("please insert /dev/vmem.ko first\n");
        return -1;
    }

    ctx.pDB = (struct db_spl_realtime_data_tbl_t *)vmem;

    /* database fops init */
    DataMgmtFops.init(vmem);

    /* data cache configure */
    dc_cfg_load(&ctx, DC_CFG_FILE);

    /* 命令行有强制执行间隔时间，不使用配置文件间隔参数 */
    if(inv_time > 0)
        ctx.attr.inv_time = inv_time;

    /* 不允许间隔为0 */
    ctx.attr.inv_time = (ctx.attr.inv_time == 0) ? 60 : ctx.attr.inv_time;

    dc_sql_cache_apply_policy(&ctx.attr);

    /* database init */
    if(dc_sql_cache_init(ctx.pDB) != 0)
    {
        printf("data cache init fail\n");
        rt_database_deattach();
        return -1;
    }

    while(1)
    {
        if(ctx.attr.checked)
        {
            /* get the time */
            t_now = time(&t_now);
            localtime_r(&t_now, &ltime);

            dc_cache_check_daily_rollover(&ctx, &ltime, &day_ymd);

            /* 分钟级间隔 */
            if(ctx.attr.inv_time >= 60)
            {
                if((ltime.tm_min % (ctx.attr.inv_time / 60) == 0) && (t_lastmin != ltime.tm_min))
                {
                    dc_cache_record_process(ctx.pDB, ctx.debug);
                    t_lastmin = ltime.tm_min;
                }
            }
            else 
            {
                /* 秒级间隔 */
                if((ltime.tm_sec % ctx.attr.inv_time == 0) && (t_lastsec != ltime.tm_sec))
                {
                    dc_cache_record_process(ctx.pDB, ctx.debug);
                    t_lastsec = ltime.tm_sec;
                }
            }
            
        }

        sleep(1);
    }

    dc_sql_cache_destroy();
    rt_database_deattach();
    return 0;
}
