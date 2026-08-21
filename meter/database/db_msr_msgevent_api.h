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

#ifndef __DB_MSR_MSGEVENT_API_H__
#define __DB_MSR_MSGEVENT_API_H__


#include <stdint.h>


/* msg key */
#define DB_MSR_MSG_KEY	                    'u'
#define DB_MSR_KEY_WAIT                     100


/* 
    event message max size 
    WARNNING:
       this size must not less than sizeof (struct xxx of event), 
       for example: key struct => struct input_event
                    timer struct =>    
*/
#define DB_MSR_EVENT_MSG_SIZE               48


/*
    write 
*/

enum
{
	DB_MSR_EVENT_TYPE_KEY,                      // publish: keyboard event
    
    // Unified generic event types (all channels)
    DB_MSR_EVENT_TYPE_TIMER,                    // private: timer event
    DB_MSR_EVENT_TYPE_ADJUST,                   //校表
    DB_MSR_EVENT_TYPE_ADJUST_OFFSET,            //校准offset
    DB_MSR_EVENT_TYPE_DEMO,                     //demo
    
    DB_MSR_EVENT_TYPE_MAXDMD_DEFAULT,           //清除需量最值
    DB_MSR_EVENT_TYPE_DEMO_END,                 //结束demo模式
    DB_MSR_EVENT_TYPE_EXTREME_DEFAULT,          //清除最值

    
    DB_MSR_EVENT_TYPE_ADJUST_RST,               //校表系数复位
    /*
        新更改。 把原来的各种事件类型合并为一种事件类型。
        原来的各种事件类型：
        DB_MSR_EVENT_TYPE_PHASE_ADJUST_HAND,        //手动相序调整，电流通道切换
        DB_MSR_EVENT_TYPE_WIRINGMODE,               //接线模式
        DB_MSR_EVENT_TYPE_PLUSE,                    //脉冲输出
        DB_MSR_EVENT_TYPE_SYS,                      //默认系统系数，即初始化
        DB_MSR_EVENT_TYPE_USER,                     //默认用户系数，即初始化
        DB_MSR_EVENT_TYPE_RST,                      //软件复位
        DB_MSR_EVENT_TYPE_ADJUST_RST,               //校表系数复位
        DB_MSR_EVENT_TYPE_PHASE_ADJUST_AUTO,        //自动相序调整，电流通道切换
        DB_MSR_EVENT_TYPE_PHASE_ADJUST_AUTO_END,    //自动相序调整，电流通道切换
        DB_MSR_EVENT_TYPE_PHASE_ADJUST_AUTO_RESULT, //自动相序调整，电流通道切换
    */
    DB_MSR_EVENT_TYPE_REINIT,           //重新初始化
    
    // Legacy aliases for backward compatibility (deprecated, will be removed)


};



/*
    struct db_msg_evt_t => this is linux message queue
        |
        |-   msg_type
        |-   mq_poll   => event memory  (struct db_msg_evt_general_t *)
                    |-  type      ---- event type, for example: timer, key and so on..
                    |-  ctx       ---- accroding to the event type, use different struct !

*/
struct db_msr_msg_evt_t
{
    /*
        event message

        mq_poll is a common memory.  (struct xxx *) to convent to some event type
    */
    long                msg_type;
    unsigned char       mq_pool[DB_MSR_EVENT_MSG_SIZE];
};




/*

*/
struct db_msr_msg_evt_general_t
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
    unsigned char       ctx[DB_MSR_EVENT_MSG_SIZE];
};

/* private function. 外部进程不能调用 */
int db_msr_msg_event_api_pend(uint8_t chnlno, struct db_msr_msg_evt_general_t *p);




/* 
    publish function.
*/
/* 全局配置函数：必须在init之前调用，用于设置最大通道数量。如果不调用，默认为2 */
int db_msr_msg_event_api_config(uint8_t max_channels);

/* 初始化指定通道的消息队列 */
int db_msr_msg_event_api_init(uint8_t chnlno);

/* 销毁所有消息队列，释放动态分配的资源 */
void db_msr_msg_event_api_destory(void);

/* 发送消息到指定通道 */
int db_msr_msg_event_api_post(uint8_t chnlno, void *p, int len); // 如需带参数，且本api未提供的接口，可用户调用该接口实现，推荐在本接口实现

/* 发送命令消息到指定通道 */
int db_msr_msg_event_cmd(uint8_t chnlno, int cmd); // 不带参数的命令消息，如发命令清电度，更新系统配置，更新用户配置等

/* 获取当前配置的最大通道数 */
uint8_t db_msr_msg_event_api_get_max_channels(void);


#endif
