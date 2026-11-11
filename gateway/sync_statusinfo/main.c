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
static const char *cpin_to_sim_status(int cpin, int sim_detect)
{
    switch (cpin) {
    case 0: return "ready";
    case 1: return "pin_locked";
    case 2: return "absent";
    case 3: return "error";
    case 4: return "unknown";
    default:
        /* AT 无应答时，只用硬件管脚确认未插卡，不猜测 ready */
        if (sim_detect == 0)
            return "absent";
        return NULL;
    }
}

static const char *creg_to_state(int stat, int ok)
{
    if (!ok)
        return NULL;
    switch (stat) {
    case 0: return "not_registered";
    case 1: return "home";
    case 2: return "searching";
    case 3: return "denied";
    case 4: return "unknown";
    case 5: return "roaming";
    default: return "unknown";
    }
}

/* 3GPP TS 27.007 COPS <AcT>。未给出 AcT 返回 NULL，不猜测。 */
static const char *act_to_tech(int act, int known)
{
    if (!known || act < 0)
        return NULL;
    switch (act) {
    case 0: case 1: case 3: case 8:
        return "gsm";
    case 2: case 4: case 5: case 6:
        return "wcdma";
    case 7: case 9: case 10: case 13:
        return "lte";
    default:
        return "unknown";
    }
}

/* CSQ 0–31 → dBm；99 与失败为未知，不填猜测值 */
static int csq_to_dbm(int csq, int *dbm)
{
    if (csq < 0 || csq > 31 || !dbm)
        return -1;
    *dbm = -113 + 2 * csq;
    return 0;
}

/* CESQ rsrp 索引 0–97 → dBm；255 为未知 */
static int cesq_rsrp_to_dbm(int idx, int *dbm)
{
    if (idx < 0 || idx > 97 || !dbm)
        return -1;
    *dbm = -140 + idx;
    return 0;
}

static int lte_module_process(char *devtty, int l)
{
    int fd;
    int ret;
    char iccid[24] = {0};
    char imei[16] = {0};
    char oper[64] = {0};
    int rsrq = 0;
    int rsrp = -1;
    int cpin = -1;
    int creg_stat = -1;
    int creg_ok = 0;
    int act = -1;
    int cops_ok = 0;
    int csq = -1;
    int rssi_dbm = 0;
    int rsrp_dbm = 0;
    struct db_info_status_type *pStatusInfo = &pDB->db_cfg_info.status;
    struct sysinfo_cellular cell;
    double lfval;
    const char *iccid_out = NULL;
    const char *imei_out = NULL;

    fd = open_serialport(devtty);
    if (fd < 0)
        return -1;

    ret = gsm_at_get_imei(fd, imei);
    if (!ret) {
        snprintf((char *)pStatusInfo->ModuleImei, sizeof(pStatusInfo->ModuleImei), "%s", imei);
        DataMgmtFops.writepoint_string(VIRTUAL_CHNL_NO, VIRTUAL_DEV_NO, INFO_193_OFFSET_IMEI, imei, sizeof(imei));
        imei_out = imei;
    }

    ret = gsm_at_get_iccid(fd, iccid);
    if (!ret) {
        snprintf((char *)pStatusInfo->SimIccid, sizeof(pStatusInfo->SimIccid), "%s", iccid);
        DataMgmtFops.writepoint_string(VIRTUAL_CHNL_NO, VIRTUAL_DEV_NO, INFO_193_OFFSET_ICCID, iccid, sizeof(iccid));
        iccid_out = iccid;
    }

    ret = gsm_at_get_cops(fd, oper, sizeof(oper), &act);
    cops_ok = (ret == 0);

    cpin = gsm_at_query_cpin(fd);
    lfval = (cpin == 0 || cpin == 1 || cpin == 4) ? 1 : 0;
    DataMgmtFops.writepoint(VIRTUAL_CHNL_NO, VIRTUAL_DEV_NO, INFO_193_OFFSET_SIM, &lfval);
    pStatusInfo->SimIsInsert = (int16_t)lfval;

    creg_ok = (gsm_at_query_creg(fd, &creg_stat) == 0);
    lfval = (creg_ok && (creg_stat == 1 || creg_stat == 5)) ? 1 : 0;
    DataMgmtFops.writepoint(VIRTUAL_CHNL_NO, VIRTUAL_DEV_NO, INFO_193_OFFSET_CREG, &lfval);
    pStatusInfo->SimIsReg = (int16_t)lfval;

    lfval = gsm_at_get_csq(fd);
    DataMgmtFops.writepoint(VIRTUAL_CHNL_NO, VIRTUAL_DEV_NO, INFO_193_OFFSET_CSQ, &lfval);
    pStatusInfo->SimCsq = (int16_t)lfval;
    csq = (int)lfval;

    ret = gsm_at_get_cesq(fd, &rsrq, &rsrp);
    if (!ret) {
        lfval = rsrp;
        pStatusInfo->SimRsrp = (int16_t)lfval;
        DataMgmtFops.writepoint(VIRTUAL_CHNL_NO, VIRTUAL_DEV_NO, INFO_193_OFFSET_RSRP, &lfval);
    } else {
        rsrp = -1;
    }

    close_serialport(fd);

    memset(&cell, 0, sizeof(cell));
    cell.sim_status = cpin_to_sim_status(cpin, pStatusInfo->SimDetect);
    cell.registration_state = creg_to_state(creg_stat, creg_ok);
    cell.carrier = (cops_ok && oper[0]) ? oper : NULL;
    cell.access_technology = act_to_tech(act, cops_ok);
    cell.iccid = iccid_out;
    cell.imei = imei_out;
    cell.has_rssi = (csq_to_dbm(csq, &rssi_dbm) == 0);
    cell.rssi_dbm = rssi_dbm;
    cell.has_rsrp = (cesq_rsrp_to_dbm(rsrp, &rsrp_dbm) == 0);
    cell.rsrp_dbm = rsrp_dbm;
    (void)sysinfo_cellular_write(&cell);

    if (debug) {
        printf("sim_status:%s\n", cell.sim_status ? cell.sim_status : "null");
        printf("registration:%s\n", cell.registration_state ? cell.registration_state : "null");
        printf("carrier:%s act:%s\n",
               cell.carrier ? cell.carrier : "null",
               cell.access_technology ? cell.access_technology : "null");
        printf("rssi_dbm:%s rsrp_dbm:%s\n",
               cell.has_rssi ? "set" : "null",
               cell.has_rsrp ? "set" : "null");
    }

    if (l)
        led_signal(cell.has_rsrp ? cell.rsrp_dbm : rsrp);

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
#define RTC_VOLT_INV_SEC    3600 /* RTC 电池电压：1小时更新一次 */
    int tick = 0;
    double lfval;
    int64_t i64val;
    uint16_t rtc_volt = 0;
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

        /* RTC 备份电池电压：I2C 读 SD3078，变化很慢，周期宜明显长于模组/网络 */
        if (tick % RTC_VOLT_INV_SEC == 0)
        {
            if (sysinfo_rtc_volt_get(&rtc_volt) == 0)
            {
                pStatusInfo->rtc_volt = (int16_t)rtc_volt;
                if (debug)
                    printf("RTC volt: %d (0.01V), ~%.2fV\n",
                           rtc_volt, rtc_volt / 100.0);
            }
            else if (debug)
            {
                printf("RTC volt: read failed\n");
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
