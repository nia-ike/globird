/*
 * db_cfg_json.h
 * JSON 文件读写缓冲区接口，与 application/ipk/mqtt_standard/src/config 风格一致。
 * 用于防逆流策略 config 目录下的 JSON 解析。
 */

#ifndef __DB_CFG_JSON_H__
#define __DB_CFG_JSON_H__

/*********************************************************************************************************
** Function name:       db_cfg_jsonfile_alloc
** Descriptions:        if a file exists, space is dynamically allocated for loading file data
** input parameters:    file    - file name
** output parameters:   size    - file size
** Returned value:      NULL    - file dosen't exit
                       else    - Allocated memory head address
*********************************************************************************************************/
char *db_cfg_jsonfile_alloc(char *file, int *size);

/*********************************************************************************************************
** Function name:       db_cfg_jsonfile_delete
** Descriptions:        release resource
** input parameters:    p       - file pointer
** output parameters:   none
** Returned value:      none
*********************************************************************************************************/
void db_cfg_jsonfile_delete(char *p);

/*********************************************************************************************************
** Function name:       db_cfg_jsonfile_read
** Descriptions:        read json file
** input parameters:    file    - json file
                       size    - file size
** output parameters:   p       - file pointer
** Returned value:      NULL    - failed
                       else    - successed
*********************************************************************************************************/
char *db_cfg_jsonfile_read(char *file, char *p, int size);

/*********************************************************************************************************
** Function name:       db_cfg_jsonfile_write
** Descriptions:        write json string to file
** input parameters:    file        - json file path
                       jsonstring  - string to write
** output parameters:   none
** Returned value:      0           - successed
                       -1          - failed
*********************************************************************************************************/
int db_cfg_jsonfile_write(char *file, char *jsonstring);

#endif /* __DB_CFG_JSON_H__ */
