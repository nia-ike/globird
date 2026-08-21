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

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <unistd.h> 
#include <sys/stat.h>  //stat

#include "mb_configfile.h"
#include "db_msgevent_api.h"

static struct mb_config_ctx_t mb_config_ctx;


/*********************************************************************************************************
** Function name:       
** Descriptions:       
** input parameters:             
** output parameters:   NONE
** Returned value:      
*********************************************************************************************************/
struct mb_config_ctx_t *mb_file_config_getctx(void)
{
    return &mb_config_ctx;
}

/*********************************************************************************************************
** Function name:       
** Descriptions:       
** input parameters:             
** output parameters:   NONE
** Returned value:      
*********************************************************************************************************/
void mb_file_config(struct mb_config_ctx_t *pNewCtx)
{
    struct mb_config_ctx_t *pLocalCtx;

    pLocalCtx = mb_file_config_getctx();
    pLocalCtx->packsize = pNewCtx->packsize;

    /*
        需要更新的配置文件与本地保存的信息不一致，因此更新新的配置文件，且更新进度为0；
    */
    if((pNewCtx->curoffset != pLocalCtx->curoffset))
    {
        pLocalCtx->curoffset    = pNewCtx->curoffset;
        return;
    }

    /*
        容错。
    */
    if(pLocalCtx->curoffset >= pNewCtx->filesize)
        pLocalCtx->curoffset = pNewCtx->filesize;
}

/*********************************************************************************************************
** Function name:       
** Descriptions:       
** input parameters:             
** output parameters:   NONE
** Returned value:      
*********************************************************************************************************/
static int mb_file_config_read(struct mb_config_ctx_t *pCtx, uint8_t *pbuf, uint16_t len)
{
    FILE *fp;

    fp = fopen(MB_CONFIG_FILE, "rb");
    if(fp == NULL)
        return -1;

    fseek(fp, pCtx->curoffset, SEEK_SET);

    fread(pbuf, 1, len, fp);

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
static int mb_file_config_write(struct mb_config_ctx_t *pCtx, uint8_t *pbuf, uint16_t len)
{
    FILE *fp;

    if( pCtx->curoffset == 0)
    {
        fp = fopen(MB_CONFIG_FILE, "wb+");
    }
    else
    {
        fp = fopen(MB_CONFIG_FILE, "rb+");
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
int mb_file_upload(struct mb_config_ctx_t *pNewCtx, uint8_t *pbuf, uint16_t len)
{
    int ret;
    ret = mb_file_config_read(pNewCtx, pbuf, len);
    if(ret < 0)
        return -1;


    return 0;
}

/*********************************************************************************************************
** Function name:       
** Descriptions:       
** input parameters:             
** output parameters:   NONE
** Returned value:      
*********************************************************************************************************/
int mb_file_download(struct mb_config_ctx_t *pNewCtx, uint8_t *pbuf, uint16_t len)
{
    int ret;
    struct mb_config_ctx_t *pLocalCtx;

    pLocalCtx = mb_file_config_getctx();

    /*
        首包。更新配置文件信息表。
    */
    if(pNewCtx->curoffset == 0)
    {
        pLocalCtx->filesize     = pNewCtx->filesize;
        pLocalCtx->filecrc      = pNewCtx->filecrc;
        pLocalCtx->curoffset    = 0;

        unlink(MB_CONFIG_FILE);     
    }

    ret = mb_file_config_write(pLocalCtx, pbuf, len);
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
int mb_file_update()
{
    system("rm -rf /userdata/config/collect/device/* >/dev/null");
    system("tar zxvf /tmp/config.tar.gz -C /userdata/config/ >/dev/null");
    db_msg_event_cmd(DB_EVENT_TYPE_REBOOT);
}


/*********************************************************************************************************
** Function name:       
** Descriptions:       
** input parameters:             
** output parameters:   NONE
** Returned value:      
*********************************************************************************************************/
int mb_file_delete(struct mb_config_ctx_t *pNewCtx)
{
    struct mb_config_ctx_t *pLocalCtx;

    pLocalCtx = mb_file_config_getctx();
    /*
        download finish. verity the file, and update
    */
    if(pLocalCtx->curoffset == pLocalCtx->filesize)
    {
        unlink(MB_CONFIG_FILE);     
        return 0;
    }
    return -1;
}

/*********************************************************************************************************
** Function name:       
** Descriptions:       
** input parameters:             
** output parameters:   NONE
** Returned value:      
*********************************************************************************************************/
//生成配置压缩文件
uint32_t create_downconfig_file(int totalsize)
{
    struct stat file_stat;
    if(stat(MB_CONFIG_FILE,  &file_stat) < 0)
    {
        unlink(MB_CONFIG_FILE);
        system("cd /userdata/config;tar zcvf /tmp/config.tar.gz * /ego/system/version >/dev/null 2>&1");
    }
    stat(MB_CONFIG_FILE, &file_stat);

    return file_stat.st_size;
}

/*********************************************************************************************************
** Function name:       
** Descriptions:       
** input parameters:             
** output parameters:   NONE
** Returned value:      
*********************************************************************************************************/
int mb_shell_command(uint8_t *pbuf, uint16_t len, uint8_t *value)
{
    char shellcmd[128] = {0};
    char *result;
    FILE *ptr;
    snprintf(shellcmd,len,"%d",pbuf);
    memset((void*)result,0,len);
    if((ptr=popen(shellcmd, "w"))!=NULL)
    {
        fgets(result, len, ptr);
        result[len] = 0;
        value = (uint8_t*)result;
        pclose(ptr);
        ptr = NULL;
        return 0;
    }
    else
    {
        return -1;
    }
}
