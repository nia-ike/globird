#include "cJSON_Utils.h"
#include "cJSON.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stddef.h>
#include "db_cfg_json.h"


/*********************************************************************************************************
** Function name:       db_cfg_json_file_alloc
** Descriptions:        if a file exists, space is dynamically allocated for loading file data
** input parameters:    file    - file name
** output parameters:   size    - file size
** Returned value:      NULL    - file dosen't exit
                        else    - Allocated memory head address
*********************************************************************************************************/
char *db_cfg_jsonfile_alloc(char *file, int *size)
{
    struct stat filestat;

    if (NULL == file)
        return NULL;

    /* get file size */
    if (0 != stat(file, &filestat))
    {
        printf("Json file[%s] not exit\n", file);
        return NULL;
    }

    *size = filestat.st_size;
    return malloc(filestat.st_size + 1);
}

/*********************************************************************************************************
** Function name:       db_cfg_json_file_delete
** Descriptions:        release resource
** input parameters:    p       - file pointer
** output parameters:   none
** Returned value:      none
*********************************************************************************************************/
void db_cfg_jsonfile_delete(char *p)
{
    if (NULL == p)
        return;

    free(p);
}

/*********************************************************************************************************
** Function name:       db_cfg_json_file_read
** Descriptions:        read json file
** input parameters:    file    - json file
                        size    - file size
** output parameters:   p       - file pointer
** Returned value:      NULL    - failed
                        else    - successed
*********************************************************************************************************/
char *db_cfg_jsonfile_read(char *file, char *p, int size)
{
    int     n;
    FILE    *fp;
    
    fp = fopen(file, "rb");
    if(fp == NULL)
        return NULL;

    n = fread(p, 1, size, fp);
    if(n <= 0)
        goto __err_close;

    fclose(fp);
    return p;

__err_close:
    fclose(fp);

    return NULL;
}

/*********************************************************************************************************
** Function name:       db_cfg_json_file_write
** Descriptions:        read json file
** input parameters:    jsonstring  - string
** output parameters:   file        - json file
** Returned value:      0           - successed
                        -1          - failed
*********************************************************************************************************/
int db_cfg_jsonfile_write(char *file, char *jsonstring)
{
    FILE *fp = NULL;

    if (!file)
        return -1;

    if (!jsonstring)
        return -1;

    fp = fopen(file, "wt");
    if (NULL == fp)
        return -1;

    fprintf(fp, "%s", jsonstring);
    fclose(fp);

    return 0;
}
