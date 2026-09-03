#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "gsm_serial.h"
#include "gsm_at.h"
#include "db_struct.h"
#include "db_device.h"
#include "db_meter_device.h"
#include "sysinfo.h"

#include "db_node_fops.h"

#define VIRTUAL_CHNL_NO    193
#define VIRTUAL_DEV_NO     1
#define QUERY_INV  15
#define GSM_USB_PORT	                "/dev/ttyUSB3"


/*ID offset 请查看 work_condition.json*/
enum 
{
    INFO_193_OFFSET_SN = 0,
    INFO_193_OFFSET_IP = 1,
    INFO_193_OFFSET_NETMASK = 2,
    INFO_193_OFFSET_GATEWAY = 3,
    INFO_193_OFFSET_MAC = 4,
    INFO_193_OFFSET_IMEI = 5,   // 占用2个units
    INFO_193_OFFSET_ICCID = 7,
    INFO_193_OFFSET_CPU = 10,
    INFO_193_OFFSET_MOMERY = 11,
    INFO_193_OFFSET_UPTIME = 12,
    INFO_193_OFFSET_CSQ = 13,
    INFO_193_OFFSET_RSRP = 14,
    INFO_193_OFFSET_SIM = 15,
    INFO_193_OFFSET_CREG = 16,
    INFO_193_OFFSET_DISK = 17,
};

static struct db_database_type *pDB = NULL; 
static int debug = 0;

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
void led_brightness(char *dev, int enable)
{
    int fd = open(dev, O_WRONLY);
    if(fd < 0)
        return;

    if(enable)
        write(fd, "1\n", 2);
    else 
        write(fd, "0\n", 2);

    close(fd);
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
void led_signal(int rsrp)
{

}





/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int lte_module_process(char *devtty, int l)
{
    int fd;
    int ret;
    char iccid[24] = {0};
    char imei[16] = {0};
    int rsrq = 0;
    int rsrp = 0;
    struct db_info_status_type *pStatusInfo = &pDB->db_cfg_info.status; 
    
    double lfval;

    fd = open_serialport(devtty);
    if(fd < 0)
        return -1;

    // sim imei
    ret = gsm_at_get_imei(fd, imei);
    if(!ret)
    {
        snprintf((char *)pStatusInfo->ModuleImei, sizeof(pStatusInfo->ModuleImei), "%s", imei);
         DataMgmtFops.writepoint_string(VIRTUAL_CHNL_NO, VIRTUAL_DEV_NO, INFO_193_OFFSET_IMEI, imei, sizeof(imei));
    }

    // sim imei
    ret = gsm_at_get_iccid(fd, iccid);
    if(!ret)
    {
        snprintf((char *)pStatusInfo->SimIccid, sizeof(pStatusInfo->SimIccid), "%s", iccid);
         DataMgmtFops.writepoint_string(VIRTUAL_CHNL_NO, VIRTUAL_DEV_NO, INFO_193_OFFSET_ICCID, iccid, sizeof(iccid));
    }
       
    // sim status (AT 命令，模组级)
    lfval = gsm_at_get_sw_sim_insert(fd);
    DataMgmtFops.writepoint(VIRTUAL_CHNL_NO, VIRTUAL_DEV_NO, INFO_193_OFFSET_SIM, &lfval);
    pStatusInfo->SimIsInsert = lfval;

    // sim creg
    lfval = gsm_at_get_creg(fd);
    DataMgmtFops.writepoint(VIRTUAL_CHNL_NO, VIRTUAL_DEV_NO, INFO_193_OFFSET_CREG, &lfval);
    pStatusInfo->SimIsReg = lfval;

    // sim csq
    lfval = gsm_at_get_csq(fd);
    DataMgmtFops.writepoint(VIRTUAL_CHNL_NO, VIRTUAL_DEV_NO, INFO_193_OFFSET_CSQ, &lfval);
    pStatusInfo->SimCsq = lfval;

    // sim rsrp
    ret = gsm_at_get_cesq(fd, &rsrq, &rsrp);
    lfval = rsrp;
    if(ret)
    {
        pStatusInfo->SimRsrp = lfval;
        DataMgmtFops.writepoint(VIRTUAL_CHNL_NO, VIRTUAL_DEV_NO, INFO_193_OFFSET_RSRP, &lfval);
    }
        

    close_serialport(fd);

    if(debug)
    {
        printf("iccid:%s\n", pStatusInfo->SimIccid);
        printf("imei:%s\n", pStatusInfo->ModuleImei);
        printf("csq:%d\n", pStatusInfo->SimCsq);
    }

    if(l)
    {
        led_signal(rsrp);
    }

    return 0;
}




/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int loopit(char *devtty, int l)
{
#define SIM_DETECT_INV_SEC  1
#define SYSINFO_INV_SEC     5   /* LTE 模组信息读取周期 */
#define NETINFO_INV_SEC     15  /* 网络/系统信息读取周期（原 cnt>2 每 3 轮 × 5s） */
    int tick = 0;
    double lfval;
    int64_t i64val;
    struct db_info_status_type *pStatusInfo = &pDB->db_cfg_info.status; 

    while(1)
    {
        pStatusInfo->SimDetect = sysinfo_sim_detect();
        if(debug)
        {
            printf("SimDetect:%d\n", pStatusInfo->SimDetect);
        }

        if (tick % SYSINFO_INV_SEC == 0)
            lte_module_process(devtty, l);

        if (tick % NETINFO_INV_SEC == 0)
        {
            // system ram 
            lfval = sysinfo_ram_free();
            DataMgmtFops.writepoint(VIRTUAL_CHNL_NO, VIRTUAL_DEV_NO, INFO_193_OFFSET_MOMERY, &lfval);
            pStatusInfo->MemFree = lfval;
            // system disk 
            lfval = sysinfo_disk_free();
            DataMgmtFops.writepoint(VIRTUAL_CHNL_NO, VIRTUAL_DEV_NO, INFO_193_OFFSET_DISK, &lfval);
            pStatusInfo->DiskFree = lfval;

            pStatusInfo->wifi_ready = sysinfo_wifi_ready();

            // IP
            i64val = sysinfo_eth_ip();
            DataMgmtFops.writepoint_int64(VIRTUAL_CHNL_NO, VIRTUAL_DEV_NO, INFO_193_OFFSET_IP, &i64val);

            // netmask
            i64val = sysinfo_eth_netmask();
            DataMgmtFops.writepoint_int64(VIRTUAL_CHNL_NO, VIRTUAL_DEV_NO, INFO_193_OFFSET_NETMASK, &i64val);

            // gateway
            i64val = sysinfo_eth_gateway();
            DataMgmtFops.writepoint_int64(VIRTUAL_CHNL_NO, VIRTUAL_DEV_NO, INFO_193_OFFSET_GATEWAY, &i64val);

            // gateway
            i64val = sysinfo_eth_mac();
            DataMgmtFops.writepoint_int64(VIRTUAL_CHNL_NO, VIRTUAL_DEV_NO, INFO_193_OFFSET_MAC, &i64val);

            // sn
            i64val = sysinfo_sn();
            DataMgmtFops.writepoint_int64(VIRTUAL_CHNL_NO, VIRTUAL_DEV_NO, INFO_193_OFFSET_SN, &i64val);

            if(debug)
            {
                printf("Ram free:%d\n", pStatusInfo->MemFree);
                printf("Disk free:%d\n", pStatusInfo->DiskFree);
                printf("WiFi ready:%u\n", pStatusInfo->wifi_ready);
                printf("IP:%X\n", sysinfo_eth_ip());
                printf("Netmask:%X\n", sysinfo_eth_netmask());
                printf("Gateway:%X\n", sysinfo_eth_gateway());
            }
        }

        tick++;
        sleep(SIM_DETECT_INV_SEC);
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
static void usage(char *_name)
{
	fprintf(stderr,"\nUsage: %s [options] \n",_name);
    fprintf(stderr,"  -l 1                : led signal acitve \n");
    fprintf(stderr,"  -s /dev/ttyUSB3     : choose the active serial port \n");
	fprintf(stderr,"  -h                  : Show this help message\n");
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int main(int argc, char *argv[])
{
    int	    leds = 0;
    int     opt;
    void    *pVmem = NULL;
    
    char    devtty[32] = "/dev/ttyUSB3";

    if(argc >= 2)
    {
        while((opt=getopt(argc, argv, "hs:d:l:")) != -1)
        {
            switch(opt)
            {
                case 's':
                    snprintf(devtty, sizeof(devtty), "%s", optarg);
                break;
                case 'l':
                    leds = atoi(optarg);
                break;
                case 'd':
                    debug = 1;
                break;
                case 'h':
                    usage(argv[0]);
                    exit(0);
                break;
            }
        }
    }
    /* 网关部分共享内存 */
    pVmem = rt_database_attach();
    if(pVmem == NULL)
    {
        printf("can not attach shared memory /dev/node_table\n");
        return -1;
    }

    /* 电表部分共享内存 */
    pDB = rt_meter_database_attach();
    if(pDB == NULL)
    {
        printf("can not attach shared memory /dev/vmem\n");
        return -1;
    }
        

    /* database fops init */
    DataMgmtFops.init(pVmem);

    loopit(devtty, leds);

    rt_meter_database_deattach();

    rt_database_deattach();

    exit(0);
}

/*****************************************************************************/
