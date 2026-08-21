
/*
*  Copyright (c) 2019,广东碳索智控科技有限公司
*  All rights reserved.
*
*  文件名称：msr_process.c
*  文件标识：
*  描    述：
*
*  当前版本：V1.0
*  作    chenzy
*  运行环境：
*  操作系统: freertos
*  硬件平台：esp32c3
*  固件库  ：
*  修改记录
*  作者       时间         版本      修改描述
*  chenzy    2025       V1.00
*/


#ifndef __OTA_HTTP_H__
#define __OTA_HTTP_H__


#include "db_struct.h"
#include "mb_updatefile.h"

#define MB_UPDATE_FILE              "/tmp/update.ipk"


// 线程参数结构体
typedef struct {
    struct db_database_type *pdb;
    const char *url;
    const char *filename;
    struct mb_update_ctx_t update_ctx;
} ThreadArgs;

struct ota_http_ctx_t;

typedef int(*ota_header_handler_cb_t)(struct ota_http_ctx_t *, uint8_t*, uint32_t);
typedef int(*ota_tail_handler_cb_t)(struct ota_http_ctx_t *, uint8_t*, uint32_t, int);

struct ota_http_ctx_t
{
    char    *url;
    struct  db_database_type *pdb;
    
    /* 
        头信息大小 
        如果头信息大小为0，则不进行头信息处理
        如果头信息大小不为0，则需要去掉头信息
        
    */
    uint32_t header_size;
    ota_header_handler_cb_t  header_handler_cb;
    void                    *header_handler_cb_args;
    ota_tail_handler_cb_t   tail_handler_cb;
    void                    *tail_handler_cb_args;

    /* 下载进度 */
    uint32_t total_content_length;  // 总长度
    uint32_t downloaded_bytes;      // 已下载长度
    uint32_t downloaded_percent;    // 下载进度
    uint32_t downloaded_speed;      // 下载速度
    uint32_t downloaded_remaining_time; // 剩余时间
    uint32_t downloaded_remaining_size; // 剩余大小
    char     downloaded_error_msg[32]; // 下载错误信息
};


extern struct ota_http_ctx_t ota_http_ctx;


int ota_http_start(char *url, struct db_database_type *pdb, uint32_t header_size);

#endif /* __OTA_HTTP_H__ */