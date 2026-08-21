/*
*  Copyright (c) 2019,珠海科技股份有限公司
*  All rights reserved.
*
*  文件名称：log.c
*  文件标识：
*  描    述：
*
*  当前版本：V1.0
*  作    者：chenzy
*  运行环境：
*  操作系统: linux
*  硬件平台：f1c100s
*  固件库  ：
*  修改记录
*  作者       时间         版本      修改描述
*  chenzy   2021       V1.00
*/

#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <linux/vt.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include "dev_log.h"

extern char *__progname;

#define LOG_DEV         "/dev/debugmsg"
#define LOG_VMEM_SIZE   (8 * 1024)   //  与驱动log定义大小一致，修改时务必同时修改drivers/log.c
#define LOG_BUFF_SIZE   (7*1024 + 512)

#define LOG_BUFF_START  0
#define LOG_BUFF_END    LOG_BUFF_SIZE
/*
    /dev/debugmsg  => vmem  buffer 

    |___________________________________________________________|_ _ _ _ _ _ _ _|
    
    |<-                         total   8KB                                   ->|
    |<-                      7KB + 512 Bytes                  ->|<- 512 Bytes ->|
    |<-                         log                             |    index      | 


*/
struct log_vmem
{
    /* buffer */
    uint8_t     pbuff[LOG_BUFF_SIZE];

    /* index */
    uint8_t     reserved[16];
    uint8_t     enable;//0 关闭调试功能 1 开启
    uint8_t     channlNo;  //1 采集调试 2 LORA调试 9 转发调试
    uint8_t     devNo;      //设备地址
    uint16_t    readPt;
    uint16_t    writePt;
    uint32_t    starttime;
    char        app_name[32];//应用名称
    uint16_t    log_buff_size;
};


static int fd_dev = 1;
static log_abort_cb_t log_abort_cb = NULL;


/*************************************************************************/
/** Function Name:                                     **/
/** Purpose:                                         **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
static struct log_vmem *log_vmem(void *vmem)
{
    static struct log_vmem *plog = NULL;

    if(vmem != NULL)
        plog = vmem;

    return plog;
}





/*************************************************************************/
/** Function Name:                                     **/
/** Purpose:                                         **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
static int log_msg_push(char *pbuf, uint16_t length)
{
    int i;
    struct log_vmem *plog = log_vmem(NULL);

    if(plog == NULL)
        return 0;

    for(i=0; i<length; i++)
    {
        plog->pbuff[plog->writePt] = pbuf[i];

        plog->writePt++;
        if(plog->writePt >= LOG_BUFF_END)
            plog->writePt = LOG_BUFF_START;
        
        if(plog->writePt == plog->readPt)
            plog->readPt++;
        
        if(plog->readPt >= LOG_BUFF_END)
            plog->readPt = LOG_BUFF_START;
    }

    return length;
}


/*************************************************************************/
/** Function Name:                                     **/
/** Purpose:                                         **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
static int log_msg_serialize(uint8_t *in, uint16_t inlength)
{
    if (in == NULL || inlength == 0)
        return -1;

    // 每个字节3字符（2位hex+空格），加结尾\r\n
    uint16_t maxlen = inlength * 3 + 2;
    char *str = (char *)malloc(maxlen);
    if (!str)
        return -1;

    char *p = str;
    uint16_t length = 0;
    for (int i = 0; i < inlength; i++) {
        int n = snprintf(p, maxlen - length, "%02X ", in[i]);
        if (n < 0 || n >= (maxlen - length)) {
            break;
        }
        p += n;
        length += n;
    }
    // 去掉最后一个多余的空格
    if (length > 0 && str[length - 1] == ' ') {
        str[length - 1] = '\0';
        length--;
    }
    snprintf(str + length, maxlen - length, "\r\n");
    length += 2;

    log_msg_push(str, length);
    free(str);
    return 0;
}




/*************************************************************************/
/** Function Name:                                     **/
/** Purpose:                                         **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
void *log_open(log_abort_cb_t abort_cb)
{
    struct log_vmem *plog = NULL;
    /* 
        attach the vmem
     */
    fd_dev = open(LOG_DEV, O_RDWR|O_NDELAY);
    if(fd_dev < 0)
    {
        return NULL;
    }

    plog = (struct log_vmem *)mmap((void*)0, LOG_VMEM_SIZE,
                                    PROT_READ|PROT_WRITE,
                                    MAP_SHARED,
                                    fd_dev,
                                    0);
    if(plog == NULL)
        return NULL;

//    plog->enable = 0;
//    plog->channlNo = 0;
//    plog->devNo = 0;
    plog->readPt = 0;
    plog->writePt = 0;    
    plog->log_buff_size = LOG_BUFF_SIZE;

    /* init the parameters */
    if(plog->readPt > LOG_BUFF_END ||
        plog->writePt > LOG_BUFF_END)
    {
        plog->readPt = 0;
        plog->writePt = 0;
    }

    log_abort_cb = abort_cb;
    /* install vmem(plog) */
    log_vmem(plog);

    return plog;                        
}

/*************************************************************************/
/** Function Name:                                     **/
/** Purpose:                                         **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
void log_close(void)
{
    struct log_vmem *plog = log_vmem(NULL);

    if(plog == NULL)
        return;

    if(plog)
    {
        munmap(plog, LOG_VMEM_SIZE);
        plog = NULL;
    }

    if(fd_dev >= 0)
    {
        close(fd_dev);
        fd_dev = -1;
    }
}


/*************************************************************************/
/** Function Name:                                     **/
/** Purpose:                                         **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
int log_get_unread_length(void)
{
    struct log_vmem *plog = log_vmem(NULL);

    if(plog == NULL)
        return 0;

    /* 
            readPt = writept, No data
     */
    if(plog->readPt == plog->writePt)
        return 0;
    /*
            read > write,   2 seg

            |-----|+++++++++++++|------|
                 write         read
            
            ----  unread
            ++++  already read
    */
    else if(plog->readPt > plog->writePt)
        return LOG_BUFF_END - (plog->readPt - plog->writePt);
    
    /*
            read > write,   2 seg

            |+++++++++++++|----------|
                          read   write        
            
            ----  unread
            ++++  already read
    */
    else 
        return plog->writePt - plog->readPt;
}

/*************************************************************************/
/** Function Name:                                     **/
/** Purpose:                                         **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
int log_work_set(int enable, int channlNo, int devNo, char *app_name)
{
    struct log_vmem *plog = log_vmem(NULL);


    plog->enable = enable;
    plog->channlNo = channlNo;
    plog->devNo = devNo;
    plog->starttime = time(NULL); 
    strncpy(plog->app_name, app_name, sizeof(plog->app_name));
    if(enable == 0 || enable == 2)
    {
        plog->readPt = 0;
        plog->writePt = 0;
    }

    return 0;
}

/*************************************************************************/
/** Function Name:                                     **/
/** Purpose:                                         **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
int log_buff_size(void)
{
    return LOG_BUFF_SIZE;
}

/*************************************************************************/
/** Function Name:                                     **/
/** Purpose:                                         **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
int log_write(int channlNo, int devNo, int dir, uint8_t *pbuf, uint16_t length)
{
    int ret;
    char   lable[64]; 
    uint16_t len;
    time_t now;
    struct tm *tm_now;
    struct timeval tv_unow;
    struct log_vmem *plog = log_vmem(NULL);

    if(plog->enable == 0)
        return -1;

    if((plog->channlNo != channlNo))
        return -1;

    if(log_abort_cb != NULL)
    {
        ret = log_abort_cb(pbuf, length);
        if(ret == true)
            return -1;
    }
        
    if(!((plog->devNo == devNo) || (plog->devNo == 0)))
        return -1;

    time(&now);

    /* debug timeout */
    if(now - plog->starttime > 1800)
    {
        plog->enable = 0;
    }
    tm_now = localtime(&now);
    gettimeofday(&tv_unow, NULL);

    // 遇到转发通道时，打印还原会通道号
    if(channlNo >= 100)
        channlNo = channlNo - 100;

    if(dir)
    {
        len = snprintf(lable, sizeof(lable), "%02d:%02d:%02d.%03ld [%s] Chnl%d Dev%d Tx:",
                                        tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec, tv_unow.tv_usec/1000,
                                        __progname, channlNo, devNo);
    }
    else 
    {

        len = snprintf(lable, sizeof(lable), "%02d:%02d:%02d.%03ld [%s] Chnl%d Dev%d Rx:",
                                        tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec, tv_unow.tv_usec/1000,
                                        __progname, channlNo, devNo);
    }

    log_msg_push(lable, len);

    log_msg_serialize(pbuf, length);
    
    
    return 0;
}

/*
 * Usage examples for log_write_string with variable arguments:
 * 
 * // Simple string logging
 * log_write_string(channlNo, devNo, LOG_TX, "Connection established");
 * 
 * // With format specifiers
 * log_write_string(channlNo, devNo, LOG_RX, "Received data: %s, length: %d", data, len);
 * 
 * // With multiple variables
 * log_write_string(channlNo, devNo, LOG_TX, "Device %d status: %s, value: %f", devId, status, value);
 * 
 * // With hex dump
 * log_write_string(channlNo, devNo, LOG_RX, "Data: %02X %02X %02X %02X", data[0], data[1], data[2], data[3]);
 */


/*************************************************************************/
/** Function Name:                                     **/
/** Purpose:                                         **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
int log_write_string(int channlNo, int devNo, char *level, const char *format, ...)
{
    int ret;
    char   lable[64]; 
    char   formatted_buf[LOG_BUFF_SIZE];
    uint16_t len;
    time_t now;
    struct tm *tm_now;
    struct timeval tv_unow;
    struct log_vmem *plog = log_vmem(NULL);
    va_list args;

    if(plog->enable == 0)
        return -1;

    if((plog->channlNo != 0))
        return -1;

    // app 插件的debug， 不采用通道号，直接使用app_name匹配
    if(strcmp(__progname, plog->app_name) != 0)
        return -1;
    
    // Format the message using variable arguments
    va_start(args, format);
    len = vsnprintf(formatted_buf, sizeof(formatted_buf), format, args);
    va_end(args);

    if(log_abort_cb != NULL)
    {
        ret = log_abort_cb(formatted_buf, len);
        if(ret == true)
            return -1;
    }

    time(&now);

    /* debug timeout */
    if(now - plog->starttime > 1800)
    {
        plog->enable = 0;
    }
    tm_now = localtime(&now);
    gettimeofday(&tv_unow, NULL);

    len = snprintf(lable, sizeof(lable), "%02d:%02d:%02d.%03ld [%s] Chnl%d %s:",
                                        tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec, tv_unow.tv_usec/1000,
                                        __progname, channlNo, level);


    log_msg_push(lable, len);

    log_msg_push(formatted_buf, strlen(formatted_buf));
   
    
    return 0;
}


/*************************************************************************/
/** Function Name:                                     **/
/** Purpose:                                         **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
int log_read(uint8_t *pbuf, uint16_t length)
{
    int i;
    int len;
    struct log_vmem *plog = log_vmem(NULL);

    len = log_get_unread_length();
    len = len < length ? len : length;

    for(i=0; i<len; i++)
    {
        pbuf[i] = plog->pbuff[plog->readPt]; 
        
        plog->readPt++;

        if(plog->readPt >= LOG_BUFF_END)
            plog->readPt = LOG_BUFF_START;
    }

    return len;
}
