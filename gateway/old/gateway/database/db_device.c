/*
*  Copyright (c) 2019,广州碳索智控科技有限公司
*  All rights reserved.
* 
*  文件名称：
*  文件标识：
*  描    述：
* 
*  当前版本：V1.0
*  作    者：chenzy
*  运行环境：
*  操作系统: linux
*  硬件平台：
*  固件库  ：
*  修改记录 
*  作者       时间         版本      修改描述
*  chenzy          V1.00      
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

/*
    兼容TAS0网关与  TASO1302网关表的 共享内存设备操作

    TASO GATEWAY，网关模块的共享内存名称为 vmem
    TASO METER  ，网关模块的共享内存名称为 mem_nodetable
*/
#define RT_DATABASE_DEV_MTR_GW      "/dev/mem_nodetable"  
#define RT_DATABASE_DEV_GW          "/dev/vmem" 

#define RT_DATABASE_SIZE    (1024 * 1024)   //  与驱动vmem定义大小一致，修改时务必同时修改drivers/vmem.c



struct rt_database_mgr
{
    int           fd_dev;
    unsigned char *pMem;
};


/*************************************************************************/
/** Function Name:                                     **/
/** Purpose:                                         **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
static struct rt_database_mgr *rt_db_getmgr(void)
{
    static struct rt_database_mgr rt_data_mgr = {
        .fd_dev = -1,
        .pMem = NULL,
    };

    return &rt_data_mgr;
}





/*************************************************************************/
/** Function Name:                                     **/
/** Purpose:                                         **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
void *rt_database_attach(void)
{
    struct rt_database_mgr *pMgr;

    pMgr = rt_db_getmgr();

    pMgr->fd_dev = open(RT_DATABASE_DEV_MTR_GW, O_RDWR|O_NDELAY);
    if(pMgr->fd_dev < 0)
    {
        pMgr->fd_dev = open(RT_DATABASE_DEV_GW, O_RDWR|O_NDELAY);
        if(pMgr->fd_dev < 0)
        {
            return NULL;
        }
    } 

    pMgr->pMem = (uint8_t*)mmap((void*)0, RT_DATABASE_SIZE,  
                        PROT_READ|PROT_WRITE,  
                        MAP_SHARED,  
                        pMgr->fd_dev,  
                        0); 
    
    return pMgr->pMem;  
}

/*************************************************************************/
/** Function Name:                                     **/
/** Purpose:                                         **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
void rt_database_deattach(void)
{
    struct rt_database_mgr *pMgr;

    pMgr = rt_db_getmgr();

    if(pMgr->pMem)
    {
        munmap(pMgr->pMem, RT_DATABASE_SIZE);
        pMgr->pMem = NULL;
    }

    if(pMgr->fd_dev >= 0)
    {
        close(pMgr->fd_dev);
        pMgr->fd_dev = -1;
    }
}


/*************************************************************************/
/** Function Name:                                     **/
/** Purpose:                                         **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
void *rt_db_getVmem(void)
{
    struct rt_database_mgr *pMgr;

    pMgr = rt_db_getmgr();

    return pMgr->pMem;
}
