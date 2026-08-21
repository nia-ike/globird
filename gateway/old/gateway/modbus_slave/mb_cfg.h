/*
* Copyright (c) 2019,珠海科技股份有限公司
* All rights reserved.
*
* 文件名称：modbusconfig.h
* 文件标识：
* 描    述：
*
* 当前版本：V1.0
* 作    者：wangcq
* 更改说明：
* 完成日期：20190530
* 运行环境：
操作系统: RT-Thread
硬件平台：STM32F030
固件库  ：V3.1.2
* 修改记录
*  作者       时间         版本      修改描述
*  wangcq   2019.05.30       V1.00      创建
*  chenzy   2019.09.18      V1.01       1. 增加modbus设备描述符的接口获取
                                        2.
*/

#ifndef _MB_CFG_H_
#define _MB_CFG_H_

#include <stdint.h>
#include <stdbool.h>
#include "mb_com_baseclass.h"


#define MB_CFG_FILE         "/userdata/config/comm/universally/modbustcp_slave_client.json"

#define MB_READ_ENABLE (1 << 0)
#define MB_WRITE_ENABLE (1 << 1)
#define MB_PASSWORD_ENABLE (1 << 2)


// 扩大2倍的属性表。 实际测点最大还是8192.  主要用于增加的30000地址做的调试
#define MB_FW_POINT_MAX         (8192 * 2) // 8192



/* 串口属性  NO USE. 使用电表本身的串口参数，不使用这里的参数，仅仅预留 */
struct config_chnl_serial_attr
{
	uint32_t 					baud;			
	uint8_t						datasize;
	uint8_t						parity;
	uint8_t						stopbit;
};

/* 通道扩展属性 */
struct config_chnl_ext_attr
{
	char							whitelist1[16];		
	char							whitelist2[16];	
};

/* 点表 */
struct config_chnl_node_attr
{
    uint8_t                         func_code;
    uint8_t                         datatype;
    uint8_t                         datattr;    /*0-AI 1-DI 2-ACC*/
	uint16_t                        reg_no;
    uint32_t                        db_link;
    float                           kfactor;
    float                           bfactor;
    
};


/*	工作参数	 */
struct config_chnl_working_attr
{
    bool                            debug;
	uint8_t							checked;
	uint8_t							id;
	char							dev[48];										/* /dev/ttyS1, /dev/lora.... */
	char							mode[16];										/* poll, listen */
	char							link[24];										/* serial, tcp client */
    char							protocol[24];									/* 协议类型：modbusTCP\modbusRTU\ */
    char							register_type[24];								/* serial, tcp client */
    char							register_context[64];							/* serial, tcp client */
    uint8_t                         rs485_addr;

    uint16_t                            node_num;
    struct config_chnl_node_attr        node_attr[MB_FW_POINT_MAX];
    struct config_chnl_ext_attr         ext_attr;
	struct config_chnl_serial_attr		serial_attr;

    char                           name[64];
    char                           sn[32];

    struct mb_comm_ctx_t                mb_comm_ctx;
};









enum
{
    RO = MB_READ_ENABLE,                                         //只读
    WO = MB_WRITE_ENABLE,                                        //只写
    RW_NP = MB_READ_ENABLE | MB_WRITE_ENABLE,                    //读写，不需要密码
    RW_P = MB_READ_ENABLE | MB_WRITE_ENABLE | MB_PASSWORD_ENABLE //读写，需要密码
};                                                               //读写权限

enum
{
    MB_TYPE_RAM = 0, //RAM内存数据
    MB_TYPE_ROM,     //存储非易失数据
    MB_TYPE_CMD,     //其他数据(命令数据)
    MB_TYPE_LOCAL,   // 非共享内存数据，由modbus进程自行产生
    MB_TYPE_CUSTOM,     // 非共享内存数据，由modbus进程自行产生
};                   //数据类型

struct remap_Modbus_node
{
    uint16_t *node;  // 该modbus寄存器指向 变量的指针, 用作读取值
    uint32_t offset; // 偏移： 该modbus寄存器指向的变量 在 数据存储区的偏移， 偏移字节数/sizeof(uint16_t)
    /*
        映射表中有16bit，32bit,64bit数值
        但modbus映射是以16bit为寄存器基本单元，所以映射表中每个寄存器会截取某32bit（或64bit）的部分16bit数
        而offset字段 是该寄存器截取的片段的地址，非原变量的起始地址。只是方便modbus读取
        origin_offset 即为原变量在结构体的偏移），方便后续程序还原原变量
        一般配合下面字段byte使用：
        origin_offset 还原变量原始地址，再根据byte来决定还原不同结构，其中一个用途就是获取改变量的范围和默认值

        局限性：
            要求32bit  64bit的变量配置时， 范围约束的最大值设置为0xffffffff。 因为变量拆分成多寄存器，范围较难确定
    */
    uint32_t origin_offset;  // 偏移字节数 / sizeof(uint16_t)
    uint8_t nItem;     // 参数所占寄存器个数  16bit为单位
    uint8_t order;   // 读时作为字节序： 0 为小端        1为大端
};



struct remap_Modbus_forword_node
{
    uint8_t         electric_flag;//电能标志，1:电能         0:非电能
    int64_t         *value;

};





typedef struct
{
    struct remap_Modbus_forword_node *p_data; //存储位置基址
    uint16_t reg_begin;               //寄存器起始位置
    uint16_t reg_len;                 //寄存器个数
    uint8_t data_type;                //数据类型
    uint8_t write_enable;             //读写权限
    int (*handle)(struct remap_Modbus_node *, uint16_t, uint16_t, uint8_t *);
} ST_REG_4XXXX_ATTR_FORWORD;





/*********************************************************************************************************
** Function name:       mb_config_getSectionCnt
** Descriptions:        modbus rtu 获取寄存器段的个数
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      寄存器段的个数
*********************************************************************************************************/
uint8_t mb_config_getSectionCnt(void);
uint8_t mb_config_getForwordSectionCnt(void);
uint8_t mb_config_getBlockSectionCnt(void);

/*********************************************************************************************************
** Function name:       mb_config_getSectionAttr
** Descriptions:        modbus rtu 第N个寄存器段的属性
** input parameters:    index 第几个寄存器段
**                      pAttr 寄存器段属性的指针
** output parameters:   NONE
** Returned value:      0:Success  Other:fail
*********************************************************************************************************/
int mb_config_getForwordSectionAttr(uint8_t index, const ST_REG_4XXXX_ATTR_FORWORD **pAttr);

/*********************************************************************************************************
** Function name:       mb_config_set
** Descriptions:        modbus    更新配置
** input parameters:    pSectionAttr 配置更新所在的段属性
**                      regAddr      寄存器地址
**                      regNum       寄存器个数
**                      pValue       预制的寄存器值
** output parameters:   None
** Returned value:      0:正常    其他:异常
*********************************************************************************************************/
int  mb_config_init(struct config_chnl_working_attr *pCfg_chnlattr, char *cfg_file);
int mb_cfg_project_load(struct config_chnl_working_attr *pCfg, char *file);
int mb_config_static_info(struct config_chnl_working_attr *pCfg_chnlattr, char *pbuf, uint16_t len, uint8_t *mac, uint8_t mac_len);

int mb_get_nodeattr_by_regno(struct config_chnl_node_attr index[], uint16_t reg_no, int low, int high);

#endif
