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


#ifndef __MQTT_FOPS_H__
#define __MQTT_FOPS_H__

#include <stdint.h>
#include <stdbool.h>

#include "mosquitto.h"

#include "rm_cfg_attr.h"


#define MQTT_TOPIC_PREFIX ""
#define MQTT_TOPIC_WILL "EGO700-WILL"
#define MQTT_TOPIC_TRANS_SUFFIX "-TRANS"

#define MQTT_QOS  0


struct mqtt_msg_t
{
    uint32_t len;
    uint16_t func;
    char     data[1280];
};


int mqtt_init(struct remote_m_ctx_t *pctx);
void mqtt_process(struct remote_m_ctx_t *pctx);
void mqtt_destroy(struct remote_m_ctx_t *pctx);

#endif
