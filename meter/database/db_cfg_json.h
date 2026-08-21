/*
* Copyright (c) 2013,珠海派诺电子有限公司
* All rights reserved.
*
* 文件名称：db_cfg_json.h
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
*  chenzy   2013.11.08   V1.00      创建
*/

#ifndef __DB_CFG_JSON_H__
#define __DB_CFG_JSON_H__


/*********************************************************************************************************
** Function name:       db_cfg_json_file_alloc
** Descriptions:        if a file exists, space is dynamically allocated for loading file data
** input parameters:    file    - file name
** output parameters:   size    - file size
** Returned value:      NULL    - file dosen't exit
                        else    - Allocated memory head address
*********************************************************************************************************/
char *db_cfg_json_file_alloc(const char *file, int *size);

/*********************************************************************************************************
** Function name:       db_cfg_json_file_delete
** Descriptions:        release resource
** input parameters:    p       - file pointer
** output parameters:   none
** Returned value:      none
*********************************************************************************************************/
void db_cfg_json_file_delete(char *p);

/*********************************************************************************************************
** Function name:       db_cfg_json_file_read
** Descriptions:        read json file
** input parameters:    file    - json file
                        size    - file size
** output parameters:   p       - file pointer
** Returned value:      NULL    - failed
                        else    - successed
*********************************************************************************************************/
char *db_cfg_json_file_read(const char *file, char *p, int size);

/*********************************************************************************************************
** Function name:       db_cfg_json_file_write
** Descriptions:        read json file
** input parameters:    jsonstring  - string
** output parameters:   file        - json file
** Returned value:      0           - successed
                        -1          - failed
*********************************************************************************************************/
int db_cfg_json_file_write(char *file, char *jsonstring);


#endif
