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

#include "info_working.h"
#include "db_node_fops.h"

#define VIRTUAL_CHNL_NO    193
#define VIRTUAL_DEV_NO     1


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
char *info_get_sn(struct remote_m_ctx_t *pctx)
{
    return pctx->sn;
}



/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int info_get_csq(struct remote_m_ctx_t *pctx)
{
    int ret;
    double lfval;

    ret = DataMgmtFops.readpoint(VIRTUAL_CHNL_NO, VIRTUAL_DEV_NO, INFO_193_OFFSET_CSQ, &lfval);
    if(ret < 0)
        return 0;

    return (int)lfval;
}



/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int info_get_rsrp(struct remote_m_ctx_t *pctx)
{
    int ret;
    double lfval;

    ret = DataMgmtFops.readpoint(VIRTUAL_CHNL_NO, VIRTUAL_DEV_NO, INFO_193_OFFSET_RSRP, &lfval);
    if(ret < 0)
        return 0;
        
    return (int)lfval;
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
float info_get_ramfree(struct remote_m_ctx_t *pctx)
{
    int ret;
    double lfval;

    ret = DataMgmtFops.readpoint(VIRTUAL_CHNL_NO, VIRTUAL_DEV_NO, INFO_193_OFFSET_MOMERY, &lfval);
    if(ret < 0)
        return 0;
        
    return (float)lfval;
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
float info_get_diskfree(struct remote_m_ctx_t *pctx)
{
    int ret;
    double lfval;

    ret = DataMgmtFops.readpoint(VIRTUAL_CHNL_NO, VIRTUAL_DEV_NO, INFO_193_OFFSET_DISK, &lfval);
    if(ret < 0)
        return 0;

    return (float)lfval;
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
char *info_get_imei(struct remote_m_ctx_t *pctx)
{
    int ret;
    static char imei[16];

    ret = DataMgmtFops.readpoint_string(VIRTUAL_CHNL_NO, VIRTUAL_DEV_NO, INFO_193_OFFSET_IMEI, imei, sizeof(imei));
    if(ret < 0)
        return "NULL";
        
    return imei;
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
char *info_get_iccid(struct remote_m_ctx_t *pctx)
{
    int ret;
    static char iccid[24];

    ret = DataMgmtFops.readpoint_string(VIRTUAL_CHNL_NO, VIRTUAL_DEV_NO, INFO_193_OFFSET_ICCID, iccid, sizeof(iccid));
    if(ret < 0)
        return "NULL";
        
    return iccid;
}

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
char *info_get_version(struct remote_m_ctx_t *pctx)
{
    return pctx->version;
}

