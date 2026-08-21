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
    struct dc_sql_cache_item_t cache_item;

    cache_item.blob_size = dc_lz4_compress((uint8_t *)pDB, DataMgmtFops.map_active_node_size(), cache_item.blob, sizeof(cache_item.blob));
    ret = dc_sql_cache_set_item(&cache_item, DataMgmtFops.map_node_num());
    if(ret < 0)
    {
        dc_sql_cache_delete();
        dc_sql_cache_create();
        dc_sql_cache_set_item(&cache_item, DataMgmtFops.map_node_num());
    }

    if(debug)
    {
        time_t    t_now;
        struct tm ltime;

        /* get the time */
        t_now = time(&t_now);
        localtime_r(&t_now, &ltime);

        printf("%04d-%02d-%02d %02d:%02d:%02d => id:%lld  Node NUM:%5d  compressed:%d  orgin:%d  ratio: %.02f %%\n", 
                ltime.tm_year+1900, ltime.tm_mon+1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec,
                cache_item.id,
                DataMgmtFops.map_node_num(), cache_item.blob_size, DataMgmtFops.map_active_node_size(), (float)cache_item.blob_size * 100 / DataMgmtFops.map_active_node_size());
    }

    return ret;
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
    uint8_t   t_lastmin;
    struct tm ltime;
    
    struct datacache_ctx_t ctx = {
        .debug = false,
        .attr = {
            .checked = 0,
            .inv_time = 300,
            .max_alarm_nums = 256,
            .max_items = 8192, // NO USE
            .max_rows = 2016,  // NO USE
        }
    };

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

    /* database init */
    dc_sql_cache_init(vmem);

    while(1)
    {
        if(ctx.attr.checked)
        {
            /* get the time */
            t_now = time(&t_now);
            localtime_r(&t_now, &ltime);

            /* 分钟级间隔 */
            if(ctx.attr.inv_time >= 60)
            {
                if((ltime.tm_min % (ctx.attr.inv_time / 60) == 0) && (t_lastmin != ltime.tm_min))
                {
                    /* 容错。数据库文件大于限制大小，删除 最旧10条 */
                    if(dc_sql_file_size() > DC_SQL_CACHE_FILE_MAXSIZE)
                    {
                        dc_sql_cache_rows_flush(10);
                    }

                    /* 定时缓存 */
                    dc_cache_record_process(vmem, ctx.debug);
                    
                }
            }
            else 
            {
                /* 秒级间隔 */
                if(ltime.tm_sec % ctx.attr.inv_time == 0)
                {
                    /* 容错。数据库文件大于限制大小，删除 最旧10条 */
                    if(dc_sql_file_size() > DC_SQL_CACHE_FILE_MAXSIZE)
                    {
                        dc_sql_cache_rows_flush(10);
                    }

                    /* 定时缓存 */
                    dc_cache_record_process(vmem, ctx.debug);
                }
            }
            
        }

        t_lastmin = ltime.tm_min;
        sleep(1);
    }

    dc_sql_cache_destroy();
    rt_database_deattach();
    return 0;
}
