#include "db_cfg_json.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/*************************************************************************/
/** Function Name:       db_cfg_jsonfile_alloc                             **/
/** Purpose:             若 JSON 文件存在则按文件大小分配读缓冲区           **/
/** Params:            file - 文件路径；size - 输出文件大小（字节）        **/
/** Return:            NULL-失败；否则为缓冲区首地址                       **/
/** Notice:            分配大小为文件大小 + 1，便于追加字符串结束符         **/
/*************************************************************************/
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

/*************************************************************************/
/** Function Name:       db_cfg_jsonfile_delete                            **/
/** Purpose:             释放 db_cfg_jsonfile_alloc 分配的缓冲区            **/
/** Params:            p - 缓冲区指针                                      **/
/** Return:            None.                                               **/
/** Notice:            p 为 NULL 时直接返回                                **/
/*************************************************************************/
void db_cfg_jsonfile_delete(char *p)
{
    if (NULL == p)
        return;
    free(p);
}

/*************************************************************************/
/** Function Name:       db_cfg_jsonfile_read                              **/
/** Purpose:             将 JSON 文件内容读入已分配缓冲区                   **/
/** Params:            file - 文件路径；p - 缓冲区；size - 读取字节数      **/
/** Return:            NULL-失败；否则返回 p                               **/
/** Notice:            读完后在末尾写入 '\0'                                **/
/*************************************************************************/
char *db_cfg_jsonfile_read(char *file, char *p, int size)
{
    FILE *fp;

    if (NULL == file || NULL == p || size <= 0)
        return NULL;

    fp = fopen(file, "rb");
    if (NULL == fp)
    {
        printf("Json file[%s] open failed\n", file);
        return NULL;
    }

    if ((int)fread(p, 1, (size_t)size, fp) != size)
    {
        printf("Json file[%s] read failed\n", file);
        fclose(fp);
        return NULL;
    }

    p[size] = '\0';
    fclose(fp);
    return p;
}

/*************************************************************************/
/** Function Name:       db_cfg_jsonfile_write                             **/
/** Purpose:             将 JSON 字符串写入文件                             **/
/** Params:            file - 目标文件路径；jsonstring - JSON 字符串       **/
/** Return:            0-成功；-1-失败                                     **/
/** Notice:            以二进制方式覆盖写入                                **/
/*************************************************************************/
int db_cfg_jsonfile_write(char *file, char *jsonstring)
{
    FILE *fp;
    size_t len;

    if (NULL == file || NULL == jsonstring)
        return -1;

    fp = fopen(file, "wb");
    if (NULL == fp)
        return -1;

    len = strlen(jsonstring);
    if (fwrite(jsonstring, 1, len, fp) != len)
    {
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return 0;
}
