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
#include <sys/sysinfo.h>


#include "cJSON.h"
#include "cJSON_Utils.h"

#include "db_struct.h"
#include "db_node_fops.h"

#include "if_data_json.h"
#include "info_working.h"


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static __inline__ void if_msg_packet(struct mqtt_msg_t *pmsg, uint16_t funcode, uint32_t len)
{
    pmsg->func = funcode;
    pmsg->len = len;
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int if_msg_pack_heartbeat(struct remote_m_ctx_t *pctx, struct mqtt_msg_t *pmsg)
{
    int n;

    n = snprintf(pmsg->data, sizeof(pmsg->data), "{\"sn\":\"%s\",\"csq\":%d, \"rsrp\":%d, \"ramfree\":%.2f,\"diskfree\":%.2f}",
                info_get_sn(pctx), 
                info_get_csq(pctx),
                info_get_rsrp(pctx),
                info_get_ramfree(pctx),
                info_get_diskfree(pctx)
                );

    if_msg_packet(pmsg, FUNC_STATUS, n + sizeof(pmsg->func));

    return 0;
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int if_msg_pack_register(struct remote_m_ctx_t *pctx, struct mqtt_msg_t *pmsg)
{
    int n;
    struct sysinfo s_info;
    long uptime = 0;

    /* get uptime  */
    if(0==sysinfo(&s_info))
    {
        uptime = s_info.uptime;
    }

    n = snprintf(pmsg->data,  sizeof(pmsg->data), "{\"sn\":\"%s\",\"version\":\"%s\",\"iccid\":\"%s\",\"imei\":\"%s\", \"uptime\":%ld, \"project\":\"%s\", \"room\":\"%s\", \"gateway\":\"%s\",\"owner\":\"%s\"}",
            info_get_sn(pctx),
            info_get_version(pctx),
            info_get_iccid(pctx),
            info_get_imei(pctx),
            time(NULL) - uptime,
            pctx->attr.project.station,
            pctx->attr.project.rooms,
            pctx->attr.project.gatewayname,
            pctx->attr.project.owner);


    if_msg_packet(pmsg, FUNC_REGISTER, n + sizeof(pmsg->func));

    return 0;
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int if_msg_pack_will(struct remote_m_ctx_t *pctx, struct mqtt_msg_t *pmsg)
{
    int n;

    n = snprintf(pmsg->data, sizeof(pmsg->data), "{\"sn\":\"%s\"}",
                info_get_sn(pctx)
                );

    if_msg_packet(pmsg, FUNC_STATUS, n + sizeof(pmsg->func));

    return 0;
}






/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int if_msg_pack_connvpn(struct remote_m_ctx_t *pctx, struct mqtt_msg_t *pmsg, int result, char *ip)
{
    int n;

    n = snprintf(pmsg->data, sizeof(pmsg->data),  "{\"sn\":\"%s\",\"result\":%d, \"ip\":\"%s\"}",
                info_get_sn(pctx),
                result,
                ip
                );

    if_msg_packet(pmsg, FUNC_VPN_CON, n + sizeof(pmsg->func));

    return 0;
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int if_msg_pack_disconnvpn(struct remote_m_ctx_t *pctx, struct mqtt_msg_t *pmsg, int result)
{
    int n;

    n = snprintf(pmsg->data, sizeof(pmsg->data),   "{\"sn\":\"%s\",\"result\":%d}",
                info_get_sn(pctx),
                result
                );

    if_msg_packet(pmsg, FUNC_VPN_DISC, n + sizeof(pmsg->func));

    return 0;
}



/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int if_msg_pack_trans(struct remote_m_ctx_t *pctx, struct mqtt_msg_t *pmsg, uint8_t *rdata, uint16_t len)
{

    /* limit the length */
    len = len < sizeof(pmsg->data) ? len : sizeof(pmsg->data);
    /* copy to the ctx  */
    memcpy(pmsg->data, rdata, len);

    if_msg_packet(pmsg, FUNC_TRANS, len + sizeof(pmsg->func));

    return 0;
}




/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int if_msg_unpack_vpninfo(struct if_vpn_info_t *pvpn_info, char *rdata, uint16_t len)
{
    int ret = -1;
    cJSON *pitem = NULL;
    cJSON *cjson_root = cJSON_Parse(rdata);

    if(cjson_root == NULL)
        goto __err;

    /* server */
    pitem = cJSON_GetObjectItem(cjson_root, "server");
    if(pitem == NULL)
        goto __err;

    snprintf(pvpn_info->server, sizeof(pvpn_info->server), "%s", pitem->valuestring);

    /* passwd */
    pitem = cJSON_GetObjectItem(cjson_root, "passwd");
    if(pitem == NULL)
        goto __err;

    snprintf(pvpn_info->passwd, sizeof(pvpn_info->passwd), "%s", pitem->valuestring);

    /* ip */
    pitem = cJSON_GetObjectItem(cjson_root, "ip");
    if(pitem == NULL)
        goto __err;

    snprintf(pvpn_info->ip, sizeof(pvpn_info->ip), "%s", pitem->valuestring);

    /* group */
    pitem = cJSON_GetObjectItem(cjson_root, "group");
    if(pitem == NULL)
        goto __err;

    snprintf(pvpn_info->group, sizeof(pvpn_info->group), "%s", pitem->valuestring);

    ret = 0;

__err:
    cJSON_Delete(cjson_root);

    return ret;
}