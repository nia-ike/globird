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
#ifndef __MB_CONFIGFILE_H__
#define __MB_CONFIGFILE_H__

#include <stdint.h>

#define MB_CONFIG_FILE              "/tmp/config.tar.gz"


struct mb_config_ctx_t
{
    uint32_t    filesize;
    uint16_t    filecrc;
    uint32_t    curoffset;
    uint16_t    packsize;
};



struct mb_config_ctx_t *mb_file_config_getctx(void);
void mb_file_config(struct mb_config_ctx_t *pNewCtx);
int mb_file_download(struct mb_config_ctx_t *pNewCtx, uint8_t *pbuf, uint16_t len);
int mb_file_update(void);

int mb_file_upload(struct mb_config_ctx_t *pNewCtx, uint8_t *pbuf, uint16_t len);
int mb_file_delete(struct mb_config_ctx_t *pNewCtx);
uint32_t create_downconfig_file(int totalsize);
int mb_shell_command(uint8_t *pbuf, uint16_t len, uint8_t *value);

#endif
