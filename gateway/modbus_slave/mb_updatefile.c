/*
* Copyright (c) 
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

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>  //stat

#include "mb_updatefile.h"
#include "mb_product_cfg.h"

static struct mb_update_ctx_t mb_update_ctx;


/*********************************************************************************************************
** Function name:
** Descriptions:
** input parameters:
** output parameters:   NONE
** Returned value:
*********************************************************************************************************/
struct mb_update_ctx_t *mb_file_getctx(void)
{
    return &mb_update_ctx;
}

/*********************************************************************************************************
** Function name:
** Descriptions:
** input parameters:
** output parameters:   NONE
** Returned value:
*********************************************************************************************************/
void mb_file_info_upate(struct mb_update_ctx_t *pNewCtx)
{
    struct mb_update_ctx_t *pLocalCtx;

    pLocalCtx = mb_file_getctx();

    /*
        需要升级的固件与本地保存的信息不一致，因此更新新的固件信息，且升级进度为0；
    */
    if((pNewCtx->filesize != pLocalCtx->filesize) || (pNewCtx->filecrc != pLocalCtx->filecrc))
    {
        pLocalCtx->filesize     = pNewCtx->filesize;
        pLocalCtx->filecrc      = pNewCtx->filecrc;
        pLocalCtx->curoffset    = 0;

        return;
    }

    /*
        容错。
    */
    if(pLocalCtx->curoffset >= pNewCtx->filesize)
        pLocalCtx->curoffset = 0;
}


/*********************************************************************************************************
** Function name:
** Descriptions:
** input parameters:
** output parameters:   NONE
** Returned value:
*********************************************************************************************************/
static int mb_file_firmware_write(struct mb_update_ctx_t *pCtx, uint8_t *pbuf, uint16_t len)
{
    FILE *fp;

    if( pCtx->curoffset == 0)
    {
        fp = fopen(MB_UPDATE_FILE, "wb+");
    }
    else
    {
        fp = fopen(MB_UPDATE_FILE, "rb+");
    }
    if(fp == NULL)
        return -1;

    fseek(fp, pCtx->curoffset, SEEK_SET);

    fwrite(pbuf, 1, len, fp);

    fclose(fp);

    return 0;
}


/*********************************************************************************************************
** Function name:
** Descriptions:
** input parameters:
** output parameters:   NONE
** Returned value:
*********************************************************************************************************/
int mb_file_pkg_download(struct mb_update_ctx_t *pNewCtx, uint8_t *pbuf, uint16_t len)
{
    int ret;
    struct mb_update_ctx_t *pLocalCtx;

    pLocalCtx = mb_file_getctx();

    /*
        首包。更新固件信息表。
    */
    if(pNewCtx->curoffset == 0)
    {
        pLocalCtx->filesize     = pNewCtx->filesize;
        pLocalCtx->filecrc      = pNewCtx->filecrc;
        pLocalCtx->curoffset    = 0;

        unlink(MB_UPDATE_FILE);
    }
    else
    {
        pLocalCtx->curoffset    = pNewCtx->curoffset;
    }

    ret = mb_file_firmware_write(pLocalCtx, pbuf, len);
    if(ret < 0)
        return -1;

    pLocalCtx->curoffset += len;

    return 0;
}


/*********************************************************************************************************
** Function name:
** Descriptions:
** input parameters:
** output parameters:   NONE
** Returned value:
*********************************************************************************************************/
int mb_file_pkg_upgrade(struct mb_update_ctx_t *pNewCtx)
{
    struct mb_update_ctx_t *pLocalCtx;

    pLocalCtx = mb_file_getctx();
    /*
        download finish. verity the file, and update
    */
    if(pLocalCtx->curoffset == pLocalCtx->filesize)
    {
        /* verity / product hook before upgrade */
        MB_UPDATE_BEFORE_UPGRADE();

        /* persist to /userdata/ota then opkg (power-loss safe) */
        system("/ech/script/ota_ipk.sh install /tmp/update.ipk &");
        return 0;
    }

    return -1;
}
