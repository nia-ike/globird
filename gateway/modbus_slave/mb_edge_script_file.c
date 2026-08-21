/*
 * edge_script.tar.gz 热更新：Modbus 4DH/4EH 分包传输，落盘后解压到
 * /userdata/config/edge_script，不 reboot（与 47H 全量 config 分离）。
 */

#include "mb_edge_script_file.h"
#include "md5.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <utime.h>

static struct mb_config_ctx_t mb_edge_script_ctx;

/*************************************************************************/
/** Function Name:       mb_edge_script_getctx                           **/
/** Purpose:             获取 edge_script 文件传输上下文                  **/
/** Params:            None.                                               **/
/** Return:            上下文指针                                          **/
/** Notice:            None.                                               **/
/*************************************************************************/
struct mb_config_ctx_t *mb_edge_script_getctx(void)
{
    return &mb_edge_script_ctx;
}

/*************************************************************************/
/** Function Name:       mb_edge_script_file_config                        **/
/** Purpose:             更新 4EH 下载请求的偏移与包长                    **/
/** Params:            pNewCtx - 上位机下发的传输参数                      **/
/** Return:            None.                                               **/
/** Notice:            逻辑同 mb_file_config                               **/
/*************************************************************************/
void mb_edge_script_file_config(struct mb_config_ctx_t *pNewCtx)
{
    struct mb_config_ctx_t *pLocalCtx;

    pLocalCtx = mb_edge_script_getctx();
    pLocalCtx->packsize = pNewCtx->packsize;

    if (pNewCtx->curoffset != pLocalCtx->curoffset)
    {
        pLocalCtx->curoffset = pNewCtx->curoffset;
        return;
    }

    if (pLocalCtx->curoffset >= pNewCtx->filesize)
        pLocalCtx->curoffset = pNewCtx->filesize;
}

/*************************************************************************/
/** Function Name:       mb_edge_script_file_read                          **/
/** Purpose:             从临时 tar 按偏移读取数据块                      **/
/** Params:            pCtx - 上下文；pbuf - 输出；len - 长度              **/
/** Return:            0-成功；-1-失败                                     **/
/** Notice:            None.                                               **/
/*************************************************************************/
static int mb_edge_script_file_read(struct mb_config_ctx_t *pCtx, uint8_t *pbuf, uint16_t len)
{
    FILE *fp;

    fp = fopen(MB_EDGE_SCRIPT_FILE, "rb");
    if (fp == NULL)
        return -1;

    fseek(fp, pCtx->curoffset, SEEK_SET);
    fread(pbuf, 1, len, fp);
    fclose(fp);

    return 0;
}

/*************************************************************************/
/** Function Name:       mb_edge_script_file_write                         **/
/** Purpose:             向临时 tar 按偏移写入数据块                      **/
/** Params:            pCtx - 上下文；pbuf - 数据；len - 长度              **/
/** Return:            0-成功；-1-失败                                     **/
/** Notice:            None.                                               **/
/*************************************************************************/
static int mb_edge_script_file_write(struct mb_config_ctx_t *pCtx, uint8_t *pbuf, uint16_t len)
{
    FILE *fp;

    if (pCtx->curoffset == 0)
        fp = fopen(MB_EDGE_SCRIPT_FILE, "wb+");
    else
        fp = fopen(MB_EDGE_SCRIPT_FILE, "rb+");

    if (fp == NULL)
        return -1;

    fseek(fp, pCtx->curoffset, SEEK_SET);
    fwrite(pbuf, 1, len, fp);
    fclose(fp);

    return 0;
}

/*************************************************************************/
/** Function Name:       mb_edge_script_file_upload                        **/
/** Purpose:             4EH 应答：读取 tar 分包数据                        **/
/** Params:            pNewCtx - 偏移；pbuf - 输出；len - 长度             **/
/** Return:            0-成功；-1-失败                                     **/
/** Notice:            None.                                               **/
/*************************************************************************/
int mb_edge_script_file_upload(struct mb_config_ctx_t *pNewCtx, uint8_t *pbuf, uint16_t len)
{
    if (mb_edge_script_file_read(pNewCtx, pbuf, len) < 0)
        return -1;
    return 0;
}

/*************************************************************************/
/** Function Name:       mb_edge_script_file_download                      **/
/** Purpose:             4DH 请求：接收 tar 分包并写入临时文件              **/
/** Params:            pNewCtx - 首包含 filesize/crc；pbuf - 数据         **/
/** Return:            0-成功；-1-失败                                     **/
/** Notice:            None.                                               **/
/*************************************************************************/
int mb_edge_script_file_download(struct mb_config_ctx_t *pNewCtx, uint8_t *pbuf, uint16_t len)
{
    struct mb_config_ctx_t *pLocalCtx;
    int ret;

    pLocalCtx = mb_edge_script_getctx();

    if (pNewCtx->curoffset == 0)
    {
        pLocalCtx->filesize  = pNewCtx->filesize;
        pLocalCtx->filecrc   = pNewCtx->filecrc;
        pLocalCtx->curoffset = 0;
        memset(pLocalCtx->md5, 0, sizeof(pLocalCtx->md5));
        unlink(MB_EDGE_SCRIPT_FILE);
    }

    ret = mb_edge_script_file_write(pLocalCtx, pbuf, len);
    if (ret < 0)
        return -1;

    pLocalCtx->curoffset += len;
    return 0;
}

/*************************************************************************/
/** Function Name:       mb_edge_script_staging_has_json                   **/
/** Purpose:             检查解压目录是否含 edge_scripts.json             **/
/** Params:            staging_dir - 解压根目录                            **/
/** Return:            1-存在；0-不存在                                    **/
/** Notice:            支持 tar 根目录或 edge_script/ 子目录两种布局       **/
/*************************************************************************/
static int mb_edge_script_staging_has_json(const char *staging_dir)
{
    char path[256];

    snprintf(path, sizeof(path), "%s/edge_scripts.json", staging_dir);
    if (access(path, R_OK) == 0)
        return 1;

    snprintf(path, sizeof(path), "%s/edge_script/edge_scripts.json", staging_dir);
    if (access(path, R_OK) == 0)
        return 1;

    return 0;
}

/*************************************************************************/
/** Function Name:       mb_edge_script_file_update                        **/
/** Purpose:             4DH 传完后解压并热更新 edge_script 目录          **/
/** Params:            None.                                               **/
/** Return:            0-成功；-1-失败                                     **/
/** Notice:            不 reboot；touch json 供 edge_script_service 轮询   **/
/*************************************************************************/
int mb_edge_script_file_update(void)
{
    char cmd[512];
    int ret;

    system("rm -rf /tmp/edge_script.staging >/dev/null 2>&1");
    system("mkdir -p /tmp/edge_script.staging >/dev/null 2>&1");

    snprintf(cmd, sizeof(cmd),
             "tar zxf %s -C /tmp/edge_script.staging >/dev/null 2>&1",
             MB_EDGE_SCRIPT_FILE);
    ret = system(cmd);
    if (ret != 0)
    {
        fprintf(stderr, "[modbus_slave] edge_script tar extract failed\n");
        return -1;
    }

    if (!mb_edge_script_staging_has_json("/tmp/edge_script.staging"))
    {
        fprintf(stderr, "[modbus_slave] edge_scripts.json missing in tar\n");
        system("rm -rf /tmp/edge_script.staging >/dev/null 2>&1");
        return -1;
    }

    system("rm -rf " MB_EDGE_SCRIPT_NEW_DIR " >/dev/null 2>&1");
    system("mkdir -p " MB_EDGE_SCRIPT_NEW_DIR "/scripts >/dev/null 2>&1");

    if (access("/tmp/edge_script.staging/edge_scripts.json", R_OK) == 0)
    {
        snprintf(cmd, sizeof(cmd),
                 "cp -a /tmp/edge_script.staging/. %s/ >/dev/null 2>&1",
                 MB_EDGE_SCRIPT_NEW_DIR);
    }
    else
    {
        snprintf(cmd, sizeof(cmd),
                 "cp -a /tmp/edge_script.staging/edge_script/. %s/ >/dev/null 2>&1",
                 MB_EDGE_SCRIPT_NEW_DIR);
    }

    ret = system(cmd);
    if (ret != 0)
    {
        fprintf(stderr, "[modbus_slave] edge_script staging copy failed\n");
        system("rm -rf /tmp/edge_script.staging " MB_EDGE_SCRIPT_NEW_DIR " >/dev/null 2>&1");
        unlink(MB_EDGE_SCRIPT_FILE);
        return -1;
    }

    snprintf(cmd, sizeof(cmd), "%s/edge_scripts.json", MB_EDGE_SCRIPT_NEW_DIR);
    if (access(cmd, R_OK) != 0)
    {
        fprintf(stderr, "[modbus_slave] edge_scripts.json missing in staged deploy\n");
        system("rm -rf /tmp/edge_script.staging " MB_EDGE_SCRIPT_NEW_DIR " >/dev/null 2>&1");
        unlink(MB_EDGE_SCRIPT_FILE);
        return -1;
    }

    system("rm -rf /userdata/config/edge_script.bak >/dev/null 2>&1");
    system("mv " MB_EDGE_SCRIPT_LIVE_DIR " /userdata/config/edge_script.bak >/dev/null 2>&1");
    ret = system("mv " MB_EDGE_SCRIPT_NEW_DIR " " MB_EDGE_SCRIPT_LIVE_DIR " >/dev/null 2>&1");
    system("rm -rf /tmp/edge_script.staging /userdata/config/edge_script.bak >/dev/null 2>&1");
    unlink(MB_EDGE_SCRIPT_FILE);

    if (ret != 0)
    {
        fprintf(stderr, "[modbus_slave] edge_script deploy rename failed\n");
        return -1;
    }

    if (access(MB_EDGE_SCRIPT_JSON, R_OK) != 0)
    {
        fprintf(stderr, "[modbus_slave] edge_scripts.json not deployed\n");
        return -1;
    }

    /* 更新 mtime，触发 edge_script_service 热 reload */
    utime(MB_EDGE_SCRIPT_JSON, NULL);
    printf("[modbus_slave] edge_script hot update applied\n");
    return 0;
}

/*************************************************************************/
/** Function Name:       mb_edge_script_create_download_file               **/
/** Purpose:             打包当前 edge_script 目录为 tar 供 4EH 下载      **/
/** Params:            pNewCtx - 可选，输出 md5                            **/
/** Return:            文件大小（字节）                                    **/
/** Notice:            缓存于 /tmp/edge_script.tar.gz                      **/
/*************************************************************************/
uint32_t mb_edge_script_create_download_file(struct mb_config_ctx_t *pNewCtx)
{
    struct stat file_stat;

    if (stat(MB_EDGE_SCRIPT_FILE, &file_stat) < 0)
    {
        unlink(MB_EDGE_SCRIPT_FILE);
        system("mkdir -p " MB_EDGE_SCRIPT_LIVE_DIR " >/dev/null 2>&1");
        system("cd " MB_EDGE_SCRIPT_LIVE_DIR " && tar zcvf " MB_EDGE_SCRIPT_FILE " * >/dev/null 2>&1");
        if (pNewCtx)
            compute_file_md5(MB_EDGE_SCRIPT_FILE, pNewCtx->md5);
    }

    if (stat(MB_EDGE_SCRIPT_FILE, &file_stat) < 0)
        return 0;

    return (uint32_t)file_stat.st_size;
}
