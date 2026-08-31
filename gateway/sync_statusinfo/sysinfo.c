#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include <sys/vfs.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "sysinfo.h"

#define DEV_SIM_DETECT      "/dev/input/event1"
#define KEY_SIM_DETECT      12   /* gpio-keys DTS 中 sim_detect 的 linux,code */


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
uint16_t sysinfo_ram_free()
{
    FILE *fd;
    char buff[256];
    int total, free, buffers, cached;
    uint16_t ret;

    fd = fopen ("/proc/meminfo", "r");

    while(!feof(fd))
    {
        fgets (buff, sizeof(buff), fd);
        if(!strncasecmp(buff, "MemTotal:", strlen("MemTotal:")))
        {
            sscanf (buff, "%*s %d", &total);
        }
        else if(!strncasecmp(buff, "MemFree:", strlen("MemFree:")))
        {
            sscanf (buff, "%*s %d", &free);
        }
        else if(!strncasecmp(buff, "Buffers:", strlen("Buffers:")))
        {
            sscanf (buff, "%*s %d", &buffers);
        }
        else if(!strncasecmp(buff, "Cached:", strlen("Cached:")))
        {
            sscanf (buff, "%*s %d", &cached);
            break;
        }
    }
    fclose(fd);     //关闭文件fd
    ret = free+buffers+cached;
    return ret;
}

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
uint16_t sysinfo_disk_free()
{
    uint16_t ret = 0;
    struct statfs statFS;
    uint32_t   freeBytes = 0;

    if (statfs("/", &statFS) == -1){
        return(-1);
    }
    freeBytes = statFS.f_bfree * statFS.f_bsize;

    ret = (freeBytes*1.000)/1024.000;

    return ret;
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
uint32_t sysinfo_eth_ip(void)
{
    FILE *fd;
    char buff[32];

    fd = fopen ("/var/run/network/ip", "r");
    if(fd == NULL)
        return -1;

    fgets(buff, sizeof(buff), fd);
    fclose(fd);
    return inet_addr(buff);
}

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
uint32_t sysinfo_eth_netmask(void)
{
    FILE *fd;
    char buff[32];

    fd = fopen ("/var/run/network/netmask", "r");
    if(fd == NULL)
        return -1;

    fgets(buff, sizeof(buff), fd);
    fclose(fd);
    return inet_addr(buff);
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
uint32_t sysinfo_eth_gateway(void)
{
    FILE *fd;
    char buff[32];

    fd = fopen ("/var/run/network/gateway", "r");
    if(fd == NULL)
        return -1;

    fgets(buff, sizeof(buff), fd);
    fclose(fd);
    return inet_addr(buff);
}

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
uint64_t sysinfo_eth_mac(void)
{
    FILE *fd;
    char    cmac[32];
    int bmac[6];
    uint64_t u64mac; 

    fd = fopen ("/var/run/network/mac", "r");
    if(fd == NULL)
        return -1;

    fgets(cmac, 32, fd);
    fclose(fd);
    sscanf(cmac, "%X:%X:%X:%X:%X:%X", &bmac[0], &bmac[1], &bmac[2], &bmac[3], &bmac[4], &bmac[5]);

    u64mac = ((uint64_t)bmac[0] << 40) | ((uint64_t)bmac[1] << 32) | ((uint64_t)bmac[2] << 24) | 
                ((uint64_t)bmac[3] << 16) | ((uint64_t)bmac[4] << 8) | ((uint64_t)bmac[5] << 0) ;
    return u64mac;
}

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
uint64_t sysinfo_sn(void)
{
    FILE *fd;
    char    sn[32];
    uint64_t u32sn; 

    fd = fopen ("/userdata/system/sn", "r");
    if(fd == NULL)
        return -1;

    fgets(sn, 32, fd);
    fclose(fd);
    sscanf(sn, "TSG%lld", &u32sn);
    
    return u32sn;
}

/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            -1:检测失败；0:未插卡；1:已插卡                        **/
/** Notice:            与 SimIsInsert 不同，本值来自硬件 IO 管脚。           **/
/*************************************************************************/
int16_t sysinfo_sim_detect(void)
{
    int fd;
    unsigned char key_map[(KEY_MAX + 7) / 8];
    int state = -1;

    fd = open(DEV_SIM_DETECT, O_RDONLY | O_NONBLOCK);
    if (fd < 0)
        return -1;

    memset(key_map, 0, sizeof(key_map));
    if (ioctl(fd, EVIOCGKEY(sizeof(key_map)), key_map) >= 0) {
        /* gpio-keys 电平与插卡状态相反，取反后：0=未插卡，1=已插卡 */
        state = !(key_map[KEY_SIM_DETECT / 8] & (1 << (KEY_SIM_DETECT % 8)));
    }

    close(fd);
    return (int16_t)state;
}