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
#include <limits.h>
#include <dirent.h>
#include <time.h>
#include <sys/types.h> 
#include <unistd.h> 
#include <sys/stat.h>  //stat

#include "mb_configfile.h"
#include "mb_product_cfg.h"
#include "md5.h"

static struct mb_config_ctx_t mb_config_ctx;
#define MB_CONFIG_BACKUP_DIR        "/userdata/config.bakup"
#define MB_CONFIG_BACKUP_MAX_COUNT  16

struct mb_backup_file_info
{
    long ts;
    char path[PATH_MAX];
};

/*********************************************************************************************************
** Function name:       mb_backup_ts_cmp_desc
** Descriptions:        backup file sort compare (desc by timestamp)
** input parameters:    a,b compare item pointers
** output parameters:   NONE
** Returned value:      >0/<0/0 for qsort compare
*********************************************************************************************************/
static int mb_backup_ts_cmp_desc(const void *a, const void *b)
{
    const struct mb_backup_file_info *fa = (const struct mb_backup_file_info *)a;
    const struct mb_backup_file_info *fb = (const struct mb_backup_file_info *)b;

    if (fa->ts < fb->ts)
    {
        return 1;
    }
    if (fa->ts > fb->ts)
    {
        return -1;
    }
    return 0;
}

/*********************************************************************************************************
** Function name:       mb_backup_dir_create
** Descriptions:        create backup directory when not exists
** input parameters:    dir_path backup directory path
** output parameters:   NONE
** Returned value:      0 success, -1 failed
*********************************************************************************************************/
static int mb_backup_dir_create(const char *dir_path)
{
    struct stat st;

    if(stat(dir_path, &st) == 0)
    {
        if(S_ISDIR(st.st_mode))
        {
            return 0;
        }
        return -1;
    }

    if(mkdir(dir_path, 0755) == 0)
    {
        return 0;
    }

    return -1;
}

/*********************************************************************************************************
** Function name:       mb_backup_rotate
** Descriptions:        keep only newest backup archives
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void mb_backup_rotate(void)
{
    DIR *dir = NULL;
    struct dirent *entry = NULL;
    struct mb_backup_file_info files[256];
    int file_count = 0;

    dir = opendir(MB_CONFIG_BACKUP_DIR);
    if (dir == NULL)
    {
        return;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        long ts = 0;
        char *endptr = NULL;
        size_t name_len;

        if (entry->d_name[0] == '.')
        {
            continue;
        }

        name_len = strlen(entry->d_name);
        if (name_len <= strlen(".tar.gz"))
        {
            continue;
        }
        if (strcmp(entry->d_name + name_len - strlen(".tar.gz"), ".tar.gz") != 0)
        {
            continue;
        }

        ts = strtol(entry->d_name, &endptr, 10);
        if ((endptr == entry->d_name) || (strcmp(endptr, ".tar.gz") != 0))
        {
            continue;
        }

        if (file_count >= (int)(sizeof(files) / sizeof(files[0])))
        {
            break;
        }

        files[file_count].ts = ts;
        snprintf(files[file_count].path, sizeof(files[file_count].path), "%s/%s", MB_CONFIG_BACKUP_DIR, entry->d_name);
        file_count++;
    }

    closedir(dir);
    dir = NULL;

    if (file_count <= MB_CONFIG_BACKUP_MAX_COUNT)
    {
        return;
    }

    qsort(files, file_count, sizeof(files[0]), mb_backup_ts_cmp_desc);

    while (file_count > MB_CONFIG_BACKUP_MAX_COUNT)
    {
        unlink(files[file_count - 1].path);
        file_count--;
    }
}

/*********************************************************************************************************
** Function name:       mb_backup_copy_file
** Descriptions:        copy backup tar file to target path
** input parameters:    src source path, dst destination path
** output parameters:   NONE
** Returned value:      0 success, -1 failed
*********************************************************************************************************/
static int mb_backup_copy_file(const char *src, const char *dst)
{
    FILE *fp_src = NULL;
    FILE *fp_dst = NULL;
    uint8_t buff[4096];
    size_t read_len = 0;

    fp_src = fopen(src, "rb");
    if (fp_src == NULL)
    {
        return -1;
    }

    fp_dst = fopen(dst, "wb");
    if (fp_dst == NULL)
    {
        fclose(fp_src);
        return -1;
    }

    while ((read_len = fread(buff, 1, sizeof(buff), fp_src)) > 0)
    {
        if (fwrite(buff, 1, read_len, fp_dst) != read_len)
        {
            fclose(fp_src);
            fclose(fp_dst);
            unlink(dst);
            return -1;
        }
    }

    fclose(fp_src);
    fclose(fp_dst);
    return 0;
}

/*********************************************************************************************************
** Function name:       mb_config_backup_save
** Descriptions:        save config tar into timestamped backup
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      0 success, -1 failed
*********************************************************************************************************/
static int mb_config_backup_save(void)
{
    struct stat st;
    long ts = 0;
    char backup_path[PATH_MAX];

    if (stat(MB_CONFIG_FILE, &st) != 0 || !S_ISREG(st.st_mode))
    {
        return -1;
    }

    if (mb_backup_dir_create(MB_CONFIG_BACKUP_DIR) != 0)
    {
        return -1;
    }

    ts = (long)time(NULL);
    if (ts <= 0)
    {
        ts = 1;
    }

    while (1)
    {
        snprintf(backup_path, sizeof(backup_path), "%s/%ld.tar.gz", MB_CONFIG_BACKUP_DIR, ts);
        if (access(backup_path, F_OK) != 0)
        {
            break;
        }
        ts++;
    }

    if (mb_backup_copy_file(MB_CONFIG_FILE, backup_path) != 0)
    {
        return -1;
    }

    mb_backup_rotate();
    return 0;
}


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
        memset(pLocalCtx->md5, 0, sizeof(pLocalCtx->md5));
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
    /* Keep timestamped tar backups before replacing live config. */
    mb_config_backup_save();
    system(MB_CONFIG_APPLY_CMD);
    MB_CONFIG_BEFORE_REBOOT();
    system("/ech/script/app_sync.sh");
    system("reboot -d 2 &");

    return 0;
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
uint32_t create_downconfig_file(struct mb_config_ctx_t *pNewCtx, int totalsize)
{
    struct stat file_stat;

    if(stat(MB_CONFIG_FILE,  &file_stat) < 0)
    {
        unlink(MB_CONFIG_FILE);
        system(MB_CONFIG_TAR_PACK_CMD);

        compute_file_md5(MB_CONFIG_FILE, pNewCtx->md5);  
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
int mb_shell_command(uint8_t *pbuf, uint16_t len, char *result, uint16_t result_len)
{
    char shellcmd[256] = {0};
    FILE *ptr;
    int ret = -1;

    // Check input parameters
    if (!pbuf || !result || len == 0) {
        return -1;
    }

    // Safely copy command to shellcmd buffer
    if (strlen((char*)pbuf) >= sizeof(shellcmd)) {
        return -1;  // Command too long
    }
    strncpy(shellcmd, (char*)pbuf, sizeof(shellcmd) - 1);
    shellcmd[sizeof(shellcmd) - 1] = '\0';

    // Open pipe for reading command output
    if ((ptr = popen(shellcmd, "r")) != NULL) {
        // Read command output
        if (fgets(result, result_len, ptr) != NULL) {
            // Remove newline character if present
            size_t result_len = strlen(result);
            if (result_len > 0 && result[result_len - 1] == '\n') {
                result[result_len - 1] = '\0';
                result_len--;
            }
            
            ret = 0;  // Success
        }
        
        // Close the pipe
        pclose(ptr);
        ptr = NULL;
    }

    return ret;
}
