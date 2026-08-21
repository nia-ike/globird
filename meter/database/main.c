#include <stdio.h>
#include <string.h> 
#include <sys/types.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <sys/stat.h>  //stat
#include <time.h>

#include "db_device.h"
#include "db_struct.h"
#include "db_msg_process.h"
#include "db_msgevent_api.h"
#include "db_msr_msgevent_api.h"
#include "db_cfg_info.h"
#include "db_channel_helper.h"  /* 多通道辅助宏 */

#include "db_measure_energy.h"

#define MSG_ENERGY_SAVE_TIME0               0
#define MSG_HISTORY_ENERGY_SAVE_TIME1       1
#define MSG_ENERGY_SAVE_TIMES               (1)   // 3600s


/*
    running in backend
*/
static int          backend = 0;
static uint32_t     inv_energy_record = MSG_ENERGY_SAVE_TIMES;
static uint8_t      max_msg_channels = 2;  // 默认支持2个消息通道
static time_t       last_energy_save_hour = 0;  // 记录上次保存电能数据的小时数

#define FILE_NAME_FIRSTBOOT "/userdata/config/system/firstboot"


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
	fprintf(stderr,"  -b                  : run in back end\n");
    fprintf(stderr,"  -t N                : record the energy every N secords\n");
    fprintf(stderr,"  -n TSM0206240000    : set device serial number\n");
    fprintf(stderr,"  -c N                : set max message channels (default: 2)\n");
	fprintf(stderr,"  -h                  : Show this help message\n");
}



/*************************************************************************/
/** Function Name:                                     **/
/** Purpose:                                         **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
static void poweroff_clear_counter(struct db_database_type *pDB)
{
    pDB->db_cfg_info.product_test.poweroff = 0;
    pDB->db_cfg_info.status.poweroff_status = 0;
}

/*************************************************************************/
/** Function Name:                                     **/
/** Purpose:           发生短时掉电次数，用于生产测试用途                              **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
static void poweroff_inc_counter(struct db_database_type *pDB)
{
    pDB->db_cfg_info.product_test.poweroff++;
}


/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
static void db_clean_boot(struct db_database_type *pDB)
{
    unlink(FILE_NAME_FIRSTBOOT);
}



/*************************************************************************/
/** Function Name:                                     **/
/** Purpose:                                         **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
static int msg_process(void *vmem)
{
    int ret, i;
    struct db_msg_evt_general_t msg;
    struct db_database_type *pDB = (struct db_database_type *)vmem;
    uint8_t num_channels = db_msr_msg_event_api_get_max_channels();

    /* block for event */
    ret = db_msg_event_api_pend(&msg);
    if(ret > 0)
    {
        /*
            process for event type
        */
        switch (msg.type)
        {
            /* timer event */
        case DB_EVENT_TYPE_TIMER:
        {
            
            struct db_msg_timer_event_t *pTimer = (struct db_msg_timer_event_t *)&msg.ctx[0];

            /* which timer? */
            switch (pTimer->idx)
            {
            case MSG_ENERGY_SAVE_TIME0:
            {
                /* 每小时第3秒执行一次 */
                time_t now = time(NULL);
                struct tm *tm_now = localtime(&now);
                time_t current_hour = now / 3600;  // 当前小时数（从1970-01-01开始）
                
                // 检查是否到达新的小时，且当前秒数在3秒处（允许1秒误差：2-4秒）
                if (current_hour != last_energy_save_hour && tm_now->tm_sec >= 2 && tm_now->tm_sec <= 4)
                {
                    last_energy_save_hour = current_hour;
                    
                    printf("Hourly energy save at %04d-%02d-%02d %02d:%02d:%02d\n",
                           tm_now->tm_year + 1900, tm_now->tm_mon + 1, tm_now->tm_mday,
                           tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec);
                    
                    /* record energy now - 循环处理所有通道 */
                    FOR_EACH_CHANNEL(num_channels, ch) {
                        db_energy_update_main(&pDB->db_measurebase[ch], ch);
                        db_energy_update_backup(&pDB->db_measurebase[ch], ch);
                        
                        db_multirate_energy_update_main(&pDB->db_measurebase[ch], ch);
                        db_multirate_energy_update_backup(&pDB->db_measurebase[ch], ch);
                    }
                    
                    /* 循环处理所有通道 */
                    FOR_EACH_CHANNEL(num_channels, ch) {
                        db_maxdmd_update(&pDB->db_measurebase[ch], ch);
                        db_runing_energy_update(&pDB->db_measurebase[ch], ch);
                        db_loadrun_update(&pDB->db_measurebase[ch], ch);
                    }
                    sync();
                }
                break;
            }
                
            case MSG_HISTORY_ENERGY_SAVE_TIME1:
                /*
                    history energy process.
                    use for record the "day" "month" "year" history energy
                */
                FOR_EACH_CHANNEL(num_channels, ch)
                {
                    db_history_energy_process(&pDB->db_measurebase[ch], ch,
                                            db_energy_get_history_main(ch));
                }
                sync();
                break;
            default:
                break;
            }
            break;
        }
        /* power off message */
        case DB_EVENT_TYPE_KEY:
        {
            struct input_event *pKey = (struct input_event *)&msg.ctx[0];
            switch(pKey->code)
            {
                case DB_KEY_EVENT_CODE_PWROFF:
                if(pKey->value == DB_KEY_EVENT_PRESSED)
                {
                    #define DB_STATUS_POWEROFF (0xF00F)
                    pDB->db_cfg_info.status.poweroff_status = DB_STATUS_POWEROFF;
                    pDB->db_cfg_info.status.poweroff_Time = pKey->time;
                    /* poweroff counter */
                    poweroff_inc_counter(pDB);
                    
                    /* record energy now - 循环处理所有通道 */
                    FOR_EACH_CHANNEL(num_channels, ch)
                    {
                        db_energy_update_main(&pDB->db_measurebase[ch], ch);
                        db_energy_update_backup(&pDB->db_measurebase[ch], ch);
                        
                        db_multirate_energy_update_main(&pDB->db_measurebase[ch], ch);
                        db_multirate_energy_update_backup(&pDB->db_measurebase[ch], ch);
                    }
                    
                    /* 循环处理所有通道 */
                    FOR_EACH_CHANNEL(num_channels, ch)
                    {
                        db_maxdmd_update(&pDB->db_measurebase[ch], ch);
                        db_runing_energy_update(&pDB->db_measurebase[ch], ch);
                        db_loadrun_update(&pDB->db_measurebase[ch], ch);
                    }

                    sync();
                }
                break;
            }
            break;
        }
        case DB_EVENT_TYPE_SYS_RAM_TO_JSON:
        {
            FOR_EACH_CHANNEL(num_channels, ch) {
                db_cfg_sys_update(&pDB->db_measurebase[ch], ch);
                db_msr_msg_event_cmd(ch, DB_MSR_EVENT_TYPE_REINIT);
            }
            sync();
            break;
        }
        case DB_EVENT_TYPE_USER_RAM_TO_JSON:
        {
            FOR_EACH_CHANNEL(num_channels, ch) {
                db_cfg_user_update(&pDB->db_measurebase[ch], ch);
                db_msr_msg_event_cmd(ch, DB_MSR_EVENT_TYPE_REINIT);
            }
            
            /* language is same across all channels (copy from channel 0) */
            FOR_EACH_CHANNEL_FROM_1(num_channels, ch) {
                pDB->db_measurebase[ch].db_cfg.user.base.Language.value = 
                    pDB->db_measurebase[0].db_cfg.user.base.Language.value;
            }

            sync();
            break;
        }
        case DB_EVENT_TYPE_USER_DIGI_RAM_TO_JSON:
        {
            db_cfg_user_expand_update(vmem);
            sync();
            break;
        }
        case DB_EVENT_TYPE_SYS_JSON_TO_RAM:
        {
            /*
                USE for some process setting the value, but be interrupted, restore the setting from json
            */
            FOR_EACH_CHANNEL(num_channels, ch)
            {
                db_cfg_sys_load(&pDB->db_measurebase[ch], db_cfg_get_sys_file(ch), ch);
            }
            sync();
            break;
        }
        case DB_EVENT_TYPE_USER_JSON_TO_RAM:
        {
            /*
                USE for some process setting the value, but be interrupted, restore the setting from json
            */
            FOR_EACH_CHANNEL(num_channels, ch)
            {
                db_cfg_user_load(&pDB->db_measurebase[ch], db_cfg_get_user_base_file(ch), ch);
            }

            // language is same across all channels (copy from channel 0)
            FOR_EACH_CHANNEL_FROM_1(num_channels, ch)
            {
                pDB->db_measurebase[ch].db_cfg.user.base.Language.value = 
                    pDB->db_measurebase[0].db_cfg.user.base.Language.value;
            }

            db_cfg_user_expand_load(vmem, db_cfg_get_expand_file(DB_CFG_TYPE_NORMAL));
            sync();
            break;
        }
        case DB_EVENT_TYPE_USER_DIGI_JSON_TO_RAM:
        {
            /*
                USE for some process setting the value, but be interrupted, restore the setting from json
            */
            db_cfg_user_expand_load(vmem, db_cfg_get_digi_file(DB_CFG_TYPE_NORMAL));
            sync();
            break;
        }
        case DB_EVENT_TYPE_CLEAR_REALTIME_ENERGY:
        {
            /* 清除所有通道的实时能量 */
            FOR_EACH_CHANNEL(num_channels, ch) {
                db_energy_default(&pDB->db_measurebase[ch], ch, 
                                 db_energy_get_realtime_main(ch), 
                                 db_energy_get_realtime_backup(ch));
            }
            sync();
            break;
        }

        case DB_EVENT_TYPE_CLEAR_RUNING_ENERGY:
        {
            /* 清除所有通道的运行能量 */
            FOR_EACH_CHANNEL(num_channels, ch) {
                db_runing_energy_default(&pDB->db_measurebase[ch], ch);
            }
            break;
        }

        case DB_EVENT_TYPE_CLEAR_HISTORY_ENERGY:
        {
            /* 清除所有通道的历史能量 */
            FOR_EACH_CHANNEL(num_channels, ch) {
                db_history_energy_default(&pDB->db_measurebase[ch], ch, 
                                         db_energy_get_history_main(ch));
            }
            sync();
            break;
        }
      
        case DB_EVENT_TYPE_SYS_CFG_DEFAULT:
        {
            /* 循环恢复所有通道的系统配置到默认值 */
            FOR_EACH_CHANNEL(num_channels, ch)
            {
                db_cfg_sys_default(&pDB->db_measurebase[ch], ch);
                db_msr_msg_event_cmd(ch, DB_MSR_EVENT_TYPE_REINIT); 
            }
            sync();
            break;
        }
        case DB_EVENT_TYPE_USER_CFG_DEFAULT:
        {
            /* 循环恢复所有通道的用户配置到默认值 */
            FOR_EACH_CHANNEL(num_channels, ch)
            {
                db_cfg_user_default(&pDB->db_measurebase[ch], ch);
                db_cfg_user_init(&pDB->db_measurebase[ch], ch);
                db_msr_msg_event_cmd(ch, DB_MSR_EVENT_TYPE_REINIT);    
            }

            sync();
            break;   
        }
        case DB_EVENT_TYPE_USER_DIGI_CFG_DEFAULT:
        {
            db_cfg_user_expand_default(vmem);
            sync();
            break;   
        }
        case DB_EVENT_TYPE_CLEAR_MAX_DEMAND:
        {
            FOR_EACH_CHANNEL(num_channels, ch) {
                db_maxdmd_default(&pDB->db_measurebase[ch], ch);
                db_msr_msg_event_cmd(ch, DB_MSR_EVENT_TYPE_MAXDMD_DEFAULT);
            }
            sync();
            break;   
        }
        case DB_EVENT_TYPE_CLEAR_MULTIRATE_ENERGY:
        {
            /* 清除所有通道的多费率能量 */
            FOR_EACH_CHANNEL(num_channels, ch) {
                db_multirate_energy_default(&pDB->db_measurebase[ch], ch,
                                           db_energy_get_multirate_main(ch), 
                                           db_energy_get_multirate_backup(ch));
            }
            sync();
            break;
        }
        case DB_EVENT_TYPE_USER_MULTIRATE_CFG_DEFAULT:
        {
            /* 多费率配置是全局共享的，所有通道使用相同的费率方案 */
            db_cfg_user_multirate_default(pDB);
            sync();
            break;   
        }
        case DB_EVENT_TYPE_USER_MULTIRATE_JSON_TO_RAM:
        {
            /* 多费率配置是全局共享的，从JSON加载到内存 */
            db_cfg_user_multirate_init(pDB);
            sync();
            break;
        }

        case DB_EVENT_TYPE_USER_MULTIRATE_RAM_TO_JSON:
        {
            /* 多费率配置是全局共享的，从内存保存到JSON */
            db_cfg_user_multirate_update(pDB);
            sync();
            break;
        }       
        case DB_EVENT_TYPE_CLEAR_LOADRUN:
        {
            /* 清除所有通道的负载运行数据 */
            FOR_EACH_CHANNEL(num_channels, ch) {
                db_loadrun_default(&pDB->db_measurebase[ch], ch);
            }
            sync();
            break;
        }

        /* 
         * 注意：通道2的清除功能已合并到通用清除命令中
         * DB_EVENT_TYPE_CLEAR_REALTIME_ENERGY  - 清除所有通道
         * DB_EVENT_TYPE_CLEAR_HISTORY_ENERGY   - 清除所有通道
         * DB_EVENT_TYPE_CLEAR_MULTIRATE_ENERGY - 清除所有通道
         * 以下特定通道事件已废弃，保留用于向后兼容
         */
        case DB_EVENT_TYPE_CLEAR_REALTIME_ENERGY2:
        case DB_EVENT_TYPE_CLEAR_HISTORY_ENERGY2:
        case DB_EVENT_TYPE_CLEAR_MULTIRATE_ENERGY2:
        {
            /* 兼容性：重定向到通用清除命令 */
            printf("Warning: DB_EVENT_TYPE_xxx2 is deprecated, use generic clear commands\n");
            break;
        }  

        case DB_EVENT_TYPE_REBOOT:
        {
            /* record energy now - 循环处理所有通道 */
            FOR_EACH_CHANNEL(num_channels, ch)
            {
                db_energy_update_main(&pDB->db_measurebase[ch], ch);
                db_energy_update_backup(&pDB->db_measurebase[ch], ch);
                
                db_multirate_energy_update_main(&pDB->db_measurebase[ch], ch);
                db_multirate_energy_update_backup(&pDB->db_measurebase[ch], ch);
            }
            
            /* 循环处理所有通道 */
            FOR_EACH_CHANNEL(num_channels, ch)
            {
                db_maxdmd_update(&pDB->db_measurebase[ch], ch);
                db_runing_energy_update(&pDB->db_measurebase[ch], ch);
                db_loadrun_update(&pDB->db_measurebase[ch], ch);
            }

            sync();

            system("reboot -d 3 &");
            break;
        }
        case DB_EVENT_TYPE_SAVE_ALL:
        {
            /* record energy now - 循环处理所有通道 */
            FOR_EACH_CHANNEL(num_channels, ch)
            {
                db_energy_update_main(&pDB->db_measurebase[ch], ch);
                db_energy_update_backup(&pDB->db_measurebase[ch], ch);
                
                db_multirate_energy_update_main(&pDB->db_measurebase[ch], ch);
                db_multirate_energy_update_backup(&pDB->db_measurebase[ch], ch);
            }
            
            /* 循环处理所有通道 */
            FOR_EACH_CHANNEL(num_channels, ch)
            {
                db_maxdmd_update(&pDB->db_measurebase[ch], ch);
                db_runing_energy_update(&pDB->db_measurebase[ch], ch);
                db_loadrun_update(&pDB->db_measurebase[ch], ch);
            }

            sync();
            break;
        }
        case DB_EVENT_TYPE_SAVE_DEVICEINFO:
        {
            db_deviceinfo_init(vmem);
            sync();
            break;
        }
        case DB_EVENT_TYPE_BOOT:
        {
            /* clear first boot now */
            db_clean_boot(vmem);
            sync();
            break;
        }
        case DB_EVENT_TYPE_CLEAR_EXTREME:
        {
            DB_BATCH_BROADCAST_EVENT(num_channels, DB_MSR_EVENT_TYPE_EXTREME_DEFAULT);
            sync();
            break;   
        }
        default:
            break;
        }
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
    void    *vmem;
    int     ret;
    int     opt;
    char    sn[48] = "TSM0206240000";
    struct db_database_type *pDB;
    
    if(argc >= 2)
    {
        while((opt=getopt(argc, argv, "bht:n:c:")) != -1)
        {
            switch(opt)
            {
                case 'b':
                    backend = 1;
                break;
                case 't':
                    inv_energy_record = atoi(optarg);
                break;
                case 'n':
                    strncpy(sn, optarg, sizeof(sn));
                break;
                case 'c':
                    max_msg_channels = (uint8_t)atoi(optarg);
                    if(max_msg_channels < 1 || max_msg_channels > 32) {
                        fprintf(stderr, "Error: max_channels must be between 1 and 32\n");
                        exit(1);
                    }
                    printf("Set max message channels to: %d\n", max_msg_channels);
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

    db_deviceinfo_init(vmem);

    pDB = (struct db_database_type *)vmem;

    /* sn */
    strncpy((char*)&pDB->db_cfg_info.device.SNInfo[0], sn, sizeof(sn));
    
    /*
        配置消息通道数（必须在所有模块初始化之前调用）
    */
    printf("Configuring message API with %d channels\n", max_msg_channels);
    ret = db_msr_msg_event_api_config(max_msg_channels);
    if(ret < 0)
    {
        printf("Failed to configure message channels\n");
        rt_database_deattach();
        return -1;
    }
    
    /*
        load the setting from meter_xx.json
        and put them into the vmem
        循环处理所有通道
    */   
    poweroff_clear_counter(pDB);

    printf("Initializing %d measurement channels...\n", max_msg_channels);
    
    FOR_EACH_CHANNEL(max_msg_channels, ch)
    {
        printf("Initializing channel %d...\n", ch);
        
        /* 加载配置 */
        ret = db_cfg_init(&pDB->db_measurebase[ch], ch);
        if(ret < 0)
        {
            printf("Failed to load config for channel %d\n", ch);
            return -1;
        }
        
        /* 加载能量数据库 */
        db_energy_init(&pDB->db_measurebase[ch], ch,
                      db_energy_get_realtime_main(ch), 
                      db_energy_get_realtime_backup(ch));
        
        db_multirate_energy_init(&pDB->db_measurebase[ch], ch,
                                db_energy_get_multirate_main(ch), 
                                db_energy_get_multirate_backup(ch));
        
        db_history_energy_init(&pDB->db_measurebase[ch], ch,
                              db_energy_get_history_main(ch));
        
        printf("Channel %d initialized successfully\n", ch);
    }

    // language is same across all channels (copy from channel 0)
    FOR_EACH_CHANNEL_FROM_1(max_msg_channels, ch)
    {
        pDB->db_measurebase[ch].db_cfg.user.base.Language.value = 
            pDB->db_measurebase[0].db_cfg.user.base.Language.value;
    }

    /* 加载多费率配置（全局共享，所有通道使用相同费率方案） */
    ret = db_cfg_user_multirate_init(pDB);
    if (ret < 0) {
        printf("Can not load the new multirate cfg file\n");
        // return -1;
    }

    /*
        初始化所有通道的统计功能（需量最值、运行能量、负载运行）
    */
    FOR_EACH_CHANNEL(max_msg_channels, ch)
    {
        db_runing_energy_init(&pDB->db_measurebase[ch], ch);
        db_maxdmd_init(&pDB->db_measurebase[ch], ch);
        db_loadrun_init(&pDB->db_measurebase[ch], ch);
    }

    if(backend)
    {
        printf("switch running in backend\n");
        daemon(0, 1);
    }

    /*
        message event. 
        use for "power off event", "save energy at time"
    */
    db_msg_event_init();

    /* 
        timer0 for period of recording the energy, max demand
        timer1 for recording the history energy
     */
    db_msg_settimer(MSG_ENERGY_SAVE_TIME0, inv_energy_record);
    db_msg_settimer(MSG_HISTORY_ENERGY_SAVE_TIME1, 1);
    /*
        
    */

    while(1)
    {
        /*
            block .  msg_process()
            this function handle the message queue. for example,  key, timer, and so on ...
        */
        msg_process(pDB);
    }

    /* 清理资源 - 循环销毁所有通道 */
    FOR_EACH_CHANNEL(max_msg_channels, ch)
    {
        db_energy_destroy(&pDB->db_measurebase[ch], ch);
        db_multirate_energy_destroy(&pDB->db_measurebase[ch], ch);
        db_history_energy_destroy(&pDB->db_measurebase[ch], ch);
    }
    
    /* 清理所有通道的统计功能 */
    FOR_EACH_CHANNEL(max_msg_channels, ch)
    {
        db_maxdmd_destroy(&pDB->db_measurebase[ch], ch);
        db_runing_energy_destroy(&pDB->db_measurebase[ch], ch);
        db_loadrun_destroy(&pDB->db_measurebase[ch], ch);
    }
    
    /* 清理所有通道的上下文 */
    db_energy_cleanup_all();
    db_multirate_energy_cleanup_all();
    db_history_energy_cleanup_all();
    db_maxdmd_cleanup_all();
    db_runing_energy_cleanup_all();
    db_loadrun_cleanup_all();
    
    /* 销毁消息队列 */
    db_msg_event_destory();
    db_msr_msg_event_api_destory();

    rt_database_deattach();    
    return 0;
}