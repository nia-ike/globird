/*
 * db_cfg_json.c
 * JSON 文件读写缓冲区实现，与 application/ipk/mqtt_standard/src/config 风格一致。
 */

#include "db_cfg_json.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

/*********************************************************************************************************
** Function name:       db_cfg_jsonfile_alloc
** Descriptions:        若文件存在则分配缓冲区，用于后续读取 JSON 文件内容
** input parameters:    file - 文件路径
** output parameters:   size - 文件大小（字节）
** Returned value:      NULL-失败；否则为缓冲区首地址
*********************************************************************************************************/
char *db_cfg_jsonfile_alloc(char *file, int *size)
{
    struct stat filestat;

    if (NULL == file)
        return NULL;

    if (0 != stat(file, &filestat))
    {
        printf("Json file[%s] not exist\n", file);
        return NULL;
    }

    *size = (int)filestat.st_size;
    return (char *)malloc((size_t)filestat.st_size + 1);
}

/*********************************************************************************************************
** Function name:       db_cfg_jsonfile_delete
** Descriptions:        释放由 db_cfg_jsonfile_alloc 分配的缓冲区
** input parameters:    p - 缓冲区指针
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void db_cfg_jsonfile_delete(char *p)
{
    if (NULL == p)
        return;
    free(p);
}

/*********************************************************************************************************
** Function name:       db_cfg_jsonfile_read
** Descriptions:        将指定 JSON 文件读入已分配的缓冲区
** input parameters:    file, p, size
** output parameters:   p - 末尾已加 NUL 的内容
** Returned value:      NULL-失败；否则返回 p
*********************************************************************************************************/
char *db_cfg_jsonfile_read(char *file, char *p, int size)
{
    int   n;
    FILE *fp;

    fp = fopen(file, "rb");
    if (fp == NULL)
        return NULL;

    n = (int)fread(p, 1, (size_t)size, fp);
    if (n <= 0)
        goto __err_close;

    p[n] = '\0';
    fclose(fp);
    return p;

__err_close:
    fclose(fp);
    return NULL;
}

/*********************************************************************************************************
** Function name:       db_cfg_jsonfile_write
** Descriptions:        将 JSON 字符串写入指定文件
** input parameters:    file - 文件路径；jsonstring - 要写入的字符串
** output parameters:   NONE
** Returned value:      0-成功；-1-失败
*********************************************************************************************************/
int db_cfg_jsonfile_write(char *file, char *jsonstring)
{
    FILE *fp;

    if (!file || !jsonstring)
        return -1;

    fp = fopen(file, "wt");
    if (fp == NULL)
        return -1;

    fprintf(fp, "%s", jsonstring);
    fclose(fp);
    return 0;
}
