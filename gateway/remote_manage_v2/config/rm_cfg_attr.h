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


#ifndef __RM_CFG_ATTR_H__
#define __RM_CFG_ATTR_H__

#include <stdint.h>
#include <stdbool.h>

#include "db_struct.h"
#include "db_cfg_json.h"
#include "com_baseclass.h"


struct remote_m_project_t
{
    char station[64];
    char rooms[64];
    char gatewayname[64];
    char owner[64];
    char company[64];
};


struct remote_m_attr_t
{
    uint8_t checked;     
      
    char    url[64];   
    char    user[32];
    char    passwd[32];

    /* split from url */
    char    ip[64];
    int     port;

    struct remote_m_project_t project;
};


struct remote_m_ctx_t
{
    uint8_t                 debug;
    char                    sn[32];
    char                    version[32];
    struct remote_m_attr_t  attr;
    struct db_spl_realtime_data_tbl_t *pDB;
    struct mosquitto        *mosq;

    time_t                  vpn_starttime;
    bool                    vpn_abort;

    struct hal_comm_ctx_t   hal_com;
};


int rm_cfg_load(struct remote_m_ctx_t *pCtx, char *file);
int rm_cfg_project_info_load(struct remote_m_ctx_t *pCtx, char *file);



#endif
