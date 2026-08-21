/*
* Copyright (c) 2019,珠海科技股份有限公司
* All rights reserved.
* 
* 文件名称：modbusconfig.c
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
*  chenzy   2019.09.18       V1.01      更改寄存器段配置方式
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <unistd.h> 
#include <sys/stat.h>  //stat
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>

#include "cJSON.h"
#include "cJSON_Utils.h"

#include "i2c.h"
#include "mb_cfg.h"
#include "mb_protocol.h"
#include "db_device.h"
#include "db_struct.h"
#include "db_msgevent_api.h"
#include "db_msr_msgevent_api.h"
#include "mb_datatype.h"
#include "db_cfg_user_multirate.h"
#include "mb_com_baseclass.h"
#include "remap_data_standard.h"






static struct system_info
{
    uint32_t    unix_time;
    uint16_t    timezone;
};

/*********************************************************************************************************
** Function name:       mb_config_init
** Descriptions:        modbus rtu 协议配置初始化,主要是从EEPROM中加载配置，并判断配置是否正确
** input parameters:    vmem 内存
**                      cfg 配置
**                      protocol 协议
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void mb_config_init(void *vmem, char *cfg, struct mb_comm_ctx_t *pCtx)
{
    struct system_info   *tmp_systeminfo = mb_systeminfo_pointer_get();
    struct db_database_type *pstVmem = (struct db_database_type *)vmem;

    /*
        根据配置文件中的协议类型，找到对应的协议配置函数表
    */        
    mb_config_standard_fops.init(pCtx->pvmem);

    /* 
        initial message queue. 
        use for post message command to process [db_process] 
     */
    
    mb_get_timezone(&tmp_systeminfo->timezone);
}


/*********************************************************************************************************
** Function name:       mb_config_destory
** Descriptions:        modbus rtu 销毁配置
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void mb_config_destory(void)
{
    db_msg_event_api_destory();
    db_msr_msg_event_api_destory();
}




/*********************************************************************************************************
** Function name:       mb_config_getSectionCnt
** Descriptions:        modbus rtu 获取寄存器段的个数
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      寄存器段的个数
*********************************************************************************************************/
uint8_t mb_config_getSectionCnt(struct mb_comm_ctx_t *pCtx)
{
    return mb_config_standard_fops.getSectionCnt();
}


/*********************************************************************************************************
** Function name:       mb_config_getSectionAttr
** Descriptions:        modbus rtu 第N个寄存器段的属性
** input parameters:    index 第几个寄存器段
**                      pAttr 寄存器段属性的指针
** output parameters:   NONE
** Returned value:      0:Success  Other:fail
*********************************************************************************************************/
int mb_config_getSectionAttr(struct mb_comm_ctx_t *pCtx, uint8_t index, const ST_REG_4XXXX_ATTR **pAttr)
{  
    return mb_config_standard_fops.getSectionAttr(index, pAttr);
}

/*********************************************************************************************************
** Function name:       mb_config_getSectionCnt
** Descriptions:        modbus rtu 获取寄存器段的个数
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      寄存器段的个数
*********************************************************************************************************/
uint8_t mb_config_getBlockSectionCnt(struct mb_comm_ctx_t *pCtx)
{
    return mb_config_standard_fops.getBlockSectionCnt();
}

/*********************************************************************************************************
** Function name:       mb_config_getSectionAttr
** Descriptions:        modbus rtu 第N个寄存器段的属性
** input parameters:    index 第几个寄存器段
**                      pAttr 寄存器段属性的指针
** output parameters:   NONE
** Returned value:      0:Success  Other:fail
*********************************************************************************************************/
int mb_config_getBlockSectionAttr(struct mb_comm_ctx_t *pCtx, uint8_t index, const ST_REG_4XXXX_ATTR_BLOCK **pAttr)
{
    return mb_config_standard_fops.getBlockSectionAttr(index, pAttr);
}

/*********************************************************************************************************
** Function name:       mb_config_setSectionSelector
** Descriptions:        modbus rtu 设置寄存器段选择器
** input parameters:    selector - 寄存器段选择器. 1:表1, 2:表2   (固定绑定modbusRTU 通讯地址1，2)
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void mb_config_setSectionSelector(struct mb_comm_ctx_t *pCtx, uint8_t selector)
{
    (void)pCtx;
    (void)selector;
}


/*********************************************************************************************************
** Function name:       modbusrtu_baudrate_update
** Descriptions:        modbus rtu 波特率更新
** input parameters:    baud 0~3:2400 4800 9600 19200
** output parameters:   None
** Returned value:      0: success other:fail
*********************************************************************************************************/
int mb_config_baudrate_update(uint8_t baud)
{
  
    return 0;
}



/*********************************************************************************************************
** Function name:       mb_config_Set4xxxx
** Descriptions:        有关写EEPram数据对应的硬件设置
** input parameters:    reg_addr 寄存器首地址
**                      reg_num，寄存器数量
**                      p_value  寄存器数据指针
** output parameters:   None
** Returned value:      None
*********************************************************************************************************/
int mb_config_Set4xxxx(struct mb_comm_ctx_t *pCtx, const ST_REG_4XXXX_ATTR *pSectionAttr, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value)
{
    struct remap_Modbus_node     *pModifyConfig;  //要修改的数据位置指针
    /*
        make sure that the writing parameters CAN NOT over the section range. 
    */
    if((reg_addr < pSectionAttr->reg_begin) || 
        ((reg_addr+reg_num) > (pSectionAttr->reg_begin+pSectionAttr->reg_len))) 
        return -1;
    
    /*
        (reg_addr - pSectionAttr->reg_begin)   calculate the offset in this section
        and get the pointer in this section.
                            
                                        section
         -----------------------------------------------------------------------
        |                                                                       |
         -----------------------------------------------------------------------
                        ^
                        |
                        ------- pModifyConfig
    */
    pModifyConfig = pSectionAttr->p_data + (reg_addr - pSectionAttr->reg_begin);
    /* 
        set the parameters. 
    */
    if(pSectionAttr->handle)
        return pSectionAttr->handle(pCtx, pModifyConfig, reg_addr, reg_num, p_value);
    
    return 0;
}

/*********************************************************************************************************
** Function name:       mb_config_block_Set4xxxx
** Descriptions:        有关写EEPram数据对应的硬件设置
** input parameters:    reg_addr 寄存器首地址
**                      reg_num，寄存器数量
**                      p_value  寄存器数据指针
** output parameters:   None
** Returned value:      None
*********************************************************************************************************/
int mb_config_block_Set4xxxx(struct mb_comm_ctx_t *pCtx, const ST_REG_4XXXX_ATTR_BLOCK *pSectionAttr, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value)
{
    struct remap_Modbus_block_node     *pModifyConfig;  //要修改的数据位置指针
    
    /*
        make sure that the writing parameters CAN NOT over the section range. 
    */
    if((reg_addr < pSectionAttr->reg_begin) || 
        ((reg_addr+reg_num) > (pSectionAttr->reg_begin+pSectionAttr->reg_len))) 
        return -1;
    
    /*
        块映射设备只有映射首地址（区别于单个映射区如usercfg）,在handle内部根据输入的原始地址orgin_regaddr 与reg_addr 计算偏移 n个u16
                            
                                        section
         -----------------------------------------------------------------------
        |                                                                       |
         -----------------------------------------------------------------------
        ^              
        |              
        ------- pModifyConfig              
    */
    pModifyConfig = pSectionAttr->p_data;
    
    /* 
        set the parameters. 
    */
    if(pSectionAttr->handle)
        return pSectionAttr->handle(pCtx, pModifyConfig, pSectionAttr->reg_begin, reg_addr, reg_num, p_value);
    
    return 0;
}


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
int mb_config_set(struct mb_comm_ctx_t *pCtx, const ST_REG_4XXXX_ATTR *pSectionAttr, uint16_t regAddr, uint8_t regNum, uint8_t *pValue)
{
    
    return mb_config_Set4xxxx(pCtx, pSectionAttr, regAddr, regNum, pValue);      
}

/*********************************************************************************************************
** Function name:       mb_config_block_set
** Descriptions:        modbus    更新配置
** input parameters:    pSectionAttr 配置更新所在的段属性
**                      regAddr      寄存器地址
**                      regNum       寄存器个数
**                      pValue       预制的寄存器值
** output parameters:   None
** Returned value:      0:正常    其他:异常
*********************************************************************************************************/
int mb_config_block_set(struct mb_comm_ctx_t *pCtx, const ST_REG_4XXXX_ATTR_BLOCK *pSectionAttr, uint16_t regAddr, uint8_t regNum, uint8_t *pValue)
{
    
    return mb_config_block_Set4xxxx(pCtx, pSectionAttr, regAddr, regNum, pValue);      
}

/*********************************************************************************************************
** Function name:       mb_config_Get4xxxx
** Descriptions:        有关写EEPram数据对应的硬件设置
** input parameters:    reg_addr 寄存器首地址
**                      reg_num，寄存器数量
**                      p_value  寄存器数据指针
** output parameters:   None
** Returned value:      None
*********************************************************************************************************/
uint16_t* mb_custom_Get4xxxx(struct mb_comm_ctx_t *pCtx, const ST_REG_4XXXX_ATTR *pSectionAttr, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value)
{   
    /*
        make sure that the writing parameters CAN NOT over the section range. 
    */
    if((reg_addr < pSectionAttr->reg_begin) || 
        ((reg_addr+reg_num) > (pSectionAttr->reg_begin+pSectionAttr->reg_len))) 
        return NULL;
 
    if(pSectionAttr->readhandle)  
        return pSectionAttr->readhandle(pCtx, reg_addr, reg_num, p_value);
    
    return NULL;
}

/*********************************************************************************************************
** Function name:       mb_custom_get
** Descriptions:        modbus    更新配置
** input parameters:    pSectionAttr 配置更新所在的段属性
**                      regAddr      寄存器地址
**                      regNum       寄存器个数
**                      pValue       预制的寄存器值
** output parameters:   None
** Returned value:      0:正常    其他:异常
*********************************************************************************************************/
uint16_t* mb_custom_get(struct mb_comm_ctx_t *pCtx, const ST_REG_4XXXX_ATTR *pSectionAttr, uint16_t regAddr, uint8_t regNum, uint8_t *pValue)
{   
    return mb_custom_Get4xxxx(pCtx, pSectionAttr, regAddr, regNum, pValue);      
}

/*********************************************************************************************************
** Function name:       mb_config_writecoil_handle()
** Descriptions:        写继电器数据写回调
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
int mb_config_writecoil_handle(struct mb_comm_ctx_t *pCtx, uint16_t wRegAddr, uint16_t wRegValue)
{
    /* DO (Digital Output) 功能已移除 */
    (void)pCtx;
    (void)wRegAddr;
    (void)wRegValue;
    return -1;  // 功能不支持
}

