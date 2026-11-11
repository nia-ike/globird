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
#include <sys/stat.h>

#include "if_process.h"
#include "if_data_json.h"
#include "cJSON.h"
#include "db_cfg_json.h"

#define RM_CFG_FILE             "/userdata/config/project/remote_m.json"

/* 保护 8502 TCP：心跳旧异步读与 MQTT 同步透传不能并发抢字节 */
static pthread_mutex_t g_trans_lock = PTHREAD_MUTEX_INITIALIZER;


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

/* 丢弃 socket 中残留数据，避免上次迟到应答污染本次请求 */
static void if_process_trans_drain(struct remote_m_ctx_t *pctx)
{
    uint8_t junk[256];
    int n;
    int rounds = 0;

    while (rounds++ < 8) {
        n = pctx->hal_com.pcombase->read(&pctx->hal_com, junk, sizeof(junk), 0);
        if (n <= 0)
            break;
    }
}

/*
 * 按 ModbusTCP MBAP 组帧读取完整应答。
 * MBAP length 字段 = UnitID + PDU 长度。
 */
static int if_process_trans_read_mbap(struct remote_m_ctx_t *pctx, uint8_t *buf, uint16_t buflen, int timeout_ms)
{
    int n;
    int total = 0;
    int need = 6;

    while (total < need) {
        n = pctx->hal_com.pcombase->read(&pctx->hal_com, buf + total,
                                         (uint16_t)(buflen - total), timeout_ms);
        if (n <= 0)
            return -1;
        total += n;
        if (need == 6 && total >= 6) {
            int mbap_len = (buf[4] << 8) | buf[5];
            if (mbap_len < 2)
                return -1;
            need = 6 + mbap_len;
            if (need > (int)buflen)
                return -1;
        }
    }

    return total;
}

/*************************************************************************/
/** Function Name:       if_process_trans_request                      **/
/** Purpose:             同步透传：写本地 ModbusTCP -> 等应答 -> MQTT回传 **/
/** Notice:              解决改连 8502 后异步读写易丢应答、云端超时问题   **/
/*************************************************************************/
int if_process_trans_request(struct remote_m_ctx_t *pctx, struct mqtt_msg_t *preq)
{
    int ret;
    int wlen;
    char topic[48];
    uint8_t rxbuf[1286];
    struct mqtt_msg_t mqtt_send_msg;
    const int rx_timeout_ms = 2000;

    if (!pctx || !preq || preq->len < 2)
        return -1;

    wlen = (int)preq->len - 2;
    if (wlen <= 0 || wlen > (int)sizeof(preq->data))
        return -1;

    pthread_mutex_lock(&g_trans_lock);

    /* 确保 TCP 已连接 */
    if (pctx->hal_com.fd < 0) {
        if (pctx->hal_com.pcombase->connect(&pctx->hal_com) < 0) {
            if (pctx->debug)
                printf("trans: connect 8502 failed\n");
            pthread_mutex_unlock(&g_trans_lock);
            return -1;
        }
    }

    if_process_trans_drain(pctx);

    if (pctx->debug) {
        int i;
        printf("trans: write %d bytes to 8502:", wlen);
        for (i = 0; i < wlen && i < 32; i++)
            printf(" %02X", (uint8_t)preq->data[i]);
        printf("%s\n", wlen > 32 ? " ..." : "");
    }

    ret = pctx->hal_com.pcombase->write(&pctx->hal_com, (uint8_t *)preq->data, (uint16_t)wlen);
    if (ret < 0) {
        /* 写失败：重连后再写一次 */
        pctx->hal_com.pcombase->disconnect(&pctx->hal_com);
        if (pctx->hal_com.pcombase->connect(&pctx->hal_com) < 0) {
            if (pctx->debug)
                printf("trans: reconnect failed\n");
            pthread_mutex_unlock(&g_trans_lock);
            return -1;
        }
        ret = pctx->hal_com.pcombase->write(&pctx->hal_com, (uint8_t *)preq->data, (uint16_t)wlen);
        if (ret < 0) {
            if (pctx->debug)
                printf("trans: write retry failed\n");
            pthread_mutex_unlock(&g_trans_lock);
            return -1;
        }
    }

    ret = if_process_trans_read_mbap(pctx, rxbuf, sizeof(rxbuf), rx_timeout_ms);
    if (ret <= 0) {
        if (pctx->debug)
            printf("trans: read timeout/fail from 8502\n");
        pthread_mutex_unlock(&g_trans_lock);
        return -1;
    }

    if (if_msg_pack_trans(pctx, &mqtt_send_msg, rxbuf, (uint16_t)ret) != 0) {
        pthread_mutex_unlock(&g_trans_lock);
        return -1;
    }

    snprintf(topic, sizeof(topic), "%s%s", pctx->sn, MQTT_TOPIC_TRANS_SUFFIX);
    mosquitto_publish(pctx->mosq, NULL, topic,
                      mqtt_send_msg.len + sizeof(mqtt_send_msg.len),
                      (uint8_t *)&mqtt_send_msg, MQTT_QOS, false);

    if (pctx->debug)
        printf("MQTT trans sync: sn=%s topic=%s req=%d rsp=%d\n",
               pctx->sn, topic, wlen, ret);

    pthread_mutex_unlock(&g_trans_lock);
    return 0;
}

/*************************************************************************/
/** Function Name:       if_process_mb_read_holding                    **/
/** Purpose:             经 8502 FC03 读保持寄存器                      **/
/*************************************************************************/
int if_process_mb_read_holding(struct remote_m_ctx_t *pctx, uint8_t unit,
                               uint16_t reg_addr, uint16_t reg_num,
                               uint8_t *out, uint16_t out_len)
{
    uint8_t req[12];
    uint8_t rsp[1286];
    int ret;
    uint16_t need_bytes;
    uint16_t byte_cnt;
    static uint16_t tid = 1;

    if (!pctx || !out || reg_num == 0)
        return -1;

    need_bytes = (uint16_t)(reg_num * 2);
    if (out_len < need_bytes)
        return -1;

    if (pctx->hal_com.fd < 0) {
        if (pctx->hal_com.pcombase->connect(&pctx->hal_com) < 0)
            return -1;
    }

    pthread_mutex_lock(&g_trans_lock);

    /* MBAP + PDU: unit, FC03, addr, qty */
    req[0] = (tid >> 8) & 0xff;
    req[1] = tid & 0xff;
    tid++;
    req[2] = 0;
    req[3] = 0;
    req[4] = 0;
    req[5] = 6;
    req[6] = unit;
    req[7] = 0x03;
    req[8] = (reg_addr >> 8) & 0xff;
    req[9] = reg_addr & 0xff;
    req[10] = (reg_num >> 8) & 0xff;
    req[11] = reg_num & 0xff;

    if_process_trans_drain(pctx);

    ret = pctx->hal_com.pcombase->write(&pctx->hal_com, req, sizeof(req));
    if (ret < 0) {
        pctx->hal_com.pcombase->disconnect(&pctx->hal_com);
        if (pctx->hal_com.pcombase->connect(&pctx->hal_com) < 0) {
            pthread_mutex_unlock(&g_trans_lock);
            return -1;
        }
        ret = pctx->hal_com.pcombase->write(&pctx->hal_com, req, sizeof(req));
        if (ret < 0) {
            pthread_mutex_unlock(&g_trans_lock);
            return -1;
        }
    }

    ret = if_process_trans_read_mbap(pctx, rsp, sizeof(rsp), 2000);
    if (ret < 9) {
        pthread_mutex_unlock(&g_trans_lock);
        return -1;
    }

    /* rsp: MBAP(6) + unit(1) + fc(1) + bytecnt(1) + data */
    if (rsp[7] != 0x03) {
        pthread_mutex_unlock(&g_trans_lock);
        return -1;
    }

    byte_cnt = rsp[8];
    if (byte_cnt < need_bytes || (9 + byte_cnt) > (uint16_t)ret) {
        pthread_mutex_unlock(&g_trans_lock);
        return -1;
    }

    memcpy(out, &rsp[9], need_bytes);
    pthread_mutex_unlock(&g_trans_lock);
    return (int)need_bytes;
}

static void if_process_cert_root_trim(char *path)
{
    char *p;

    if (!path)
        return;
    p = path + strlen(path);
    while (p > path && (p[-1] == '\0' || p[-1] == ' ' || p[-1] == '\r' || p[-1] == '\n'))
        *--p = '\0';
    /* 去掉末尾 '/'，避免拼接 current 时双斜杠（根目录 "/" 除外） */
    while (p > path + 1 && p[-1] == '/')
        *--p = '\0';
}

/*************************************************************************/
/** Function Name:       if_process_cert_root_refresh                  **/
/** Purpose:             从 remote_m.json 的 cert_path 刷新证书根        **/
/** Notice:              不下载；不写死目录；拉起方锁定则不覆盖          **/
/*************************************************************************/
int if_process_cert_root_refresh(struct remote_m_ctx_t *pctx)
{
    char *p = NULL;
    int size;
    cJSON *pJson = NULL;
    cJSON *pAttr = NULL;
    cJSON *pItem = NULL;

    if (!pctx)
        return -1;

    if (pctx->attr.cert_root_locked) {
        if (pctx->attr.cert_path[0] == '\0')
            return -1;
        /* -c 注入的路径也可能带尾部 '/'，拼文件名前必须去掉 */
        if_process_cert_root_trim(pctx->attr.cert_path);
        return 0;
    }

    /* 未锁定：重读 remote_m.json 的 cert_path（57200 只写 json） */
    p = db_cfg_jsonfile_alloc(RM_CFG_FILE, &size);
    if (p) {
        p = db_cfg_jsonfile_read(RM_CFG_FILE, p, size);
        if (p) {
            pJson = cJSON_Parse(p);
            if (pJson) {
                pAttr = cJSON_GetObjectItem(pJson, "attr");
                pItem = pAttr ? cJSON_GetObjectItem(pAttr, "cert_path") : NULL;
                if (pItem && cJSON_IsString(pItem) && pItem->valuestring &&
                    pItem->valuestring[0] != '\0') {
                    snprintf(pctx->attr.cert_path, sizeof(pctx->attr.cert_path),
                             "%s", pItem->valuestring);
                    if_process_cert_root_trim(pctx->attr.cert_path);
                    if (pctx->debug)
                        printf("MQTT cert root: %s\n", pctx->attr.cert_path);
                }
                cJSON_Delete(pJson);
            }
        }
        db_cfg_jsonfile_delete(p);
    }

    if (pctx->attr.cert_path[0] != '\0') {
        if_process_cert_root_trim(pctx->attr.cert_path);
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

        /*
         * 透传已改为 MQTT 回调内同步读写 8502。
         * 这里绝不能再异步 read 同一 socket，否则会抢走应答导致云端「接收失败」。
         */
        sleep(1);

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
