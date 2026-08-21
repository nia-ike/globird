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


#ifndef __INFO_WORKING_H__
#define __INFO_WORKING_H__

#include <stdint.h>
#include <stdbool.h>

#include "rm_cfg_attr.h"

/*ID offset 请查看 work_condition.json*/
enum 
{
    INFO_193_OFFSET_SN = 0,
    INFO_193_OFFSET_IP = 1,
    INFO_193_OFFSET_NETMASK = 2,
    INFO_193_OFFSET_GATEWAY = 3,
    INFO_193_OFFSET_MAC = 4,
    INFO_193_OFFSET_IMEI = 5,   // 占用2个units
    INFO_193_OFFSET_ICCID = 7,
    INFO_193_OFFSET_CPU = 10,
    INFO_193_OFFSET_MOMERY = 11,
    INFO_193_OFFSET_UPTIME = 12,
    INFO_193_OFFSET_CSQ = 13,
    INFO_193_OFFSET_RSRP = 14,
    INFO_193_OFFSET_SIM = 15,
    INFO_193_OFFSET_CREG = 16,
    INFO_193_OFFSET_DISK = 17,
};





char *info_get_sn(struct remote_m_ctx_t *pctx);
int info_get_csq(struct remote_m_ctx_t *pctx);
int info_get_rsrp(struct remote_m_ctx_t *pctx);
float info_get_ramfree(struct remote_m_ctx_t *pctx);
float info_get_diskfree(struct remote_m_ctx_t *pctx);
char *info_get_imei(struct remote_m_ctx_t *pctx);
char *info_get_iccid(struct remote_m_ctx_t *pctx);
char *info_get_version(struct remote_m_ctx_t *pctx);

#endif