#include "mb_datacache_file.h"
#include "md5.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

static struct mb_config_ctx_t mb_datacache_ctx;
static uint8_t s_mb_file46_devtype = 0;

struct mb_config_ctx_t *mb_datacache_file_getctx(void)
{
    return &mb_datacache_ctx;
}

void mb_file46_set_devtype(uint8_t devtype)
{
    s_mb_file46_devtype = devtype;
}

uint8_t mb_file46_get_devtype(void)
{
    return s_mb_file46_devtype;
}

void mb_datacache_file_config(struct mb_config_ctx_t *pNewCtx)
{
    struct mb_config_ctx_t *pLocalCtx = mb_datacache_file_getctx();

    pLocalCtx->packsize = pNewCtx->packsize;

    if (pNewCtx->curoffset != pLocalCtx->curoffset)
    {
        pLocalCtx->curoffset = pNewCtx->curoffset;
        return;
    }

    if (pLocalCtx->curoffset >= pNewCtx->filesize)
        pLocalCtx->curoffset = pNewCtx->filesize;
}

static int mb_datacache_snapshot(void)
{
    char cmd[256];

    if (access(MB_DATACACHE_SRC, F_OK) != 0)
        return -1;

    snprintf(cmd, sizeof(cmd), "cp \"%s\" \"%s\"", MB_DATACACHE_SRC, MB_DATACACHE_DL);
    return (system(cmd) == 0) ? 0 : -1;
}

static int mb_datacache_file_read(struct mb_config_ctx_t *pCtx, uint8_t *pbuf, uint16_t len)
{
    FILE *fp;

    fp = fopen(MB_DATACACHE_DL, "rb");
    if (fp == NULL)
        return -1;

    fseek(fp, (long)pCtx->curoffset, SEEK_SET);
    fread(pbuf, 1, len, fp);
    fclose(fp);

    return 0;
}

int mb_datacache_file_upload(struct mb_config_ctx_t *pNewCtx, uint8_t *pbuf, uint16_t len)
{
    if (mb_datacache_file_read(pNewCtx, pbuf, len) < 0)
        return -1;

    return 0;
}

uint32_t create_down_datacache_file(struct mb_config_ctx_t *pNewCtx, int totalsize)
{
    struct stat file_stat;
    int need_md5 = 0;

    (void)totalsize;

    if (pNewCtx->curoffset == 0 || access(MB_DATACACHE_DL, F_OK) != 0)
    {
        unlink(MB_DATACACHE_DL);
        if (mb_datacache_snapshot() != 0)
        {
            memset(pNewCtx->md5, 0, sizeof(pNewCtx->md5));
            return 0;
        }
        need_md5 = 1;
    }

    if (stat(MB_DATACACHE_DL, &file_stat) != 0 || !S_ISREG(file_stat.st_mode))
    {
        memset(pNewCtx->md5, 0, sizeof(pNewCtx->md5));
        return 0;
    }

    /* MD5 only when snapshot is (re)created — avoid hashing whole DB every packet. */
    if (need_md5)
        compute_file_md5(MB_DATACACHE_DL, pNewCtx->md5);

    return (uint32_t)file_stat.st_size;
}
