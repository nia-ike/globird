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


#ifndef __IF_PROCESS_H__
#define __IF_PROCESS_H__

#include <stdint.h>
#include <stdbool.h>

#include "mqtt_fops.h"
#include "if_data_json.h"
#include "mosquitto.h"



#define VPN_MAX_TIME     (2*60*60)


struct if_process_thread_param_t
{
    struct remote_m_ctx_t *pctx;         // thread 用于回应发布时调用mqtt接口用途
    void                  *parameter;      // 用于适配不同线程参数装载
};




int if_process_vpn_conn(struct remote_m_ctx_t *pctx, struct mqtt_msg_t *pmqtt_recieve_msg);
int if_process_vpn_disconn(struct remote_m_ctx_t *pctx, struct mqtt_msg_t *pmqtt_recieve_msg);
int if_process_heartbeat_start(struct remote_m_ctx_t *pctx);

#endif
