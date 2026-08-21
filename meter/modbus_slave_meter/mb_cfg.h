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
#include "mb_com_baseclass.h"


#define MB_CFG_FILE         "/userdata/config/comm/universally/modbustcp_slave_client.json"

#define MB_WRITE_PASSWORD   700
#define MB_WRITE_CMD        888

#define MB_READ_ENABLE (1 << 0)
#define MB_WRITE_ENABLE (1 << 1)
#define MB_PASSWORD_ENABLE (1 << 2)


#define MB_WRITE_SEQ_REGADDR    0xFFFF       //65535寄存器, 作为生产测试写序号的寄存器
#define MB_PRODUCT_SEQ_FILE         "/tmp/seq"       //生产测试临时序号文件


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


struct remap_Modbus_custom_node
{
    /*
        原始值属性。  从寄存器转换得来
    */
    uint16_t *origin_node;  // 原始值地址
    float    origin_kfactor;// 原始值系数，一般为放大系数
    uint8_t  origin_type;   // 原始值类型。

    /*
        转发属性
    */
    uint8_t  fw_type;       // 转发类型
    uint8_t  fw_xth;        // 本寄存器填入值的 第x个字

};

struct remap_Modbus_forword_node
{
    uint8_t         electric_flag;//电能标志，1:电能         0:非电能
    int64_t         *value;

};

struct remap_Modbus_block_node
{
    /*
        原始值属性。  
    */
    uint16_t *node;  // 原始值地址, 通常是 等于pDB 共享内存的首地址+offset。也可以是本进程定义的连续结构体首地址
    uint32_t  offset; // 偏移： 该数据段在pDB 的偏移量。  一般如果是本进程定义的结构体，这里为0 如
                      // db_measure.energy,  则 检索时 按 offset_of(pDB, db_measure.energy)
    uint32_t  reg_len;//寄存器个数
    uint8_t   order;   // 读时作为字节序： 0 为小端        1为大端

};


struct remap_Modbus_soe_node
{
    /*
        原始值属性。  从寄存器转换得来
    */
    uint16_t *node;  // 原始值地址
    uint8_t nItem;     // 参数所占寄存器个数  16bit为单位
    uint8_t order;   // 读时作为字节序： 0 为小端 
};

typedef struct
{
    struct remap_Modbus_node *p_data; //存储位置基址
    uint16_t reg_begin;               //寄存器起始位置
    uint16_t reg_len;                 //寄存器个数
    uint8_t data_type;                //数据类型
    uint8_t write_enable;             //读写权限
    uint16_t* (*readhandle)(struct mb_comm_ctx_t *pCtx, uint16_t, uint16_t, uint8_t *);
    int (*handle)(struct mb_comm_ctx_t *pCtx, struct remap_Modbus_node *, uint16_t, uint16_t, uint8_t *);
} ST_REG_4XXXX_ATTR;



typedef struct
{
    struct remap_Modbus_block_node *p_data; //存储位置基址
    uint16_t reg_begin;               //寄存器起始位置
    uint16_t reg_len;                 //寄存器个数
    uint8_t data_type;                //数据类型
    uint8_t write_enable;             //读写权限
	uint16_t* (*readhandle)(struct mb_comm_ctx_t *pCtx, uint16_t, uint16_t, uint8_t *);		//读取回调
    int (*handle)(struct mb_comm_ctx_t *pCtx, struct remap_Modbus_block_node *, uint16_t, uint16_t, uint16_t, uint8_t *);
} ST_REG_4XXXX_ATTR_BLOCK;


/*
    厂家配置函数表
*/
struct mb_remap_vendor_fops_t
{
    void (*init)(void *vmem);
    uint8_t (*getSectionCnt)(void);
    int (*getSectionAttr)(uint8_t index, const ST_REG_4XXXX_ATTR **pAttr);
    uint8_t (*getBlockSectionCnt)(void);
    int (*getBlockSectionAttr)(uint8_t index, const ST_REG_4XXXX_ATTR_BLOCK **pAttr);
    void (*setSectionSelector)(uint8_t selector);
};


void mb_config_init(void *vmem, char *cfg, struct mb_comm_ctx_t *pCtx);
int  mb_get_timezone(uint16_t *timezone);
int  mb_set_timezone(uint16_t timezone);
struct system_info *mb_systeminfo_pointer_get(void);
/*********************************************************************************************************
** Function name:       mb_config_getSectionCnt
** Descriptions:        modbus rtu 获取寄存器段的个数
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      寄存器段的个数
*********************************************************************************************************/
uint8_t mb_config_getSectionCnt(struct mb_comm_ctx_t *pCtx);
uint8_t mb_config_getCustomSectionCnt(struct mb_comm_ctx_t *pCtx);
uint8_t mb_config_getForwordSectionCnt(struct mb_comm_ctx_t *pCtx);
uint8_t mb_config_getBlockSectionCnt(struct mb_comm_ctx_t *pCtx);

/*********************************************************************************************************
** Function name:       mb_config_getSectionAttr
** Descriptions:        modbus rtu 第N个寄存器段的属性
** input parameters:    index 第几个寄存器段
**                      pAttr 寄存器段属性的指针
** output parameters:   NONE
** Returned value:      0:Success  Other:fail
*********************************************************************************************************/
int mb_config_getSectionAttr(struct mb_comm_ctx_t *pCtx, uint8_t index, const ST_REG_4XXXX_ATTR **pAttr);
int mb_config_getBlockSectionAttr(struct mb_comm_ctx_t *pCtx, uint8_t index, const ST_REG_4XXXX_ATTR_BLOCK **pAttr);


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
int mb_config_set(struct mb_comm_ctx_t *pCtx, const ST_REG_4XXXX_ATTR *pSectionAttr, uint16_t regAddr, uint8_t regNum, uint8_t *pValue);
int mb_config_block_set(struct mb_comm_ctx_t *pCtx, const ST_REG_4XXXX_ATTR_BLOCK *pSectionAttr, uint16_t regAddr, uint8_t regNum, uint8_t *pValue);

/*********************************************************************************************************
** Function name:       modbusrtu_baudrate_update
** Descriptions:        modbus rtu 波特率更新
** input parameters:    baud 0~3:2400 4800 9600 19200
** output parameters:   None
** Returned value:      0: success other:fail
*********************************************************************************************************/
int mb_config_baudrate_update(uint8_t baud);
void mb_config_setSectionSelector(struct mb_comm_ctx_t *pCtx, uint8_t selector);


void mb_config_getUnitTime(void);

uint16_t* mb_custom_get(struct mb_comm_ctx_t *pCtx, const ST_REG_4XXXX_ATTR *pSectionAttr, uint16_t regAddr, uint8_t regNum, uint8_t *pValue);

void *Vmem(void *P);

int mb_config_writecoil_handle(struct mb_comm_ctx_t *pCtx, uint16_t wRegAddr, uint16_t wRegValue);

#endif
