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


#ifndef __IF_DATA_JSON_H__
#define __IF_DATA_JSON_H__

#include <stdint.h>
#include <stdbool.h>

#include "mqtt_fops.h"
#include "rm_cfg_attr.h"




enum MQTT_MSG_FUNC
{
    FUNC_REGISTER = 0,  /*注册包*/
    FUNC_STATUS = 1,    /*状态包*/
    FUNC_VPN_CON,       /*连接VPN*/
    FUNC_VPN_DISC ,     /*断开VPN*/
    FUNC_UP_CONFIG,     /*上传配置*/
    FUNC_UP_FIRMWARE,   /*升级固件*/
    FUNC_TRANS,         /*透传modbusrtu*/
    FUNC_DOWN_CONFIG,   /*下载配置*/
    FUNC_REBOOT,        /*重启设备*/
    FUNC_WILL = 255,    /*遗嘱*/
};


struct if_vpn_info_t
{
    char server[64];
    char passwd[32];
    char ip[32];
    char group[32];
};


int if_msg_pack_heartbeat(struct remote_m_ctx_t *pctx, struct mqtt_msg_t *pmsg);
int if_msg_pack_register(struct remote_m_ctx_t *pctx, struct mqtt_msg_t *pmsg);
int if_msg_pack_will(struct remote_m_ctx_t *pctx, struct mqtt_msg_t *pmsg);
int if_msg_pack_connvpn(struct remote_m_ctx_t *pctx, struct mqtt_msg_t *pmsg, int result, char *ip);
int if_msg_pack_disconnvpn(struct remote_m_ctx_t *pctx, struct mqtt_msg_t *pmsg, int result);
int if_msg_pack_trans(struct remote_m_ctx_t *pctx, struct mqtt_msg_t *pmsg, uint8_t *rdata, uint16_t len);
int if_msg_unpack_vpninfo(struct if_vpn_info_t *pvpn_info, char *rdata, uint16_t len);


#endif
