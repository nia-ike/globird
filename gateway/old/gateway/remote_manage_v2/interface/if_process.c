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
#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include <net/if.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "if_process.h"
#include "if_data_json.h"


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int if_process_vpn_get_ip(char *iface, char *ip, uint8_t ip_len)
{
    char *temp = NULL;
    int inet_sock;
    struct ifreq ifr;
 
    inet_sock = socket(AF_INET, SOCK_DGRAM, 0); 
	memset(ip, 0, ip_len);
    memset(ifr.ifr_name, 0, sizeof(ifr.ifr_name));
    memcpy(ifr.ifr_name, iface, strlen(iface));
 
    if(0 != ioctl(inet_sock, SIOCGIFADDR, &ifr)) 
    {   
        perror("ioctl error");
        return -1;
    }
 
    temp = inet_ntoa(((struct sockaddr_in*)&(ifr.ifr_addr))->sin_addr);     
    memcpy(ip, temp, strlen(temp));
 
    close(inet_sock);
 
    return 0;

}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static void *if_process_vpn_pthread(void *param)
{
    int ret, conn_result = -1;
    time_t t_start;
    char cmd[256];
    char ip[24];
    
    struct if_process_thread_param_t *pprocess  = (struct if_process_thread_param_t *)param;
    struct remote_m_ctx_t *pctx = pprocess->pctx;
    struct if_vpn_info_t   *pif_vpn_info        = (struct if_vpn_info_t *)pprocess->parameter;
    struct mqtt_msg_t      mqtt_send_msg;

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    /* 等待上一次可能未退出的进程退出 */
    pctx->vpn_abort = false;

    /* 关闭旧的 VPN */
    printf("killall edge >/dev/null 2>&1 \n");

    /* 开启VPN */
    snprintf(cmd, sizeof(cmd)-1, "edge -a %s -c %s -k %s -l %s >/dev/null 2>&1 &", pif_vpn_info->ip, pif_vpn_info->group, pif_vpn_info->passwd, pif_vpn_info->server);
    system(cmd);

    /*开始连接*/
    t_start = time(NULL);
    while(!pctx->vpn_abort)
    {
        /* 等待VPN连接成功 */
        ret = if_process_vpn_get_ip("edge0", ip, sizeof(ip));
        if(ret == 0)
        {
            /* vpn 连接成功 赋值连接时间 （非0）*/
            pctx->vpn_starttime = time(NULL);
            conn_result = 0;
            break;
        }
        else 
        {
            /* 连接超时60s，放弃 */
            if(time(NULL) - t_start > 60)
            {
                system("killall edge");
                pctx->vpn_starttime = 0;
                pctx->vpn_abort = false;
                conn_result = -1;
            }
            sleep(1);
        }    
    }

    // 连接结束，可能成功，也可能失败，回复连接信息
    ret = if_msg_pack_connvpn(pctx, &mqtt_send_msg, conn_result, ip);
    if(ret < 0)
        return NULL;
    
    mosquitto_publish(pctx->mosq, NULL, MQTT_TOPIC_WILL, mqtt_send_msg.len+sizeof(mqtt_send_msg.len), (uint8_t *)&mqtt_send_msg, MQTT_QOS, false);
    if(pctx->debug)
    {
        printf("MQTT VPN: publish length: %d\n", mqtt_send_msg.len);
        printf("Context:\n%s\n", mqtt_send_msg.data);
    }

    return NULL;
}

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int if_process_vpn_conn(struct remote_m_ctx_t *pctx, struct mqtt_msg_t *pmqtt_recieve_msg)
{
    int ret;
    pthread_t pid;
    pthread_attr_t attr;
    static struct if_vpn_info_t    if_vpn_info;
    static struct if_process_thread_param_t thread_parameter;

    /* 解析下发的vpn 命令 */
    ret = if_msg_unpack_vpninfo(&if_vpn_info, pmqtt_recieve_msg->data, pmqtt_recieve_msg->len - sizeof(pmqtt_recieve_msg->func)); 
    if(ret < 0)
        return -1;

    /* 装载线程参数 */
    thread_parameter.pctx = pctx;
    thread_parameter.parameter = &if_vpn_info;

    /* 退出上一次可能还在运行的VPN启用进程 */
    pctx->vpn_abort = true;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
    pthread_create(&pid, &attr, if_process_vpn_pthread, (void*)&thread_parameter);
    pthread_attr_destroy(&attr);
    
    return 0;
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int if_process_vpn_disconn(struct remote_m_ctx_t *pctx, struct mqtt_msg_t *pmqtt_recieve_msg)
{
    system("killall edge >/dev/null 2>&1");

    pctx->vpn_abort = true;

    return 0;
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int if_process_trans_handle(struct remote_m_ctx_t *pctx, struct mqtt_msg_t *pmsg)
{
    int ret;
    char topic[48];
    uint8_t ttybuf[1286];

    ret = pctx->hal_com.pcombase->read(&pctx->hal_com, (uint8_t *)ttybuf, sizeof(ttybuf), 1000);
    if(ret <= 0)
        return -1;

    ret = if_msg_pack_trans(pctx, pmsg, ttybuf, ret);
    if(ret == 0)
    {
        snprintf(topic, sizeof(topic), "%s%s", pctx->sn, MQTT_TOPIC_TRANS_SUFFIX);
        mosquitto_publish(pctx->mosq, NULL, topic, pmsg->len + sizeof(pmsg->len), (uint8_t *)pmsg, MQTT_QOS, false);
        if(pctx->debug)
        {
            printf("MQTT trans: publish length: %d\n", pmsg->len);
        }

        return 0;
    }

    return -1; 
}



/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static void *if_process_heartbeat_pthread(void *param)
{
    int ret;
    time_t    t_now;
    uint8_t   t_lastmin;
    struct tm ltime;
    struct mqtt_msg_t mqtt_send_msg;
    
    struct if_process_thread_param_t *pprocess  = (struct if_process_thread_param_t *)param;
    struct remote_m_ctx_t *pctx = pprocess->pctx;
    
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    /* 首次连接，尝试发送心跳 */
    ret = if_msg_pack_heartbeat(pprocess->pctx, &mqtt_send_msg);
    if(ret == 0)
    {
        mosquitto_publish(pctx->mosq, NULL, MQTT_TOPIC_WILL, mqtt_send_msg.len + sizeof(mqtt_send_msg.len), (uint8_t *)&mqtt_send_msg, MQTT_QOS, false);
        if(pctx->debug)
        {
            printf("MQTT Heartbeat: publish length: %d\n", mqtt_send_msg.len);
            printf("Context:\n%s\n", mqtt_send_msg.data);
        }
    }

    while(1)
    {
        /* get the time */
        t_now = time(&t_now);
        localtime_r(&t_now, &ltime);

        /* 阻塞超时1s 等待透传modbus有数据， 如果无数据，降超时1000ms返回*/
        if_process_trans_handle(pctx, &mqtt_send_msg);

        /* 15 分钟推送一次 */
        if((ltime.tm_min % (900/60) == 0) && (t_lastmin != ltime.tm_min))
        {
            ret = if_msg_pack_heartbeat(pprocess->pctx, &mqtt_send_msg);
            if(ret == 0)
            {
                mosquitto_publish(pctx->mosq, NULL, MQTT_TOPIC_WILL, mqtt_send_msg.len + sizeof(mqtt_send_msg.len), (uint8_t *)&mqtt_send_msg, MQTT_QOS, false);
                if(pctx->debug)
                {
                    printf("MQTT Heartbeat: publish length: %d\n", mqtt_send_msg.len);
                    printf("Context:\n%s\n", mqtt_send_msg.data);
                }
            }
        }

        /* vpn 超时 关断 */
        if((pctx->vpn_starttime != 0) &&        // VPNstarttime != 0 开启VPN
            ((t_now - pctx->vpn_starttime) > VPN_MAX_TIME))
        {
            system("killall edge >/dev/null 2>&1");
            pctx->vpn_starttime = 0;
        }

        t_lastmin = ltime.tm_min;
    }


    return NULL;
}

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int if_process_heartbeat_start(struct remote_m_ctx_t *pctx)
{
    pthread_t pid;
    pthread_attr_t attr;

    static struct if_process_thread_param_t thread_parameter;

    /* 装载线程参数 */
    thread_parameter.pctx = pctx;
    thread_parameter.parameter = NULL;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
    pthread_create(&pid, &attr, if_process_heartbeat_pthread, (void*)&thread_parameter);
    pthread_attr_destroy(&attr);
    
    return 0;
}
