/*
*  Copyright (c) 2019,珠海科技股份有限公司
*  All rights reserved.
* 
*  文件名称：Interface.c
*  文件标识：
*  描    述：SimpleGUI接口
* 
*  当前版本：V1.0
*  作    者：chenzy
*  运行环境：
*  操作系统: linux
*  硬件平台：f1c100s
*  固件库  ：
*  修改记录 
*  作者       时间         版本      修改描述
*  chenzy   2020       V1.00      
*/

#ifndef __DB_MSGEVENT_API_H__
#define __DB_MSGEVENT_API_H__




/* msg key */
#define DB_MSG_KEY	                    'd'
#define DB_KEY_WAIT                     250


/* 
    event message max size 
    WARNNING:
       this size must not less than sizeof (struct xxx of event), 
       for example: key struct => struct input_event
                    timer struct =>    
*/
#define DB_EVENT_MSG_SIZE               48


/*
    write 
*/

enum
{
	DB_EVENT_TYPE_KEY,                  // publish: keyboard event
	DB_EVENT_TYPE_TIMER,                // private: timer    event
    DB_EVENT_TYPE_SYS_RAM_TO_JSON,      // publish：  settting update from RAM to JSON
    DB_EVENT_TYPE_USER_RAM_TO_JSON,     // publish:   settting update from RAM to JSON
    DB_EVENT_TYPE_CLEAR_REALTIME_ENERGY,
    DB_EVENT_TYPE_CLEAR_HISTORY_ENERGY,
    DB_EVENT_TYPE_SYS_CFG_DEFAULT,  //
    DB_EVENT_TYPE_USER_CFG_DEFAULT,  //
    DB_EVENT_TYPE_CLEAR_MAX_DEMAND,
    DB_EVENT_TYPE_CLEAR_EXTREME,
    DB_EVENT_TYPE_SYS_JSON_TO_RAM,      // publish：  settting update from JSON to RAM
    DB_EVENT_TYPE_USER_JSON_TO_RAM,     // publish:   settting update from JSON to RAM
    DB_EVENT_TYPE_CLEAR_MULTIRATE_ENERGY,
    DB_EVENT_TYPE_USER_MULTIRATE_RAM_TO_JSON,     // publish:   settting update from RAM to JSON
    DB_EVENT_TYPE_USER_MULTIRATE_CFG_DEFAULT,  //
    DB_EVENT_TYPE_USER_MULTIRATE_JSON_TO_RAM,     // publish:   settting update from JSON to RAM

    DB_EVENT_TYPE_CLEAR_LOADRUN,     //清除运行时间
    DB_EVENT_TYPE_REBOOT,   // REBOOT message type
    DB_EVENT_TYPE_SAVE_ALL, // save all type
    DB_EVENT_TYPE_SAVE_DEVICEINFO,
    
    DB_EVENT_TYPE_USER_DIGI_RAM_TO_JSON,     // publish:   settting update from RAM to JSON
    DB_EVENT_TYPE_USER_DIGI_CFG_DEFAULT,  //
    DB_EVENT_TYPE_USER_DIGI_JSON_TO_RAM,     // publish:   settting update from JSON to RAM
    DB_EVENT_TYPE_BOOT,                 //恢复出厂页面
    DB_EVENT_TYPE_CLEAR_RUNING_ENERGY,
    DB_EVENT_TYPE_CLEAR_REALTIME_ENERGY2,
    DB_EVENT_TYPE_CLEAR_HISTORY_ENERGY2,
    DB_EVENT_TYPE_CLEAR_MULTIRATE_ENERGY2,

};



/*
    struct db_msg_evt_t => this is linux message queue
        |
        |-   msg_type
        |-   mq_poll   => event memory  (struct db_msg_evt_general_t *)
                    |-  type      ---- event type, for example: timer, key and so on..
                    |-  ctx       ---- accroding to the event type, use different struct !

*/
struct db_msg_evt_t
{
    /*
        event message

        mq_poll is a common memory.  (struct xxx *) to convent to some event type
    */
    long                msg_type;
    unsigned char       mq_pool[DB_EVENT_MSG_SIZE];
};




/*

*/
struct db_msg_evt_general_t
{
    /*
        event message：
        DB_EVENT_TYPE_KEY,             
        DB_EVENT_TYPE_TIMER,           
        DB_EVENT_TYPE_SYS_RAM_TO_JSON, 
        DB_EVENT_TYPE_USER_RAM_TO_JSON,
    */
    int                 type;

    /*
        rebuild by "struct input_event" , "struct db_msg_timer_event_t", and so on
    */
    unsigned char       ctx[DB_EVENT_MSG_SIZE];
};

/* private function. 外部进程不能调用 */
int db_msg_event_api_pend(struct db_msg_evt_general_t *p);




/* 
    publish function.
*/
int db_msg_event_cmd(int cmd); // 不带参数的命令消息，如发命令清电度，更新系统配置，更新用户配置等
int db_msg_event_api_init(void);
void db_msg_event_api_destory(void);
int db_msg_event_api_post(void *p, int len); // 如需带参数，且本api未提供的接口，可用户调用该接口实现，推荐在本接口实现
int db_msg_event_cmd_send(int cmd);
#endif
