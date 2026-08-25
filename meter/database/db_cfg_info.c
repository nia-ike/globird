/*
* Copyright (c) 2013,珠海派诺电子有限公司
* All rights reserved.
*
* 文件名称：db_cfg_info.c
* 文件标识：
* 描    述：
*
* 当前版本：V1.0
* 作    者：chenzy
* 更改说明：
* 完成日期：20131108
*
* 修改记录
*  作者       时间         版本      修改描述
*  chenzy   2013.11.08   V1.00      创建
*/
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "db_cfg_info.h"
#include "db_struct.h"

/*********************************************************************************************************
*                                                 仪表版本
*********************************************************************************************************
*/
//版本的格式必须 "Vxx.xxx.xx "  如 V6.1.222 必须如此，通讯才能解析出版本号
//软件版本号
#define SOFT_VER 100
//SVN中的版本号
#define SVN_VER  "r0134"
//硬件版本号
#define HARD_VER 100
//测试版本号
#define TEST_VER  2
//定制信息
#define CUSTOM_INFO   " "   //无定制单信息时 为空格
//通讯协议版本号
#define PROTOCOL_VER 1

#define DEVICE_NAME "GBE-CH100-CW"

#define DEVICE_ID   700

#define VERSION_FILE        "/ego/system/version"
#define SN_FILE             "/userdata/system/sn"
#define MODEL_FILE          "/userdata/system/model"
#define MAC_FILE            "/userdata/system/mac"
#define SYSTEM_MAC_FILE     "/sys/class/net/eth0/address"

/* EGO700 and EGO700L model */
enum 
{
    MODEL_EGO700,
    MODEL_EGO700L,
};
/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            0 - EGO700  1 - EGO700L.                       **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int rn7302_ego_model(void)
{
    int ret = MODEL_EGO700;
    char model[32], n;

    int fd = open("/userdata/system/model", O_RDONLY);

    if(fd < 0)
        return ret;

    n = read(fd, model, sizeof(model));
    if(n <= 0)
    {
        goto __error;
    }

    if((model[0] == 'E') && (model[1] == 'G') && (model[2] == 'O'))
    {
        if(model[6] == 'L')
            ret = MODEL_EGO700L;
    }
    
__error:
    close(fd);
    return ret;    

}

/*********************************************************************************************************
** Function name:       get_softversion
** Descriptions:        从版本号文件中获取版本号，写入共享内存
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
uint16_t get_softver(char *filename)
{
    FILE * fp = NULL;
    int n = 0;
    uint16_t softver = SOFT_VER;
    char version[16] = {0};
    uint8_t ver1 = 0, ver2 = 0, ver3 = 0;
    int major = 0, minor = 0, patch = 0;

    fp = fopen(filename, "r");
    if(NULL == fp)
    {
        return softver;
    }

    n = fread(version, 1, sizeof(version)-1, fp);
    if(n <= 0)
    {
        goto __err_close;
    }
    version[n] = '\0';
    
    /* 格式：
     * V2.0.1r0002 - 格式：V{major}.{minor}.{patch}r{build}
     */
    if(version[0] == 'V' || version[0] == 'v')
    {
        /* 跳过 'V'，解析版本号 */
        /* 先尝试解析三部分版本号：V{major}.{minor}.{patch}r{build} */
        sscanf(version + 1, "%d.%d.%d", &major, &minor, &patch);        
        /* 格式：V{major}.{minor}.{patch}r{build} */
        ver1 = (major > 99) ? 99 : major;
        ver2 = (minor > 9) ? 9 : minor;
        ver3 = (patch > 9) ? 9 : patch;
        softver = ver1 * 100 + ver2 * 10 + ver3;
    }
    else
    {
        goto __err_close;
    }

__err_close:
    fclose(fp);

    return softver;
}


/*********************************************************************************************************
** Function name:       get_SVNver
** Descriptions:        从版本号文件中获取版本号，写入共享内存
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
uint16_t get_SVNver(char *filename)
{
    FILE * fp = NULL;
    int n = 0;
    uint16_t softver = TEST_VER;
    char version[16] = {0};
    uint8_t ver1 = 0, ver2 = 0, ver3 = 0;
    int major = 0, minor = 0, patch = 0, build = 0;

    fp = fopen(filename, "r");
    if(NULL == fp)
    {
        return softver;
    }

    n = fread(version, 1, sizeof(version)-1, fp);
    if(n <= 0)
    {
        goto __err_close;
    }
    version[n] = '\0';
    
    /* 格式：
     * V2.0.1r0002 - 格式：V{major}.{minor}.{patch}r{build}
     */
    if(version[0] == 'V' || version[0] == 'v')
    {
        /* 跳过 'V'，解析版本号 */
        /* 先尝试解析三部分版本号：V{major}.{minor}.{patch}r{build} */
        sscanf(version + 1, "%d.%d.%dr%d", &major, &minor, &patch, &build);        
        /* 格式：V{major}.{minor}.{patch}r{build} */
        build = (build > 9999) ? 9999 : build;
    }
    else
    {
        goto __err_close;
    }

__err_close:
    fclose(fp);

    return build;
}
/*********************************************************************************************************
** Function name:       merge_model_sn
** Descriptions:        条形码+设备型号拼接
                        条形码:TSM2205010001
                        设备型号:TASO700Pro-LLCL44-6CT
                        拼接后：TSM2205010001-TASO700Pro-LLCL44-6CT
** input parameters:    SnInfo - 输出缓冲区
                        sn - 条形码字符串
                        model - 设备型号字符串
                        sn_len - 条形码长度
                        model_len - 设备型号长度
** output parameters:   NONE
** Returned value:      0 - 成功
*********************************************************************************************************/
int merge_model_sn(char *SnInfo, char *sn, char *model, uint8_t sn_len, uint8_t model_len)
{
    /* 确保字符串以null结尾 */
    char sn_buf[64] = {0};
    char model_buf[64] = {0};
    size_t max_len;
    
    /* 安全复制条形码和设备型号 */
    if(sn_len > 0 && sn_len < sizeof(sn_buf))
    {
        strncpy(sn_buf, sn, sn_len);
        sn_buf[sn_len] = '\0';
    }
    else if(sn_len > 0)
    {
        strncpy(sn_buf, sn, sizeof(sn_buf) - 1);
        sn_buf[sizeof(sn_buf) - 1] = '\0';
    }
    
    if(model_len > 0 && model_len < sizeof(model_buf))
    {
        strncpy(model_buf, model, model_len);
        model_buf[model_len] = '\0';
    }
    else if(model_len > 0)
    {
        strncpy(model_buf, model, sizeof(model_buf) - 1);
        model_buf[sizeof(model_buf) - 1] = '\0';
    }
    
    /* 计算最大可用长度（CustomInfo从索引30开始，总长度122，可用92字节） */
    max_len = 92;  /* 122 - 30 = 92 */
    
    /* 拼接：条形码-设备型号 */
    snprintf(SnInfo, max_len, "%s-%s", sn_buf, model_buf);
    
    return 0;
}

/*********************************************************************************************************
** Function name:       get_softversion
** Descriptions:        从版本号文件中获取版本号，写入共享内存
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
int get_char(char *filename, char *character, uint8_t len)
{
    FILE * fp = NULL;
    int n = 0;

    fp = fopen(filename, "r");
    if(NULL == fp)
    {
        return -1;
    }

    n = fread(character, 1, len-1, fp);
    if(n <= 0)
    {
        goto __err_close;
    }

    character[n] = '\0';

__err_close:
    fclose(fp);

    return n;
}

/*********************************************************************************************************
** Function name:       get_device_name_from_model
** Descriptions:        从MODEL_FILE文件中读取完整型号字符串，赋值给device
** input parameters:    filename - 模型文件路径
                        device - 输出缓冲区
                        device_size - 缓冲区大小
** output parameters:   device - 提取的设备名称
** Returned value:      0 - 成功, -1 - 失败
*********************************************************************************************************/
int get_device_name_from_model(const char *filename, char *device, size_t device_size)
{
    FILE *fp = NULL;
    char model[64] = {0};
    int n = 0;
    size_t copy_len = 0;

    if(!filename || !device || device_size == 0)
    {
        return -1;
    }

    fp = fopen(filename, "r");
    if(NULL == fp)
    {
        return -1;
    }

    n = fread(model, 1, sizeof(model) - 1, fp);
    if(n <= 0)
    {
        goto __err_close;
    }
    model[n] = '\0';

    /* 去掉末尾换行和空白 */
    while(n > 0 && isspace((unsigned char)model[n - 1]))
    {
        model[--n] = '\0';
    }

    copy_len = strlen(model);
    if(copy_len == 0)
    {
        goto __err_close;
    }

    /* 确保不超过缓冲区大小 */
    if(copy_len >= device_size)
    {
        copy_len = device_size - 1;
    }

    /* 复制完整型号 */
    strncpy(device, model, copy_len);
    device[copy_len] = '\0';

    fclose(fp);
    return 0;

__err_close:
    if(fp != NULL)
    {
        fclose(fp);
    }
    return -1;
}

/*********************************************************************************************************
** Function name:       modbus_config_deviceinfo_init
** Descriptions:        modbus rtu 协议设备信息初始化
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void db_deviceinfo_init(void *args)
{
    struct db_database_type *pstVmem  = (struct db_database_type *)args;
    struct db_cfg_user_para *pCfgUser = &(pstVmem->db_measurebase[0].db_cfg.user);
    struct db_info_device_type *pstDeviceInfo     = &(pstVmem->db_cfg_info.device);
    struct db_info_version_type *pVersion         = &(pstVmem->db_cfg_info.version);
    struct db_cfg_user_network_para *db_cfg_user_network = &(pstVmem->db_measurebase[0].db_cfg.user.network);

    int model_num = 0;
    int sn_num = 0;
    char model[32];
    char sn[20];

    pstDeviceInfo->SpecialSymbol  = 65000;
    pstDeviceInfo->encrypt = 0;
    pstDeviceInfo->pt = pCfgUser->base.PT.value;                    //PT变比
    pstDeviceInfo->ct = pCfgUser->base.CT.value;                    //CT变比
    pstDeviceInfo->ProtocolVer = PROTOCOL_VER;        //协议版本号
    pstDeviceInfo->SoftVersion = get_softver(VERSION_FILE);            //软件版本号
    pstDeviceInfo->TestVersion = get_SVNver(VERSION_FILE);              //SVN版本号
    pstDeviceInfo->HardVersion = HARD_VER;             //硬件版本号
    pstDeviceInfo->reserve = 0;
    get_char(SN_FILE, (char*)&pstDeviceInfo->SNInfo[0], sizeof(pstDeviceInfo->SNInfo));
    /* 从MODEL_FILE文件中获取完整型号作为设备名称 */
    if(get_device_name_from_model(MODEL_FILE, (char*)&pstDeviceInfo->device[0], sizeof(pstDeviceInfo->device)) != 0)
    {
        /* 如果获取失败，使用默认值 */
        snprintf((char*)&pstDeviceInfo->device[0], sizeof(pstDeviceInfo->device),"%s", DEVICE_NAME);
    }
    snprintf((char*)&pstDeviceInfo->CustomInfo[0], sizeof(pstDeviceInfo->CustomInfo),"%s", CUSTOM_INFO);
    model_num = get_char(MODEL_FILE, (char*)model, sizeof(model));
    sn_num    = get_char(SN_FILE, (char*)sn, sizeof(sn));
    if(sn_num > 4)
    {
        merge_model_sn((char*)&pVersion->CustomInfo[30],  (char*)sn, (char*)model, sn_num, model_num);
    } 

    /*load mac*/
    if(0 > get_char(MAC_FILE, (char*)&db_cfg_user_network->mac_addr, sizeof(db_cfg_user_network->mac_addr)))
    {
        get_char(SYSTEM_MAC_FILE, (char*)&db_cfg_user_network->mac_addr, sizeof(db_cfg_user_network->mac_addr));
    }
}




