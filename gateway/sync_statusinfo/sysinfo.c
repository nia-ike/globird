#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include <sys/vfs.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "sysinfo.h"
#include "cJSON.h"

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

static int json_is_true(const cJSON *item)
{
    return (item && cJSON_IsBool(item) && cJSON_IsTrue(item)) ? 1 : 0;
}

/*************************************************************************/
/** Function Name:  sysinfo_wifi_ready                                 **/
/** Purpose:        从 netmgrd_v4 status.json 读取 WiFi 就绪状态         **/
/** Return:         0=未就绪；1=已连接且已获取 IPv4                        **/
/*************************************************************************/
uint16_t sysinfo_wifi_ready(void)
{
    FILE *fp = NULL;
    long fsize;
    char *buf = NULL;
    size_t nread;
    cJSON *root = NULL;
    cJSON *ifaces = NULL;
    cJSON *wlan = NULL;
    uint16_t ready = 0;

    fp = fopen(NM4_STATUS_JSON, "r");
    if (!fp)
        return 0;

    if (fseek(fp, 0, SEEK_END) != 0)
        goto out;

    fsize = ftell(fp);
    if (fsize <= 0 || fsize > 64 * 1024)
        goto out;

    rewind(fp);
    buf = (char *)malloc((size_t)fsize + 1);
    if (!buf)
        goto out;

    nread = fread(buf, 1, (size_t)fsize, fp);
    if (nread != (size_t)fsize)
        goto out;

    buf[fsize] = '\0';

    root = cJSON_Parse(buf);
    if (!root)
        goto out;

    ifaces = cJSON_GetObjectItem(root, "interfaces");
    wlan = ifaces ? cJSON_GetObjectItem(ifaces, "wlan0") : NULL;
    if (wlan && json_is_true(cJSON_GetObjectItem(wlan, "exists")) &&
        json_is_true(cJSON_GetObjectItem(wlan, "link")) &&
        json_is_true(cJSON_GetObjectItem(wlan, "ipv4")))
    {
        ready = 1;
    }

out:
    if (root)
        cJSON_Delete(root);
    free(buf);
    if (fp)
        fclose(fp);

    return ready;
}

/*************************************************************************/
/** Function Name:  json_escape_str                                    **/
/** Purpose:        将字符串转义写入 out（不含两侧引号）                 **/
/*************************************************************************/
static void json_escape_str(char *out, size_t outlen, const char *in)
{
    size_t oi = 0;
    const unsigned char *p;

    if (!out || outlen == 0)
        return;
    out[0] = '\0';
    if (!in)
        return;

    for (p = (const unsigned char *)in; *p && oi + 2 < outlen; ++p) {
        if (*p == '"' || *p == '\\') {
            if (oi + 3 >= outlen)
                break;
            out[oi++] = '\\';
            out[oi++] = (char)*p;
        } else if (*p >= 0x20 && *p != 0x7f) {
            out[oi++] = (char)*p;
        }
    }
    out[oi] = '\0';
}

/*************************************************************************/
/** Function Name:  sysinfo_cellular_write                             **/
/** Purpose:        原子写入 /var/run/netmgr/cellular.json             **/
/** Return:         0 成功；-1 失败                                    **/
/*************************************************************************/
int sysinfo_cellular_write(const char *imei, const char *iccid,
                           const char *oper, int sim, int creg,
                           int csq, int rsrp)
{
    char tmp[128];
    char e_imei[48];
    char e_iccid[64];
    char e_oper[96];
    char ts[32];
    FILE *fp;
    time_t now;
    struct tm tmv;

    (void)mkdir(NM4_STATUS_DIR, 0755);

    now = time(NULL);
    localtime_r(&now, &tmv);
    (void)strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", &tmv);

    json_escape_str(e_imei, sizeof(e_imei), imei ? imei : "");
    json_escape_str(e_iccid, sizeof(e_iccid), iccid ? iccid : "");
    json_escape_str(e_oper, sizeof(e_oper), oper ? oper : "");

    (void)snprintf(tmp, sizeof(tmp), "%s.tmp", NM4_CELLULAR_JSON);
    fp = fopen(tmp, "w");
    if (!fp)
        return -1;

    (void)fprintf(fp,
                  "{\n"
                  "  \"time\": \"%s\",\n"
                  "  \"imei\": \"%s\",\n"
                  "  \"iccid\": \"%s\",\n"
                  "  \"operator\": \"%s\",\n"
                  "  \"sim\": %d,\n"
                  "  \"creg\": %d,\n"
                  "  \"csq\": %d,\n"
                  "  \"rsrp\": %d\n"
                  "}\n",
                  ts, e_imei, e_iccid, e_oper, sim, creg, csq, rsrp);

    if (fclose(fp) != 0)
        return -1;
    if (rename(tmp, NM4_CELLULAR_JSON) != 0)
        return -1;
    return 0;
}