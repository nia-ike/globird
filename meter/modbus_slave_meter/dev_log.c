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
};


static int fd_dev = 1;



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
    int i;
    uint16_t length;
    char str[128];
    char *p;

    /*
        信息换行处理
        BYTES_NUM_PER_LINE = 32
    */
    for(i=0; i<inlength/32; i++)
    {
        length = snprintf(str, sizeof(str), "%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X "
                                                    "%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\r\n"
                                                    ,in[0],in[1],in[2],in[3],in[4],in[5],in[6],in[7],in[8],in[9],in[10],in[11],in[12],in[13],in[14],in[15],
                                                     in[16],in[17],in[18],in[19],in[20],in[21],in[22],in[23],in[24],in[25],in[26],in[27],in[28],in[29],in[30],in[31]);
        
        log_msg_push(str, length);
        in += 32;
    }

    p = str;
    length = 0;
    for(i=0; i<inlength%32; i++)
    {
        snprintf(p,  sizeof(str) - length, "%02X ",  in[i]);
        in += 1;
        p += 3;
        length += 3;  
    }   
    snprintf(p, sizeof(str) - length, "\r\n");
    length +=2;

    log_msg_push(str, length);

    return 0;
}





/*************************************************************************/
/** Function Name:                                     **/
/** Purpose:                                         **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
void *log_open(void)
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

    plog->enable = 1;
    plog->channlNo = 1;
    plog->devNo = 1;
    plog->readPt = 0;
    plog->writePt = 0;    

    /* init the parameters */
    if(plog->readPt > LOG_BUFF_END ||
        plog->writePt > LOG_BUFF_END)
    {
        plog->readPt = 0;
        plog->writePt = 0;
    }

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
int log_work_set(int enable, int channlNo, int devNo)
{
    struct log_vmem *plog = log_vmem(NULL);

    if(channlNo > 16 || devNo > 16)
        return -1;

    plog->enable = enable;
    plog->channlNo = channlNo;
    plog->devNo = devNo;
    plog->starttime = time(NULL); 

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
int log_write(int channlNo, int devNo, int dir, uint8_t *pbuf, uint16_t length)
{
    char   lable[64]; 
    uint16_t len;
    time_t now;
    struct tm *tm_now;
    struct timeval tv_unow;
    struct log_vmem *plog = log_vmem(NULL);

    if(plog->enable == 0)
        return -1;

    if(plog->channlNo != channlNo)
        return -1;


    if((plog->devNo == devNo) || (plog->devNo == 0))
    {
        time(&now);

        /* debug timeout */
        if(now - plog->starttime > 1800)
        {
            plog->enable = 0;
        }
        tm_now = localtime(&now);
        gettimeofday(&tv_unow, NULL);
        if(dir)
        {
            len = snprintf(lable, sizeof(lable), "%2d:%2d:%2d.%03ld Sample Chnl%d Dev%d Tx:",
                                            tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec, tv_unow.tv_usec/1000,
                                            channlNo, devNo);
        }
        else 
        {

            len = snprintf(lable, sizeof(lable), "%2d:%2d:%2d.%03ld Sample Chnl%d Dev%d Rx:",
                                            tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec, tv_unow.tv_usec/1000,
                                            channlNo, devNo);
        }

        log_msg_push(lable, len);

        log_msg_serialize(pbuf, length);
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
