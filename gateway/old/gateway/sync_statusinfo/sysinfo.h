#ifndef SYS_INFO_DEF_H
#define SYS_INFO_DEF_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


// 内置表的database 前面片段，用于网关和内置表都能获取到系统状态用途
#define KEY_NUM     2

struct db_database_info_type
{
    uint16_t version;           //版本号
    uint16_t md5;               //md5 
    uint16_t size;              //结构体大小  
    uint16_t reserve[61];       //占位，尽量不影响后面的结构体
};


/*
    //设备信息，65000特殊寄存器段
*/
struct db_info_device_type
{
    uint16_t  SpecialSymbol;        //特殊标识
    char      device[16];           //产品型号
    uint16_t  encrypt;              //是否加密，0表示未加密，1表示加密
    uint32_t  pt;                   //PT变比，计算因子0.01   100 (1.00)
    uint32_t  ct;                   //CT变比，计算因子0.01
    uint16_t  ProtocolVer;          //协议版本号
    uint16_t  SoftVersion;          //软件版本号 
    uint16_t  TestVersion;          //测试版本号   
    uint16_t  HardVersion;          //硬件版本号
    char      CustomInfo[24];       //定制单信息
    char      SNInfo[48];           // SN号信息
    uint16_t  reserve;
}; 

/*
    版本号信息
*/
struct db_info_version_type
{
    uint16_t SoftVersion;          //软件版本号
    uint16_t TestVersion;          //测试版本号
    uint8_t  CustomInfo[122];      //定制单信息
    uint16_t HardVersion;          //硬件版本号
    uint16_t reserve;
};

/*
    状态报文, 由进程sync_status更新
    其他进程读取进行上送或显示等操作
*/
struct db_info_status_type
{
    int16_t   SimIsInsert;          //是否插卡
    int16_t   SimIsReg;             //是否注册
    int16_t   SimCsq;               //SIM CSQ 0-31 -99
    // int16_t    SimRsrq;             //CESQ Rsrq
    int16_t   SimRsrp;              //CESQ Rsrp
    uint8_t   SimIccid[24];         //SIM ICCID
    uint8_t   ModuleImei[16];       //模块IMEI
    uint32_t  MemFree;              //mem free, unit:kB
    uint32_t  DiskFree;             //rootfs free, unit:kB
    uint16_t   poweroff_status;      //0:掉电恢复 0xF00F:产生掉电
    struct timeval poweroff_Time;
    uint16_t  wifi_ready;
    uint16_t  reserve[1];           //占位，尽量不影响后面的结构体
};

struct db_info_product_test_type
{
    uint16_t    key[KEY_NUM];
    uint16_t    poweroff;
    uint16_t    rtc;
    uint16_t    testing;        
    uint16_t    s1;
    uint16_t    s2;
};


struct db_database_cfg_info_type
{
    struct db_info_device_type         device;
    struct db_info_version_type        version;
    struct db_info_status_type         status;
    struct db_info_product_test_type   product_test;
};

struct db_database_type
{
    struct db_database_info_type            db_info;        //结构体信息
    struct db_database_cfg_info_type        db_cfg_info;

    // 其他未用到，暂不加进来，详细可查看内置表的 db_struct.h
};
// ENDOF 内置表的database 前面片段，用于网关和内置表都能获取到系统状态用途



uint16_t sysinfo_ram_free();
uint16_t sysinfo_disk_free();
uint64_t sysinfo_eth_mac(void);
uint32_t sysinfo_eth_gateway(void);
uint32_t sysinfo_eth_netmask(void);
uint32_t sysinfo_eth_ip(void);
uint64_t sysinfo_sn(void);
#endif
