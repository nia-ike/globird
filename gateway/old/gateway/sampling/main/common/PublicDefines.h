/*************************************************
  Copyright (C), 2003-2014, pilot Co., Ltd.
  File name:      PublicDefines.h
  Author:       wangcq    Version: 1.0       Date: 2016-02-19
  Description:    公共头文件类
  Others:
  History:         // 历史修改记录
      wangcq    2016/02/19     1.0     build this moudle
*************************************************/

#ifndef __PUBLIC_DEFINES_H__
#define __PUBLIC_DEFINES_H__

#include <stdint.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>

#define  CLOUD_COMMSTATUS  170

/* 标志内置通道和内置设备 */
#define INTERNAL_CHNL 0xC3
#define INTERNAL_DEV  0x01

enum {
    TYPE_UINT8_T = 0,
    TYPE_SINT8_T,
    TYPE_UINT16_T,
    TYPE_SINT16_T,
    TYPE_UINT32_T,
    TYPE_SINT32_T,
    TYPE_UINT64_T,
    TYPE_SINT64_T,

    TYPE_MAX_T=255
};

typedef struct
{
    int                       seqStatus;   //返回结果，1表示没有批准，0表示已经批准
    int                       status;      //接口调用结果代码
    char                      message[128];//接口代码返回描述
}ST_AUTH_ACK;

typedef struct
{
    char                      realIp[64];  //真实上传数据的IP
    char                      realport;    //真实的port
    int                       status;      //接口调用结果代码
    char                      message[128];//接口代码返回描述
}ST_SKIP_ADDRESS_ACK;

typedef struct
{
    char                      data[64];  //真实上传数据的IP
    char                      message[128];//接口代码返回描述
    int                       status;      //接口调用结果代码
}ST_UPLOAD_FILE_ACK;


//清空目录
void delete_path(const char* path);

//创建目录
int8_t mkdirs(const char *pcPath);

//得到文件大小
unsigned long get_file_size(const char *path);

//执行命令
int32_t executeCMD(const char *cmd, char *result, int len);


#endif
