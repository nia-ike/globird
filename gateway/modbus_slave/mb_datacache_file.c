#include "mb_datacache_file.h"
#include "mb_updatefile.h"
#include "md5.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>

static struct mb_config_ctx_t mb_datacache_ctx;
static uint8_t s_mb_file46_devtype = 0;

/* destinype=3 会话：s_hist_name 为 ".list" 或 "YYYYMMDD.db.gz" */
static char s_hist_name[32];
static char s_hist_read_path[MB_DATACACHE_PATH_MAX];
static int s_hist_is_list = 0;

struct mb_config_ctx_t *mb_datacache_file_getctx(void)
{
    return &mb_datacache_ctx;
}

void mb_file46_set_devtype(uint8_t destinype)
{
    s_mb_file46_devtype = destinype;
}

uint8_t mb_file46_get_devtype(void)
{
    return s_mb_file46_devtype;
}

void mb_datacache_history_clear_name(void)
{
    s_hist_name[0] = '\0';
    s_hist_read_path[0] = '\0';
    s_hist_is_list = 0;
}

static int mb_hist_is_ymd_db(const char *name)
{
    int i;

    if (name == NULL || strlen(name) != 14) /* YYYYMMDD.db.gz */
        return 0;
    if (strcmp(name + 8, ".db.gz") != 0)
        return 0;
    for (i = 0; i < 8; i++)
    {
        if (!isdigit((unsigned char)name[i]))
            return 0;
    }
    return 1;
}

static int mb_gzip_file(const char *plain, const char *gz, int fast)
{
    char cmd[384];
    struct stat st;

    if (plain == NULL || gz == NULL)
        return -1;

    unlink(gz);
    if (fast)
        snprintf(cmd, sizeof(cmd), "gzip -1 -c \"%s\" > \"%s\"", plain, gz);
    else
        snprintf(cmd, sizeof(cmd), "gzip -c \"%s\" > \"%s\"", plain, gz);

    if (system(cmd) != 0)
    {
        unlink(gz);
        return -1;
    }

    if (stat(gz, &st) != 0 || !S_ISREG(st.st_mode) || st.st_size < 10)
    {
        unlink(gz);
        return -1;
    }

    unlink(plain);
    return 0;
}

/*
 * 接受：".list" / "list" / 绝对路径以 /.list 结尾
 * 或 "YYYYMMDD.db.gz" / ".../YYYYMMDD.db.gz"
 */
int mb_datacache_history_set_name(const char *name, uint8_t name_len)
{
    char tmp[MB_DATACACHE_PATH_MAX];
    const char *base;

    mb_datacache_history_clear_name();
    if (name == NULL || name_len == 0 || name_len >= sizeof(tmp))
        return -1;

    memcpy(tmp, name, name_len);
    tmp[name_len] = '\0';

    base = strrchr(tmp, '/');
    base = (base != NULL) ? (base + 1) : tmp;

    if (strcmp(base, ".list") == 0 || strcmp(base, "list") == 0)
    {
        strncpy(s_hist_name, ".list", sizeof(s_hist_name) - 1);
        s_hist_is_list = 1;
        snprintf(s_hist_read_path, sizeof(s_hist_read_path), "%s", MB_DATACACHE_HISTORY_LIST_DL);
        return 0;
    }

    if (!mb_hist_is_ymd_db(base))
        return -1;

    strncpy(s_hist_name, base, sizeof(s_hist_name) - 1);
    s_hist_name[sizeof(s_hist_name) - 1] = '\0';
    s_hist_is_list = 0;
    snprintf(s_hist_read_path, sizeof(s_hist_read_path), "%s/%s",
             MB_DATACACHE_HISTORY_DIR, s_hist_name);
    return 0;
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

void mb_datacache_history_config(struct mb_config_ctx_t *pNewCtx)
{
    mb_datacache_file_config(pNewCtx);
}

static int mb_datacache_snapshot(void)
{
    char cmd[256];

    if (access(MB_DATACACHE_SRC, F_OK) != 0)
        return -1;

    unlink(MB_DATACACHE_DL);
    unlink(MB_DATACACHE_DL_PLAIN);

    snprintf(cmd, sizeof(cmd), "cp \"%s\" \"%s\"", MB_DATACACHE_SRC, MB_DATACACHE_DL_PLAIN);
    if (system(cmd) != 0)
    {
        unlink(MB_DATACACHE_DL_PLAIN);
        return -1;
    }

    if (mb_gzip_file(MB_DATACACHE_DL_PLAIN, MB_DATACACHE_DL, 1) != 0)
    {
        unlink(MB_DATACACHE_DL_PLAIN);
        unlink(MB_DATACACHE_DL);
        return -1;
    }

    return 0;
}

void mb_datacache_file_finish(void)
{
    unlink(MB_DATACACHE_DL);
    unlink(MB_DATACACHE_DL_PLAIN);
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
        unlink(MB_DATACACHE_DL_PLAIN);
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

    if (need_md5)
        compute_file_md5(MB_DATACACHE_DL, pNewCtx->md5);

    return (uint32_t)file_stat.st_size;
}

static int mb_hist_build_list_file(void)
{
    DIR *dir;
    struct dirent *ent;
    FILE *fp;
    char path[MB_DATACACHE_PATH_MAX];
    struct stat st;

    unlink(MB_DATACACHE_HISTORY_LIST_DL);
    fp = fopen(MB_DATACACHE_HISTORY_LIST_DL, "wb");
    if (fp == NULL)
        return -1;

    dir = opendir(MB_DATACACHE_HISTORY_DIR);
    if (dir == NULL)
    {
        fclose(fp);
        /* 目录不存在：空列表仍算成功 */
        return 0;
    }

    while ((ent = readdir(dir)) != NULL)
    {
        if (!mb_hist_is_ymd_db(ent->d_name))
            continue;
        snprintf(path, sizeof(path), "%s/%s", MB_DATACACHE_HISTORY_DIR, ent->d_name);
        if (stat(path, &st) != 0 || !S_ISREG(st.st_mode))
            continue;
        fprintf(fp, "%s\t%ld\n", ent->d_name, (long)st.st_size);
    }

    closedir(dir);
    fclose(fp);
    return 0;
}

static int mb_hist_file_read(struct mb_config_ctx_t *pCtx, uint8_t *pbuf, uint16_t len)
{
    FILE *fp;

    if (s_hist_read_path[0] == '\0')
        return -1;

    fp = fopen(s_hist_read_path, "rb");
    if (fp == NULL)
        return -1;

    fseek(fp, (long)pCtx->curoffset, SEEK_SET);
    fread(pbuf, 1, len, fp);
    fclose(fp);
    return 0;
}

int mb_datacache_history_upload(struct mb_config_ctx_t *pNewCtx, uint8_t *pbuf, uint16_t len)
{
    if (mb_hist_file_read(pNewCtx, pbuf, len) < 0)
        return -1;
    return 0;
}

uint32_t create_down_datacache_history(struct mb_config_ctx_t *pNewCtx, int totalsize)
{
    struct stat file_stat;
    int need_md5 = 0;

    (void)totalsize;

    if (s_hist_name[0] == '\0')
    {
        memset(pNewCtx->md5, 0, sizeof(pNewCtx->md5));
        return 0;
    }

    if (s_hist_is_list)
    {
        /* 列表每次首包重建到 /tmp */
        if (pNewCtx->curoffset == 0 || access(MB_DATACACHE_HISTORY_LIST_DL, F_OK) != 0)
        {
            if (mb_hist_build_list_file() != 0)
            {
                memset(pNewCtx->md5, 0, sizeof(pNewCtx->md5));
                return 0;
            }
            need_md5 = 1;
        }
        snprintf(s_hist_read_path, sizeof(s_hist_read_path), "%s", MB_DATACACHE_HISTORY_LIST_DL);
    }
    else
    {
        /* 日库：直接读 /userdata/history/YYYYMMDD.db.gz，不 cp */
        snprintf(s_hist_read_path, sizeof(s_hist_read_path), "%s/%s",
                 MB_DATACACHE_HISTORY_DIR, s_hist_name);
        if (pNewCtx->curoffset == 0)
            need_md5 = 1;
    }

    if (stat(s_hist_read_path, &file_stat) != 0 || !S_ISREG(file_stat.st_mode))
    {
        memset(pNewCtx->md5, 0, sizeof(pNewCtx->md5));
        return 0;
    }

    if (need_md5)
        compute_file_md5(s_hist_read_path, pNewCtx->md5);

    return (uint32_t)file_stat.st_size;
}

void mb_datacache_history_finish(void)
{
    /* 仅清理临时列表；日库源文件不删 */
    if (s_hist_is_list)
        unlink(MB_DATACACHE_HISTORY_LIST_DL);
}
