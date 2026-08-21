/*
* Copyright (c) 2013,珠海一构物联有限公司
* All rights reserved.
*
* 文件名称：db_cfg.h
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
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <dirent.h>
#include <time.h>
#include <sys/types.h> 
#include <unistd.h> 
#include "mqtt_fops.h"

#include "if_data_json.h"
#include "if_process.h"

#include "com_baseclass.h"

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static void mqtt_log_callback(struct mosquitto *mosq, void *userdata, int level, const char *str)
{

}

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static void mqtt_subscribe_callback(struct mosquitto *mosq, void *userdata, int mid, int qos_count, const int *granted_qos)
{
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static void mqtt_connect_callback(struct mosquitto *mosq, void *userdata, int result)
{
    int  ret, i = 0;
    char buffer[48];
    struct mqtt_msg_t mqtt_send_msg;
    struct remote_m_ctx_t *pctx = (struct remote_m_ctx_t *)userdata;

    if(!result){
        /* Subscribe to broker information topics on successful connect. */
        /* 由本机序列号生成本地订阅 */
        snprintf(buffer, 32, "%s%s", MQTT_TOPIC_PREFIX, pctx->sn);
        mosquitto_subscribe(mosq, NULL, buffer, MQTT_QOS);
        /* 生成注册帧 */
        ret = if_msg_pack_register(pctx, &mqtt_send_msg);
        if(ret < 0)
            return;
        
        do 
        {
            ret = mosquitto_publish(mosq, NULL, MQTT_TOPIC_WILL, mqtt_send_msg.len + sizeof(mqtt_send_msg.len), (uint8_t *)&mqtt_send_msg, MQTT_QOS, false);
            if(ret > 0)
                break;
            else 
                sleep(1);

        }while(++i > 3);

        if(pctx->debug)
        {
            printf("MQTT register: publish length: %d\n", mqtt_send_msg.len);
            printf("Context:\n%s\n", mqtt_send_msg.data);
        }
        
    }
}

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static void mqtt_message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message)
{
    int ret;
    struct mqtt_msg_t *pmsg;
    struct remote_m_ctx_t *pctx = (struct remote_m_ctx_t *)userdata;

    if(message->payloadlen <= 0)
        return;

    pmsg = (struct mqtt_msg_t *)(message->payload);
    if(pctx->debug)
        printf("MQTT function code:%d\n", pmsg->func);

    switch (pmsg->func)
    {
        case FUNC_WILL:
            /*  do nothing */
            break;
        case FUNC_STATUS:
            /*  do nothing */
            break;
        case FUNC_VPN_CON:
            if_process_vpn_conn(pctx, pmsg);
            break;
        case FUNC_VPN_DISC:
            if_process_vpn_disconn(pctx, pmsg);
            break;
        case FUNC_TRANS:
            ret = pctx->hal_com.pcombase->write(&pctx->hal_com, (uint8_t *)pmsg->data, pmsg->len-2);
            if(ret < 0)
            {
                pctx->hal_com.pcombase->connect(&pctx->hal_com);
            }
            break;
        case FUNC_REBOOT:
            system("sync; reboot;");
            break;
        default:
            break;
    }
}

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int mqtt_init(struct remote_m_ctx_t *pctx)
{
    int  ret;
    
    bool session = true;
    struct mqtt_msg_t msg;

    static uint8_t conntime_index = 0;
    const uint8_t conntimeout[] = {30, 60, 120, 240};

    mosquitto_lib_init();

    pctx->mosq = mosquitto_new(pctx->sn, session, pctx);
    if(!pctx->mosq){
        mosquitto_lib_cleanup();
        return -1;
    }

    mosquitto_username_pw_set(pctx->mosq, pctx->attr.user, pctx->attr.passwd);
    mosquitto_log_callback_set(pctx->mosq, mqtt_log_callback);
    mosquitto_connect_callback_set(pctx->mosq, mqtt_connect_callback);
    mosquitto_message_callback_set(pctx->mosq, mqtt_message_callback);
    mosquitto_subscribe_callback_set(pctx->mosq, mqtt_subscribe_callback);
    mosquitto_reconnect_delay_set(pctx->mosq, 30, 480, false);

    /* 遗愿设定 */
    ret = if_msg_pack_will(pctx, &msg);
    if(ret < 0)
        return -1;

    mosquitto_will_set(pctx->mosq, MQTT_TOPIC_WILL, msg.len+sizeof(msg.len), (uint8_t *)&msg, 1, false);

    while( mosquitto_connect(pctx->mosq, pctx->attr.ip, pctx->attr.port, 300) ) 
    {

        conntime_index = (conntime_index >= sizeof(conntimeout)) ? 0 : conntime_index;
        sleep(conntimeout[conntime_index]);
        conntime_index++;
    }


    // heartbeat thread process
    if_process_heartbeat_start(pctx);

    return 0;
}



/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
void mqtt_process(struct remote_m_ctx_t *pctx)
{
    mosquitto_loop_forever(pctx->mosq, -1, 1);
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
void mqtt_destroy(struct remote_m_ctx_t *pctx)
{
    mosquitto_destroy(pctx->mosq);
    mosquitto_lib_cleanup();
}