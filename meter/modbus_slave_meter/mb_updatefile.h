/*
* Copyright (c) 2013,珠海派诺电子有限公司
* All rights reserved.
* 
* 文件名称：
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
*  chenzy   2020.6.09   V1.00      创建
*/
#ifndef __MB_UPDATEFILE_H__
#define __MB_UPDATEFILE_H__


#define MB_UPDATE_FILE              "/tmp/update.ipk"


enum{
    MB_FILE_DEV_TYPE = 1,           // EGO700

};
    


struct mb_update_ctx_t
{
    uint32_t    filesize;
    uint16_t    filecrc;
    uint32_t    curoffset;
};



struct mb_update_ctx_t *mb_file_getctx(void);
void mb_file_info_upate(struct mb_update_ctx_t *pNewCtx);
int mb_file_pkg_download(struct mb_update_ctx_t *pNewCtx, uint8_t *pbuf, uint16_t len);
int mb_file_pkg_upgrade(struct mb_update_ctx_t *pNewCtx);
#endif
