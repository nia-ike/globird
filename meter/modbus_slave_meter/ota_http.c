
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
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <sys/sysinfo.h>
#include <curl/curl.h>  // 用于访问固件下载 URL
#include <pthread.h>


#include "ota_http.h"

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
// 回调函数，用于将下载的数据写入文件
size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) 
{
    return fwrite(ptr, size, nmemb, stream);    
}

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
// 修改后的下载函数，传递 pctx 作为进度回调的参数
int download_firmware_progress(const char *url, const char *filename, const char *username, const char *password)
{
    CURL *curl;
    FILE *fp;
    CURLcode res;

    // 确保正确初始化 libcurl 
    curl_global_init(CURL_GLOBAL_DEFAULT);

    curl = curl_easy_init();
    if (curl) 
    {
        printf("download_firmware_with_progress:%s filename:%s\n", url,filename);
        fp = fopen(filename, "wb");
        if (fp) 
        {
            // 设置 url
            curl_easy_setopt(curl, CURLOPT_URL, url);
            // 设置写入回调函数
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
            // 设置写入文件指针
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
            // 临时禁用服务器证书验证
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            // 临时禁用服务器主机名验证
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L); 
            // 开启调试模式
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

            // 设置账号和密码
            if (username && password) {
                char userpwd[256];
                snprintf(userpwd, sizeof(userpwd), "%s:%s", username, password);
                curl_easy_setopt(curl, CURLOPT_USERPWD, userpwd);
            }

            res = curl_easy_perform(curl);
            printf("res:%d\n", res);
            if (res != CURLE_OK) 
            {
                fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
                fclose(fp);
                curl_easy_cleanup(curl);
                return -1;
            }
            fclose(fp);
        } 
        else 
        {
            fprintf(stderr, "Failed to open file %s\n", filename);
            curl_easy_cleanup(curl);
            return -1;
        }
        curl_easy_cleanup(curl);
        return 0;
    }
    return -1;
}

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
// 线程函数，处理下载和升级逻辑
void *upgrade_firmware_thread(void *arg) 
{
    ThreadArgs *args = (ThreadArgs *)arg;
    struct db_database_type *pdb = args->pdb;
    const char *url = args->url;
    const char *filename = args->filename;
    struct mb_update_ctx_t update_ctx = args->update_ctx;
    int ret;

    const char *username = "";
    const char *password = "";
    
    // 下载固件
    if (download_firmware_progress(url, filename, username, password) == 0)  
    {
        // 固件升级
        mb_file_pkg_upgrade(&update_ctx);             
    } 
    else 
    {
        // 发送下载失败状态
        printf("Failed to download firmware from %s\n", url);
    }

    free(args);
    pthread_exit(NULL);
}



/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:             
** output parameters:   NONE
** Returned value:      
*********************************************************************************************************/
int ota_http_start(char *url, struct db_database_type *pdb, uint32_t header_size)
{

    // 创建线程参数
    ThreadArgs *args = (ThreadArgs *)malloc(sizeof(ThreadArgs));
    if (!args) {
        fprintf(stderr, "Memory allocation failed for ThreadArgs\n");
        free((char *)url); // 释放之前分配的 url 内存
        return -1;
    }
    args->pdb = pdb;
    args->url = url;
    args->filename = MB_UPDATE_FILE;

    // 创建线程
    pthread_t thread;
    if (pthread_create(&thread, NULL, upgrade_firmware_thread, (void *)args) != 0)
    {
        fprintf(stderr, "Failed to create thread\n");
        free((char *)args->url); // 释放 url 内存
        if (args->filename) {
            free((char *)args->filename); // 释放 filename 内存
        }
        free(args);
    } 
    else 
    {
        pthread_detach(thread); // 分离线程，让其自动释放资源
    }

    return 0;
}