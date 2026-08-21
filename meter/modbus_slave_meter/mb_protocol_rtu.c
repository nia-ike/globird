/*
* Copyright (c) 2013,珠海派诺电子有限公司
* All rights reserved.
*
* 文件名称：
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
*  chenzy   2020.6.09   V1.00      创建
*/
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "db_struct.h"

#include "mb_protocol.h"
#include "mb_cfg.h"
#include "mb_com_baseclass.h"
#include "mb_datatype.h"
#include "mb_updatefile.h"
#include "mb_configfile.h"
#include "dev_log.h"
#include "crc16.h"

#pragma pack(push,1)

/*
*   读寄存器状态 03H
*/
typedef struct tagExceptMsg
{
    uint8_t     SlaveAddr;      // 从机地址
    uint8_t     Cmd;            // 功能码
    uint8_t     errCode;        // 异常码
}ModBusExceptMsg, *LPModBusExceptMsg;

/*
*   读寄存器状态 03H
*/
typedef struct tagReadREG03H
{
    uint8_t     SlaveAddr;      // 从机地址
    uint8_t     Cmd;            // 03H 读开关量
    uint16_t    wRegAddr;        // 寄存器地址
    uint16_t    wRegNum;         // 寄存器个数
}ModBusReadREG03H, *LPModBusReadREG03H;

/*
*   读寄存器回应 03H
*/
typedef struct tagReadREG03H_ack
{
    uint8_t     SlaveAddr;      // 从机地址
    uint8_t     Cmd;            // 03H 读寄存器
    uint8_t     BytesNum;         // 字节个数  （寄存器 * 2）
} ModBusReadREG03H_ack, *LPModBusReadREG03H_ack;

/*
*   读寄存器状态 04H
*/
typedef struct tagReadREG04H
{
    uint8_t     SlaveAddr;      // 从机地址
    uint8_t     Cmd;            // 03H 读开关量
    uint16_t    wRegAddr;        // 寄存器地址
    uint16_t    wRegNum;         // 寄存器个数
}ModBusReadREG04H, *LPModBusReadREG04H;

/*
*   读寄存器回应 04H
*/
typedef struct tagReadREG04H_ack
{
    uint8_t     SlaveAddr;      // 从机地址
    uint8_t     Cmd;            // 03H 读寄存器
    uint8_t     BytesNum;         // 字节个数  （寄存器 * 2）
} ModBusReadREG04H_ack, *LPModBusReadREG04H_ack;


/*
*   写继电器 05H
*/
typedef struct tagWriteCoilREG05H
{
    uint8_t     SlaveAddr;      // 从机地址
    uint8_t     Cmd;            // 03H 读开关量
    uint16_t    wRegAddr;        // 寄存器地址
    uint16_t    wRegValue;      // 继电器命令
}ModBusWriteCoilREG05H, *LPModBusWriteCoilREG05H;

/*
*   写继电器回应 05H
*/
typedef struct tagWriteCoilREG05H_ack
{
    uint8_t     SlaveAddr;      // 从机地址
    uint8_t     Cmd;            // 03H 读开关量
    uint16_t    wRegAddr;        // 寄存器地址
    uint16_t    wRegValue;      // 继电器命令
} ModBusWriteCoilREG05H_ack, *LPModBusWriteCoilREG05H_ack;

/*
*   写寄存器  10H
*/
typedef struct tagWriteREG10H
{
    uint8_t     SlaveAddr;      // 从机地址
    uint8_t     Cmd;            // 10H 读寄存器
    uint16_t    wRegAddr;       // 寄存器地址
    uint16_t    wRegNum;        // 寄存器个数
    uint8_t     BytesNum;       // 字节个数
}ModBusWriteREG10H, *LPModBusWriteREG10H;

/*
*   写寄存器回应  10H
*/
typedef struct tagWriteREG10H_ack
{
    uint8_t     SlaveAddr;      // 从机地址
    uint8_t     Cmd;            // 10H 读寄存器
    uint16_t    wRegAddr;        // 寄存器地址
    uint16_t    wRegNum;         // 寄存器个数
}ModBusWriteREG10H_ack, *LPModBusWriteREG10H_ack;


/*
*   写寄存器  06H
*/
typedef struct tagWriteREG06H
{
    uint8_t     SlaveAddr;      // 从机地址
    uint8_t     Cmd;            // 06H 读寄存器
    uint16_t    wRegAddr;       // 寄存器地址
//    uint16_t    wRegNum;        // 寄存器个数
//    uint8_t     BytesNum;       // 字节个数
}ModBusWriteREG06H, *LPModBusWriteREG06H;

/*
*   写寄存器回应  06H
*/
typedef struct tagWriteREG06H_ack
{
    uint8_t     SlaveAddr;      // 从机地址
    uint8_t     Cmd;            // 06H 读寄存器
    uint16_t    wRegAddr;        // 寄存器地址
//    uint16_t    wRegNum;         // 寄存器个数
}ModBusWriteREG06H_ack, *LPModBusWriteREG06H_ack;

/**Modbus异常码**/
typedef enum
{
    MB_ILLEGAL_FUNCTION = 0x01,     //异常码01，非法功能
    MB_ILLEGAL_DATA_ADDRESS = 0x02, //异常码02，非法数据地址
    MB_ILLEGAL_DATA_VALUE = 0x03    //异常码03，非法数据值
} ModbusExptCode;

/*
*   读寄存器状态 03H
*/
typedef struct tagModbusHead
{
    uint8_t SlaveAddr; // 从机地址
    uint8_t Cmd;       // 功能码
} *LPModBusHead;


typedef struct tagGetFileInfo44H
{
    uint8_t     SlaveAddr;      // 从机地址
    uint8_t     Cmd;            // 44H
    uint8_t     devtype;
    uint32_t    filesize;
    uint16_t    filecrc;
    uint8_t     reserved[8];
}ModBusGetFileInfo44H, *LPModBusGetFileInfo44H;

typedef struct tagGetFileInfo44H_ack
{
    uint8_t     SlaveAddr;      // 从机地址
    uint8_t     Cmd;            // 44H
    uint8_t     devtype;
    uint32_t    filesize;
    uint16_t    filecrc;
    uint32_t    curoffset;
    uint8_t     id;
    uint8_t     reserved[4];
}ModBusGetFileInfo44H_ack, *LPModBusGetFileInfo44H_ack;


typedef struct tagSndFile45H
{
    uint8_t     SlaveAddr;      // 从机地址
    uint8_t     Cmd;            // 45H
    uint8_t     devtype;
    uint32_t    filesize;
    uint16_t    filecrc;
    uint32_t    curoffset;
    uint8_t     request_id;
    uint16_t    packsize;

}ModBusSndFile45H, *LPModBusSndFile45H;


typedef struct tagSndFile45H_ack
{
    uint8_t     SlaveAddr;      // 从机地址
    uint8_t     Cmd;            // 45H
    uint8_t     devtype;
    uint32_t    filesize;
    uint16_t    filecrc;
    uint32_t    curoffset;
    uint8_t     id;
    uint8_t     reserved[4];

}ModBusSndFile45H_ack, *LPModBusSndFile45H_ack;


typedef struct tagGetFile46H
{
    uint8_t     SlaveAddr;      // 从机地址
    uint8_t     Cmd;            // 46H
    uint8_t     devtype;
    uint32_t    filesize;
    uint16_t    filecrc;
    uint32_t    curoffset;
    uint8_t     request_id;
    uint16_t    packsize;
}ModBusGetFile46H, *LPModBusGetFile46H;

typedef struct tagGetFile46H_ack
{
    uint8_t     SlaveAddr;      // 从机地址
    uint8_t     Cmd;            // 46H
    uint8_t     devtype;
    uint32_t    filesize;
    uint16_t    filecrc;
    uint32_t    curoffset;
    uint8_t     id;
    uint16_t    packsize;
    uint8_t     reserved[2];
}ModBusGetFile46H_ack, *LPModBusGetFile46H_ack;


typedef struct tagSndFile47H
{
    uint8_t     SlaveAddr;      // 从机地址
    uint8_t     Cmd;            // 47H
    uint8_t     devtype;
    uint32_t    filesize;
    uint16_t    filecrc;
    uint32_t    curoffset;
    uint8_t     request_id;
    uint16_t    packsize;

}ModBusSndFile47H, *LPModBusSndFile47H;


typedef struct tagSndFile47H_ack
{
    uint8_t     SlaveAddr;      // 从机地址
    uint8_t     Cmd;            // 47H
    uint8_t     devtype;
    uint32_t    filesize;
    uint16_t    filecrc;
    uint32_t    curoffset;
    uint8_t     id;
    uint8_t     reserved[4];

}ModBusSndFile47H_ack, *LPModBusSndFile47H_ack;

typedef struct tagShell48H
{
    uint8_t     SlaveAddr;      // 从机地址
    uint8_t     Cmd;            // 48H
    uint8_t     devtype;
    uint16_t    packsize;
    uint16_t    packcrc;
}ModBusShell48H, *LPModBusShell48H;


typedef struct tagShell48H_ack
{
    uint8_t     SlaveAddr;      // 从机地址
    uint8_t     Cmd;            // 48H
    uint8_t     devtype;
    uint32_t    packsize;
    uint16_t    packcrc;
}ModBusShell48H_ack, *LPModBusShell48H_ack;

typedef struct tagDebugMsg49H
{
    uint8_t     SlaveAddr;      // 从机地址
    uint8_t     Cmd;            // 49H
    uint8_t     DebugState;
    uint8_t     ChannelNo;
    uint8_t     DevNo;
    uint16_t    ReadLen;
    uint16_t    Reserved;
}ModBusDebugMsg49H, *LPModBusDebugMsg49H;


typedef struct tagDebugMsg49H_ack
{
    uint8_t     SlaveAddr;      // 从机地址
    uint8_t     Cmd;            // 49H
    uint16_t    UnreadLen;
    uint16_t    Reserved;
    uint16_t    DataLen;
}ModBusDebugMsg49H_ack, *LPModBusDebugMsg49H_ack;


typedef struct tagDebugMsg4AH_ack
{
    uint8_t     SlaveAddr;      // 从机地址
    uint8_t     Cmd;            // 4AH
    uint8_t     uninstall_result;
    uint16_t    undefined;
}ModBusDebugMsg4AH_ack, *LPModBusDebugMsg4AH_ack;

typedef struct tagReadREG4AH
{
    uint8_t     SlaveAddr;      // 从机地址
    uint8_t     Cmd;            // 4AH 读开关量
    uint8_t     app_name_len;   // 名字长度
    char        app_name[0];    // app名
}ModBusReadREG4AH, *LPModBusReadREG4AH;

#pragma pack(pop)

static int executeCMD(const char *cmd, char *result,int len)
{
    FILE *ptr;
    memset((void*)result,0,len);
    if((ptr=popen(cmd, "r"))!=NULL)
    {
        fgets(result, len, ptr);
        result[len] = 0;
        pclose(ptr);
        ptr = NULL;
        return 0;
    }
    else
    {
        return -1;
    }
}


/************************************************************************
函数功能：  mb_protocol_packet_exceptmsg
            MODBUS RTU 异常报文回复响应组包函数
输入参数：  pBuff    组包缓冲区（一般设置缓冲不大于300）
            funcode  对应的功能码
            devAddr  设备地址
            errCode  异常码
输出参数：
返回值  ：  数据包大小
说明    ：
************************************************************************/
static uint8_t mb_protocol_packet_exceptmsg(uint8_t *pBuff, uint8_t devAddr, uint8_t funcode, uint8_t errCode)
{
    uint16_t             crc;

    LPModBusExceptMsg    pExceptMsg;
    pExceptMsg = (LPModBusExceptMsg)pBuff;
    pExceptMsg->SlaveAddr = devAddr;
    pExceptMsg->Cmd       = funcode | 0x80;
    pExceptMsg->errCode   = errCode;

    crc = crc16(&pExceptMsg->SlaveAddr, sizeof(ModBusExceptMsg));
    crc = SWAP_16BIT(crc); // CRC byte order is little endian
    MB_ALIGN_PT_16BIT_WRITE(pBuff + sizeof(ModBusExceptMsg), crc);

    return sizeof(ModBusExceptMsg) + CRC16_SIZE;
}

/*********************************************************************************************************
** Function name:       mb_write_process_10H
** Descriptions:        modbus 写寄存器值消息处理函数
** input parameters:    transAddr 设备地址
**                      pRecvBuff 接收到的数据
**                      recvLen   接收到的数据长度
** output parameters:   None
** Returned value:      0:正常    其他:异常
*********************************************************************************************************/
static int mb_process_userdef(struct mb_comm_ctx_t *pCtx, uint16_t regAddr, uint16_t regNum, uint8_t *pVal)
{
    int ret = -1;
    switch(regAddr)
    {
        case MB_WRITE_SEQ_REGADDR:  //生产测试写序号，只在rtu模式下有意义
            {
                uint16_t val;
                char ch;
                if(regNum == 1)
                {
                    val = MB_ALIGN_PT_16BIT_READ(pVal);
                    if(val >= 1 & val <= 4)       //生产测试工装 1-4#
                    {
                        ch = '0' + val;      //数字转为字符，方便生产测试zmodem获取解析
                        FILE * fp = fopen(MB_PRODUCT_SEQ_FILE, "w");
                        if(NULL == fp)
                        {
                            ret = -1;
                            break;
                        }
                        fwrite(&ch, sizeof(ch), 1, fp);
                        fclose(fp);
                        fp = NULL;
                        ret = 0;
                    }
                }
            }
            break;

        default:                    //预留未来可扩展
            break;
    }
    return ret;
}

/************************************************************************
函数功能：  mb_protocol_packet03h
            MODBUS RTU 读保持寄存器响应组包函数
输入参数：  pBuff       组包缓冲区（一般设置缓冲不大于300）
            devAddr     设备地址
            valueFiled  寄存器值所在位置
            regNum      寄存器数量
输出参数：
返回值  ：  数据包大小
说明    ：
************************************************************************/
static uint8_t mb_protocol_packet03h(uint8_t *pBuff, uint8_t devAddr, struct remap_Modbus_node *pNode, uint8_t regNum)
{
    uint8_t                     *pValue;
    uint16_t                    crc;
    uint8_t                     pkgsize;
    uint8_t                     i;
    LPModBusReadREG03H_ack      pReadReg;

    pReadReg = (LPModBusReadREG03H_ack)pBuff;
    pReadReg->SlaveAddr = devAddr;
    pReadReg->Cmd       = _FC_READ_HOLDING_REGISTERS;
    pReadReg->BytesNum  = 0;
    for(i = 0; i < regNum;i++)
    {
        pValue = (pBuff + sizeof(ModBusReadREG03H_ack) + pReadReg->BytesNum);
        if(pNode->order == 0) //REG_LITTLE_EDIAN
            MB_ALIGN_PT_16BIT_WRITE(pValue, *pNode->node);
        else
            MB_ALIGN_PT_16BIT_WRITE(pValue, SWAP_16BIT((*pNode->node)));

        pReadReg->BytesNum +=  REG_SIZE;
        pNode++;
    }
    pkgsize = sizeof(ModBusReadREG03H_ack) + pReadReg->BytesNum;

    crc = crc16(&pReadReg->SlaveAddr, pkgsize);
    crc = SWAP_16BIT(crc); // CRC byte order is little endian
    MB_ALIGN_PT_16BIT_WRITE(pBuff + pkgsize, crc);

    return pkgsize + CRC16_SIZE;
}

/************************************************************************
函数功能：  mb_protocol_remap_packet03h
            MODBUS RTU 读保持寄存器响应组包函数
输入参数：  pBuff       组包缓冲区（一般设置缓冲不大于300）
            devAddr     设备地址
            valueFiled  寄存器值所在位置
            regNum      寄存器数量
输出参数：
返回值  ：  数据包大小
说明    ：
************************************************************************/
static int mb_protocol_remap_packet03h(uint8_t *pBuff, uint8_t devAddr, struct remap_Modbus_custom_node *pNode, uint8_t regNum)
{
    uint8_t                     *pValue;
    uint16_t                    crc;
    uint8_t                     pkgsize;
    uint8_t                     i;
    double                      f64val;
    uint16_t                    regValue;
    LPModBusReadREG03H_ack      pReadReg;

    pReadReg = (LPModBusReadREG03H_ack)pBuff;
    pReadReg->SlaveAddr = devAddr;
    pReadReg->Cmd       = _FC_READ_HOLDING_REGISTERS;
    pReadReg->BytesNum  = 0;
    for(i = 0; i < regNum;i++)
    {
        pValue = (pBuff + sizeof(ModBusReadREG03H_ack) + pReadReg->BytesNum);
        /*
            convent the origin register value to double.
        */
        if(pNode->origin_node != NULL)
        {
            Modbus_RegDatatoDouble(pNode->origin_type, 0, pNode->origin_node, &f64val);
            f64val = f64val * pNode->origin_kfactor;
            f64val = ((double)(((f64val+0.0005)*1000)))/1000;
            Modbus_DoubleToReg16(pNode->fw_type, pNode->fw_xth, &regValue, &f64val);
            MB_ALIGN_PT_16BIT_WRITE(pValue, regValue);
        }
        else
        {
            MB_ALIGN_PT_16BIT_WRITE(pValue, 0);
        }

        pReadReg->BytesNum +=  REG_SIZE;
        pNode++;
    }
    pkgsize = sizeof(ModBusReadREG03H_ack) + pReadReg->BytesNum;

    crc = crc16(&pReadReg->SlaveAddr, pkgsize);
    crc = SWAP_16BIT(crc); // CRC byte order is little endian
    MB_ALIGN_PT_16BIT_WRITE(pBuff + pkgsize, crc);

    return pkgsize + CRC16_SIZE;
}

/************************************************************************
函数功能：  mb_protocol_packet03h
            MODBUS RTU 读保持寄存器响应组包函数
输入参数：  pBuff       组包缓冲区（一般设置缓冲不大于300）
            devAddr     设备地址
            valueFiled  寄存器值所在位置
            regNum      寄存器数量
输出参数：
返回值  ：  数据包大小
说明    ：
************************************************************************/
static uint8_t mb_protocol_custom_packet03h(uint8_t *pBuff, uint8_t devAddr, uint16_t *pNode, uint8_t regNum, uint8_t order)
{
    uint8_t                     *pValue;
    uint16_t                    crc;
    uint8_t                     pkgsize;
    uint8_t                     i;
    LPModBusReadREG03H_ack      pReadReg;

    pReadReg = (LPModBusReadREG03H_ack)pBuff;
    pReadReg->SlaveAddr = devAddr;
    pReadReg->Cmd       = _FC_READ_HOLDING_REGISTERS;
    pReadReg->BytesNum  = 0;
    for(i = 0; i < regNum;i++)
    {
        pValue = (pBuff + sizeof(ModBusReadREG03H_ack) + pReadReg->BytesNum);
       
        if(order == 0) //REG_LITTLE_EDIAN
            MB_ALIGN_PT_16BIT_WRITE(pValue, *pNode);
        else
            MB_ALIGN_PT_16BIT_WRITE(pValue, SWAP_16BIT(*pNode));

        pReadReg->BytesNum +=  REG_SIZE;
        pNode++;
    }
    pkgsize = sizeof(ModBusReadREG03H_ack) + pReadReg->BytesNum;

    crc = crc16(&pReadReg->SlaveAddr, pkgsize);
    crc = SWAP_16BIT(crc); // CRC byte order is little endian
    MB_ALIGN_PT_16BIT_WRITE(pBuff + pkgsize, crc);

    return pkgsize + CRC16_SIZE;
}


/************************************************************************
函数功能：  mb_protocol_block_packet03h
            MODBUS RTU 读保持寄存器响应组包函数
输入参数：  pBuff       组包缓冲区（一般设置缓冲不大于300）
            devAddr     设备地址
            valueFiled  寄存器值所在位置
            regNum      寄存器数量
输出参数：
返回值  ：  数据包大小
说明    ：
************************************************************************/
static uint8_t mb_protocol_block_packet03h(uint8_t *pBuff, uint8_t devAddr, struct remap_Modbus_block_node *pinfo, uint16_t *pNode, uint8_t regNum)
{
    uint8_t                     *pValue;
    uint16_t                    crc;
    uint8_t                     pkgsize;
    uint8_t                     i;
    LPModBusReadREG03H_ack      pReadReg;

    pReadReg = (LPModBusReadREG03H_ack)pBuff;
    pReadReg->SlaveAddr = devAddr;
    pReadReg->Cmd       = _FC_READ_HOLDING_REGISTERS;
    pReadReg->BytesNum  = 0;
    for(i = 0; i < regNum;i++)
    {
        pValue = (pBuff + sizeof(ModBusReadREG03H_ack) + pReadReg->BytesNum);

        if(pinfo->order == 0) //REG_LITTLE_EDIAN
            MB_ALIGN_PT_16BIT_WRITE(pValue, *pNode);
        else
            MB_ALIGN_PT_16BIT_WRITE(pValue, SWAP_16BIT((*pNode)));

        pReadReg->BytesNum +=  REG_SIZE;
        pNode++;
    }
    pkgsize = sizeof(ModBusReadREG03H_ack) + pReadReg->BytesNum;

    crc = crc16(&pReadReg->SlaveAddr, pkgsize);
    crc = SWAP_16BIT(crc); // CRC byte order is little endian
    MB_ALIGN_PT_16BIT_WRITE(pBuff + pkgsize, crc);

    return pkgsize + CRC16_SIZE;
}

/************************************************************************
函数功能：  mb_protocol_packet04h
            MODBUS RTU 读保持寄存器响应组包函数
输入参数：  pBuff       组包缓冲区（一般设置缓冲不大于300）
            devAddr     设备地址
            valueFiled  寄存器值所在位置
            regNum      寄存器数量
输出参数：
返回值  ：  数据包大小
说明    ：
************************************************************************/
static uint8_t mb_protocol_packet04h(uint8_t *pBuff, uint8_t devAddr, struct remap_Modbus_node *pNode, uint8_t regNum)
{
    uint8_t                     *pValue;
    uint16_t                    crc;
    uint8_t                     pkgsize;
    uint8_t                     i;
    LPModBusReadREG04H_ack      pReadReg;

    pReadReg = (LPModBusReadREG04H_ack)pBuff;
    pReadReg->SlaveAddr = devAddr;
    pReadReg->Cmd       = _FC_READ_INPUT_REGISTERS;
    pReadReg->BytesNum  = 0;
    for(i = 0; i < regNum;i++)
    {
        pValue = (pBuff + sizeof(ModBusReadREG04H_ack) + pReadReg->BytesNum);
        if(pNode->order == 0) //REG_LITTLE_EDIAN
            MB_ALIGN_PT_16BIT_WRITE(pValue, *pNode->node);
        else
            MB_ALIGN_PT_16BIT_WRITE(pValue, SWAP_16BIT((*pNode->node)));

        pReadReg->BytesNum +=  REG_SIZE;
        pNode++;
    }
    pkgsize = sizeof(ModBusReadREG04H_ack) + pReadReg->BytesNum;

    crc = crc16(&pReadReg->SlaveAddr, pkgsize);
    crc = SWAP_16BIT(crc); // CRC byte order is little endian
    MB_ALIGN_PT_16BIT_WRITE(pBuff + pkgsize, crc);

    return pkgsize + CRC16_SIZE;
}

/************************************************************************
函数功能：  mb_protocol_remap_packet03h
            MODBUS RTU 读保持寄存器响应组包函数
输入参数：  pBuff       组包缓冲区（一般设置缓冲不大于300）
            devAddr     设备地址
            valueFiled  寄存器值所在位置
            regNum      寄存器数量
输出参数：
返回值  ：  数据包大小
说明    ：
************************************************************************/
static int mb_protocol_remap_packet04h(uint8_t *pBuff, uint8_t devAddr, struct remap_Modbus_custom_node *pNode, uint8_t regNum)
{
    uint8_t                     *pValue;
    uint16_t                    crc;
    uint8_t                     pkgsize;
    uint8_t                     i;
    double                      f64val;
    uint16_t                    regValue;
    LPModBusReadREG04H_ack      pReadReg;

    pReadReg = (LPModBusReadREG04H_ack)pBuff;
    pReadReg->SlaveAddr = devAddr;
    pReadReg->Cmd       = _FC_READ_INPUT_REGISTERS;
    pReadReg->BytesNum  = 0;
    for(i = 0; i < regNum;i++)
    {
        pValue = (pBuff + sizeof(ModBusReadREG04H_ack) + pReadReg->BytesNum);
        /*
            convent the origin register value to double.
        */
        if(pNode->origin_node != NULL)
        {
            Modbus_RegDatatoDouble(pNode->origin_type, 0, pNode->origin_node, &f64val);
            f64val = f64val * pNode->origin_kfactor;
            f64val = ((double)(((f64val+0.0005)*1000)))/1000;
            Modbus_DoubleToReg16(pNode->fw_type, pNode->fw_xth, &regValue, &f64val);
            MB_ALIGN_PT_16BIT_WRITE(pValue, regValue);
        }
        else
        {
            MB_ALIGN_PT_16BIT_WRITE(pValue, 0);
        }

        pReadReg->BytesNum +=  REG_SIZE;
        pNode++;
    }
    pkgsize = sizeof(ModBusReadREG04H_ack) + pReadReg->BytesNum;

    crc = crc16(&pReadReg->SlaveAddr, pkgsize);
    crc = SWAP_16BIT(crc); // CRC byte order is little endian
    MB_ALIGN_PT_16BIT_WRITE(pBuff + pkgsize, crc);

    return pkgsize + CRC16_SIZE;
}

/************************************************************************
函数功能：  mb_protocol_packet04h
            MODBUS RTU 读保持寄存器响应组包函数
输入参数：  pBuff       组包缓冲区（一般设置缓冲不大于300）
            devAddr     设备地址
            valueFiled  寄存器值所在位置
            regNum      寄存器数量
输出参数：
返回值  ：  数据包大小
说明    ：
************************************************************************/
static uint8_t mb_protocol_custom_packet04h(uint8_t *pBuff, uint8_t devAddr, uint16_t *pNode, uint8_t regNum, uint8_t order)
{
    uint8_t                     *pValue;
    uint16_t                    crc;
    uint8_t                     pkgsize;
    uint8_t                     i;
    LPModBusReadREG04H_ack      pReadReg;

    pReadReg = (LPModBusReadREG04H_ack)pBuff;
    pReadReg->SlaveAddr = devAddr;
    pReadReg->Cmd       = _FC_READ_INPUT_REGISTERS;
    pReadReg->BytesNum  = 0;
    for(i = 0; i < regNum;i++)
    {
        pValue = (pBuff + sizeof(ModBusReadREG04H_ack) + pReadReg->BytesNum);
       
        if(order == 0) //REG_LITTLE_EDIAN
            MB_ALIGN_PT_16BIT_WRITE(pValue, *pNode);
        else
            MB_ALIGN_PT_16BIT_WRITE(pValue, SWAP_16BIT(*pNode));

        pReadReg->BytesNum +=  REG_SIZE;
        pNode++;
    }
    pkgsize = sizeof(ModBusReadREG04H_ack) + pReadReg->BytesNum;

    crc = crc16(&pReadReg->SlaveAddr, pkgsize);
    crc = SWAP_16BIT(crc); // CRC byte order is little endian
    MB_ALIGN_PT_16BIT_WRITE(pBuff + pkgsize, crc);

    return pkgsize + CRC16_SIZE;
}


/************************************************************************
函数功能：  mb_protocol_block_packet04h
            MODBUS RTU 读保持寄存器响应组包函数
输入参数：  pBuff       组包缓冲区（一般设置缓冲不大于300）
            devAddr     设备地址
            valueFiled  寄存器值所在位置
            regNum      寄存器数量
输出参数：
返回值  ：  数据包大小
说明    ：
************************************************************************/
static uint8_t mb_protocol_block_packet04h(uint8_t *pBuff, uint8_t devAddr, struct remap_Modbus_block_node *pinfo, uint16_t *pNode, uint8_t regNum)
{
    uint8_t                     *pValue;
    uint16_t                    crc;
    uint8_t                     pkgsize;
    uint8_t                     i;
    LPModBusReadREG04H_ack      pReadReg;

    pReadReg = (LPModBusReadREG04H_ack)pBuff;
    pReadReg->SlaveAddr = devAddr;
    pReadReg->Cmd       = _FC_READ_INPUT_REGISTERS;
    pReadReg->BytesNum  = 0;
    for(i = 0; i < regNum;i++)
    {
        pValue = (pBuff + sizeof(ModBusReadREG04H_ack) + pReadReg->BytesNum);

        if(pinfo->order == 0) //REG_LITTLE_EDIAN
            MB_ALIGN_PT_16BIT_WRITE(pValue, *pNode);
        else
            MB_ALIGN_PT_16BIT_WRITE(pValue, SWAP_16BIT((*pNode)));

        pReadReg->BytesNum +=  REG_SIZE;
        pNode++;
    }
    pkgsize = sizeof(ModBusReadREG04H_ack) + pReadReg->BytesNum;

    crc = crc16(&pReadReg->SlaveAddr, pkgsize);
    crc = SWAP_16BIT(crc); // CRC byte order is little endian
    MB_ALIGN_PT_16BIT_WRITE(pBuff + pkgsize, crc);

    return pkgsize + CRC16_SIZE;
}

/************************************************************************
函数功能：  mb_protocol_packet05h
            MODBUS RTU 写继电器响应组包函数
输入参数：  pBuff       组包缓冲区（一般设置缓冲不大于300）
            devAddr     设备地址
            valueFiled  寄存器值所在位置
            regNum      寄存器数量
输出参数：
返回值  ：  数据包大小
说明    ：
************************************************************************/
static uint8_t mb_protocol_packet05h(uint8_t *pBuff, uint8_t devAddr, uint16_t regNo, uint16_t value)
{
    uint16_t                    crc;
    uint8_t                     pkgsize;
    LPModBusWriteCoilREG05H_ack pWriteCoil_ack;

    pWriteCoil_ack = (LPModBusWriteCoilREG05H_ack)pBuff;
    pWriteCoil_ack->SlaveAddr = devAddr;
    pWriteCoil_ack->Cmd       = _FC_WRITE_COIL_REGISTERS;
    pWriteCoil_ack->wRegAddr  = SWAP_16BIT(regNo);
    pWriteCoil_ack->wRegValue = SWAP_16BIT(value);
    
    pkgsize = sizeof(ModBusWriteCoilREG05H_ack) ;

    crc = crc16(&pWriteCoil_ack->SlaveAddr, pkgsize);
    crc = SWAP_16BIT(crc); // CRC byte order is little endian
    MB_ALIGN_PT_16BIT_WRITE(pBuff + pkgsize, crc);

    return pkgsize + CRC16_SIZE;
}

/************************************************************************
函数功能：  mb_protocol_packet10h
            MODBUS RTU 写寄存器响应组包函数
输入参数：  pBuff    组包缓冲区（一般设置缓冲不大于300）
            devAddr  设备地址
            regAddr  寄存器起始地址
            regNum   寄存器数量
输出参数：
返回值  ：  数据包大小
说明    ：
************************************************************************/
static uint8_t mb_protocol_packet10h(uint8_t *pBuff, uint8_t devAddr, uint16_t regAddr, uint8_t regNum)
{
    uint16_t            crc;
    LPModBusWriteREG10H_ack  pWriteReg;

    pWriteReg = (LPModBusWriteREG10H_ack)pBuff;
    pWriteReg->SlaveAddr = devAddr;
    pWriteReg->Cmd       = _FC_WRITE_MULTIPLE_REGISTERS;
    pWriteReg->wRegAddr  = SWAP_16BIT(regAddr);
    pWriteReg->wRegNum   = SWAP_16BIT(regNum);

    crc = crc16(&pWriteReg->SlaveAddr, sizeof(ModBusWriteREG10H_ack));
    crc = SWAP_16BIT(crc); // CRC byte order is little endian
    MB_ALIGN_PT_16BIT_WRITE(pBuff + sizeof(ModBusWriteREG10H_ack), crc);

    return sizeof(ModBusWriteREG10H_ack)+ CRC16_SIZE;
}

/************************************************************************
函数功能：  mb_protocol_packet06h
            MODBUS RTU 写寄存器响应组包函数
输入参数：  pBuff    组包缓冲区（一般设置缓冲不大于300）
            devAddr  设备地址
            regAddr  寄存器起始地址
            regNum   寄存器数量
输出参数：
返回值  ：  数据包大小
说明    ：
************************************************************************/
static uint8_t mb_protocol_packet06h(uint8_t *pBuff, uint8_t devAddr, uint16_t regAddr, uint8_t regNum)
{
    uint16_t            crc;
    LPModBusWriteREG06H_ack  pWriteReg;

    pWriteReg = (LPModBusWriteREG06H_ack)pBuff;
    pWriteReg->SlaveAddr = devAddr;
    pWriteReg->Cmd       = _FC_WRITE_SINGLE_REGISTERS;
    pWriteReg->wRegAddr  = SWAP_16BIT(regAddr);

    crc = crc16(&pWriteReg->SlaveAddr, sizeof(ModBusWriteREG06H_ack));
    crc = SWAP_16BIT(crc); // CRC byte order is little endian
    MB_ALIGN_PT_16BIT_WRITE(pBuff + sizeof(ModBusWriteREG06H_ack), crc);

    return sizeof(ModBusWriteREG06H_ack)+ CRC16_SIZE;
}


/************************************************************************
函数功能：  mb_protocol_packet44h
            MODBUS RTU 写寄存器响应组包函数
输入参数：  pBuff    组包缓冲区（一般设置缓冲不大于300）
            devAddr  设备地址
            regAddr  寄存器起始地址
            regNum   寄存器数量
输出参数：
返回值  ：  数据包大小
说明    ：
************************************************************************/
static uint8_t mb_protocol_packet44h(uint8_t *pBuff, uint8_t devAddr)
{
    uint16_t            crc;
    LPModBusGetFileInfo44H_ack  pGetFile;
    struct mb_update_ctx_t *pupdate = mb_file_getctx();

    pGetFile = (LPModBusGetFileInfo44H_ack)pBuff;
    pGetFile->SlaveAddr = devAddr;
    pGetFile->Cmd       = _FC_GET_FILE_INFO;
    pGetFile->devtype   = MB_FILE_DEV_TYPE;
    pGetFile->filesize  = pupdate->filesize;
    pGetFile->filecrc   = pupdate->filecrc;
    pGetFile->curoffset = pupdate->curoffset;
    pGetFile->id        = devAddr;

    crc = crc16(&pGetFile->SlaveAddr, sizeof(ModBusGetFileInfo44H_ack));
    crc = SWAP_16BIT(crc); // CRC byte order is little endian
    MB_ALIGN_PT_16BIT_WRITE(pBuff + sizeof(ModBusGetFileInfo44H_ack), crc);

    return sizeof(ModBusGetFileInfo44H_ack)+ CRC16_SIZE;
}

/************************************************************************
函数功能：  mb_protocol_packet45h
            MODBUS RTU 写寄存器响应组包函数
输入参数：  pBuff    组包缓冲区（一般设置缓冲不大于300）
            devAddr  设备地址
            regAddr  寄存器起始地址
            regNum   寄存器数量
输出参数：
返回值  ：  数据包大小
说明    ：
************************************************************************/
static uint8_t mb_protocol_packet45h(uint8_t *pBuff, uint8_t devAddr)
{
    uint16_t            crc;
    LPModBusSndFile45H_ack  pSndFile;
    struct mb_update_ctx_t *pupdate = mb_file_getctx();

    pSndFile = (LPModBusSndFile45H_ack)pBuff;
    pSndFile->SlaveAddr = devAddr;
    pSndFile->Cmd       = _FC_GET_FILE_INFO;
    pSndFile->devtype   = MB_FILE_DEV_TYPE;
    pSndFile->filesize  = pupdate->filesize;
    pSndFile->filecrc   = pupdate->filecrc;
    pSndFile->curoffset = pupdate->curoffset;
    pSndFile->id        = devAddr;

    crc = crc16(&pSndFile->SlaveAddr, sizeof(ModBusSndFile45H_ack));
    crc = SWAP_16BIT(crc); // CRC byte order is little endian
    MB_ALIGN_PT_16BIT_WRITE(pBuff + sizeof(ModBusSndFile45H_ack), crc);

    return sizeof(ModBusSndFile45H_ack)+ CRC16_SIZE;
}


/************************************************************************
函数功能：  mb_protocol_packet46h
            MODBUS RTU 写寄存器响应组包函数
输入参数：  pBuff    组包缓冲区（一般设置缓冲不大于300）
            devAddr  设备地址
            regAddr  寄存器起始地址
            regNum   寄存器数量
输出参数：
返回值  ：  数据包大小
说明    ：
************************************************************************/
static uint16_t mb_protocol_packet46h(uint8_t *pBuff, uint8_t devAddr)
{
    uint16_t                crc;
    LPModBusGetFile46H_ack  pGetFile;
    struct mb_config_ctx_t *pConfig_ctx = mb_file_config_getctx();

    pConfig_ctx->filesize = create_downconfig_file(0);
    pGetFile = (LPModBusGetFile46H_ack)pBuff;
    pGetFile->SlaveAddr = devAddr;
    pGetFile->Cmd       = _FC_GET_FILE_HANDLE;
    pGetFile->devtype   = MB_FILE_DEV_TYPE;
    pGetFile->filesize  = pConfig_ctx->filesize;
    pGetFile->curoffset = pConfig_ctx->curoffset;
    //printf("packsize=%d   filesize=%d   curoffset=%d \r\n",pConfig_ctx->packsize,pConfig_ctx->filesize,pConfig_ctx->curoffset  );
    if(pConfig_ctx->packsize == 0)
    {
        if(pConfig_ctx->filesize - pConfig_ctx->curoffset < 1024)
        {
            pGetFile->packsize = pConfig_ctx->filesize - pConfig_ctx->curoffset;
        }
        else
        {
            pGetFile->packsize = 512;        
        }
    }
    else
    {
        if(pConfig_ctx->curoffset + pConfig_ctx->packsize > pConfig_ctx->filesize)
        {
            if(pConfig_ctx->curoffset < pConfig_ctx->filesize)
            {
                pGetFile->packsize = pConfig_ctx->filesize - pConfig_ctx->curoffset;
            }
            else
            {
                pGetFile->packsize = 0;
            }
        }
        else
        {
            pGetFile->packsize = pConfig_ctx->packsize;
        }
    }
    mb_file_upload(pConfig_ctx, &pGetFile->SlaveAddr + sizeof(ModBusGetFile46H_ack), pGetFile->packsize);
    
    crc = crc16(&pGetFile->SlaveAddr + sizeof(ModBusGetFile46H_ack), pGetFile->packsize);
    pGetFile->filecrc = SWAP_16BIT(crc);
    pGetFile->id        = devAddr;
    crc = crc16(&pGetFile->SlaveAddr, pGetFile->packsize+sizeof(ModBusGetFile46H_ack));
    crc = SWAP_16BIT(crc); // CRC byte order is little endian
    MB_ALIGN_PT_16BIT_WRITE(pBuff + sizeof(ModBusGetFile46H_ack) + pGetFile->packsize, crc);

    //判断到当前包为最后一包的时候，删除压缩配置。使得下一次加载配置重新压缩，以解决配置更改了，但是为重启的使用情况。
    if((pConfig_ctx->curoffset + pGetFile->packsize)>=(pConfig_ctx->filesize))
    {
        unlink(MB_CONFIG_FILE);
    }
    return sizeof(ModBusGetFile46H_ack)+ CRC16_SIZE + pGetFile->packsize;
}

/************************************************************************
函数功能：  mb_protocol_packet47h
            MODBUS RTU 写寄存器响应组包函数
输入参数：  pBuff    组包缓冲区（一般设置缓冲不大于300）
            devAddr  设备地址
            regAddr  寄存器起始地址
            regNum   寄存器数量
输出参数：
返回值  ：  数据包大小
说明    ：
************************************************************************/
static uint8_t mb_protocol_packet47h(uint8_t *pBuff, uint8_t devAddr)
{
    uint16_t            crc;
    LPModBusSndFile47H_ack  pSndFile;
    struct mb_config_ctx_t *pConfig_ctx = mb_file_config_getctx();

    pSndFile = (LPModBusSndFile47H_ack)pBuff;
    pSndFile->SlaveAddr = devAddr;
    pSndFile->Cmd       = _FC_SND_FILE_HANDLE;
    pSndFile->devtype   = MB_FILE_DEV_TYPE;
    pSndFile->filesize  = pConfig_ctx->filesize;
    pSndFile->filecrc   = pConfig_ctx->filecrc;
    pSndFile->curoffset = pConfig_ctx->curoffset;
    pSndFile->id        = devAddr;

    crc = crc16(&pSndFile->SlaveAddr, sizeof(ModBusSndFile47H_ack));
    crc = SWAP_16BIT(crc); // CRC byte order is little endian
    MB_ALIGN_PT_16BIT_WRITE(pBuff + sizeof(ModBusSndFile47H_ack), crc);

    return sizeof(ModBusSndFile47H_ack)+ CRC16_SIZE;
}

/************************************************************************
函数功能：  mb_protocol_packet48h
            MODBUS RTU 写寄存器响应组包函数
输入参数：  pBuff    组包缓冲区（一般设置缓冲不大于300）
            devAddr  设备地址
            regAddr  寄存器起始地址
            regNum   寄存器数量
输出参数：
返回值  ：  数据包大小
说明    ：
************************************************************************/
static uint8_t mb_protocol_packet48h(uint8_t *pBuff, uint8_t devAddr)
{
    uint16_t                crc;
    LPModBusShell48H_ack    pShell;
    struct mb_config_ctx_t *pconfig = mb_file_config_getctx();
    uint8_t                 i;
    uint8_t                 *pValue;

    pShell = (LPModBusShell48H_ack)pBuff;
    pShell->SlaveAddr = devAddr;
    pShell->Cmd       = _FC_SHELL_COMMAND;
    pShell->devtype   = MB_FILE_DEV_TYPE;
    pShell->packsize   = pconfig->filesize;
    pShell->packcrc   = pconfig->filecrc;

    crc = crc16(&pShell->SlaveAddr, sizeof(ModBusShell48H_ack));
    crc = SWAP_16BIT(crc); // CRC byte order is little endian
    MB_ALIGN_PT_16BIT_WRITE(pBuff + sizeof(ModBusShell48H_ack), crc);

    return sizeof(ModBusShell48H_ack)+ CRC16_SIZE;
}

/************************************************************************
函数功能：  mb_protocol_packet49h
            MODBUS RTU 写寄存器响应组包函数
输入参数：  pBuff    组包缓冲区（一般设置缓冲不大于300）
            devAddr  设备地址
            regAddr  寄存器起始地址
            regNum   寄存器数量
输出参数：
返回值  ：  数据包大小
说明    ：
************************************************************************/
static uint16_t mb_protocol_packet49h(uint8_t *pBuff, uint8_t devAddr,char *MsgBuff,int ReadLen)
{
    uint16_t            crc;
    LPModBusDebugMsg49H_ack  pDebugMsg;

    pDebugMsg = (LPModBusDebugMsg49H_ack)pBuff;
    pDebugMsg->SlaveAddr = devAddr;
    pDebugMsg->Cmd       = _FC_DEBUG_MESSAGE;
    pDebugMsg->UnreadLen = log_get_unread_length();
    pDebugMsg->Reserved  = 0;
    pDebugMsg->DataLen   = ReadLen;
    
    memcpy((pBuff + sizeof(ModBusDebugMsg49H_ack)),MsgBuff,pDebugMsg->DataLen);

    crc = crc16(&pDebugMsg->SlaveAddr,pDebugMsg->DataLen + sizeof(ModBusDebugMsg49H_ack));
    crc = SWAP_16BIT(crc); // CRC byte order is little endian
    MB_ALIGN_PT_16BIT_WRITE(pBuff + sizeof(ModBusDebugMsg49H_ack) + pDebugMsg->DataLen, crc);

    return (sizeof(ModBusDebugMsg49H_ack)+ CRC16_SIZE + pDebugMsg->DataLen) ;
}

/************************************************************************
函数功能：  mb_protocol_packet4Ah
            MODBUS RTU 删除APP响应组包函数
输入参数：  pBuff    组包缓冲区（一般设置缓冲不大于300）
            devAddr  设备地址
            regAddr  寄存器起始地址
            regNum   寄存器数量
输出参数：
返回值  ：  数据包大小
说明    ：
************************************************************************/
static uint16_t mb_protocol_packet4Ah(uint8_t *pBuff, uint8_t devAddr,uint8_t result)
{
    uint16_t            crc;
    LPModBusDebugMsg4AH_ack  pDebugMsg;

    pDebugMsg = (LPModBusDebugMsg4AH_ack)pBuff;
    pDebugMsg->SlaveAddr = devAddr;
    pDebugMsg->Cmd       = _FC_UNINSTALL_APP;
    pDebugMsg->uninstall_result = result;
    pDebugMsg->undefined  = 0;
    

    crc = crc16(&pDebugMsg->SlaveAddr,sizeof(ModBusDebugMsg4AH_ack));
    crc = SWAP_16BIT(crc); // CRC byte order is little endian
    MB_ALIGN_PT_16BIT_WRITE(pBuff + sizeof(ModBusDebugMsg4AH_ack), crc);

    return (sizeof(ModBusDebugMsg4AH_ack)+ CRC16_SIZE) ;
}

/************************************************************************
函数功能：  mb_protocol_unpacket03h
            MODBUS RTU 读保持寄存器解析函数
输入参数：  pBuff    报文所在的缓冲区（一般设置缓冲不大于300）
            bufflen  报文长度
输出参数：  pRegAddr 请求的寄存器起始地址
            pRegNum  请求的寄存器数量
返回值  ：  0:解析成功  其他：解析失败
说明    ：
************************************************************************/
static int mb_protocol_unpacket03h(uint8_t *pBuff, uint16_t bufflen, uint16_t *pRegAddr, uint8_t *pRegNum)
{
    uint16_t              wTpCrc;
    uint16_t              wCalCrc;
    LPModBusReadREG03H    pReadHoldReg;

    pReadHoldReg = (LPModBusReadREG03H)pBuff;
    /*
          armv5 byte alignment

    */
    wTpCrc = MB_ALIGN_PT_16BIT_READ(&pReadHoldReg->SlaveAddr + sizeof(ModBusReadREG03H));
    wTpCrc = SWAP_16BIT(wTpCrc); // CRC byte order is little endian
    wCalCrc = crc16(&pReadHoldReg->SlaveAddr, sizeof(ModBusReadREG03H));
    if(wTpCrc == wCalCrc)
    {
        *pRegAddr = SWAP_16BIT(pReadHoldReg->wRegAddr);
        *pRegNum = SWAP_16BIT(pReadHoldReg->wRegNum);
        return 0;
    }
    return -1;
}

/************************************************************************
函数功能：  mb_protocol_unpacket04h
            MODBUS RTU 读保持寄存器解析函数
输入参数：  pBuff    报文所在的缓冲区（一般设置缓冲不大于300）
            bufflen  报文长度
输出参数：  pRegAddr 请求的寄存器起始地址
            pRegNum  请求的寄存器数量
返回值  ：  0:解析成功  其他：解析失败
说明    ：
************************************************************************/
static int mb_protocol_unpacket04h(uint8_t *pBuff, uint16_t bufflen, uint16_t *pRegAddr, uint8_t *pRegNum)
{
    uint16_t              wTpCrc;
    uint16_t              wCalCrc;
    LPModBusReadREG04H    pReadHoldReg;

    pReadHoldReg = (LPModBusReadREG04H)pBuff;
    /*
          armv5 byte alignment

    */
    wTpCrc = MB_ALIGN_PT_16BIT_READ(&pReadHoldReg->SlaveAddr + sizeof(ModBusReadREG04H));
    wTpCrc = SWAP_16BIT(wTpCrc); // CRC byte order is little endian
    wCalCrc = crc16(&pReadHoldReg->SlaveAddr, sizeof(ModBusReadREG04H));
    if(wTpCrc == wCalCrc)
    {
        *pRegAddr = SWAP_16BIT(pReadHoldReg->wRegAddr);
        *pRegNum = SWAP_16BIT(pReadHoldReg->wRegNum);
        return 0;
    }
    return -1;
}

/************************************************************************
函数功能：  mb_protocol_packet05h
            MODBUS RTU 写继电器响应组包函数
输入参数：  pBuff       组包缓冲区（一般设置缓冲不大于300）
            devAddr     设备地址
            valueFiled  寄存器值所在位置
            regNum      寄存器数量
输出参数：
返回值  ：  数据包大小
说明    ：
************************************************************************/
static int mb_protocol_unpacket05h(uint8_t *pBuff, uint16_t bufflen, uint16_t *pRegAddr, uint16_t *pValue)
{
    uint16_t                    wTpCrc;
    uint16_t                    wCalCrc;
    LPModBusWriteCoilREG05H     pWriteCoil;

    pWriteCoil = (LPModBusWriteCoilREG05H)pBuff;
    wTpCrc = MB_ALIGN_PT_16BIT_READ(&pWriteCoil->SlaveAddr + sizeof(ModBusWriteCoilREG05H));
    wTpCrc = SWAP_16BIT(wTpCrc); // CRC byte order is little endian
    wCalCrc = crc16(&pWriteCoil->SlaveAddr, sizeof(ModBusWriteCoilREG05H));

    if(wTpCrc == wCalCrc)
    {
        *pRegAddr = SWAP_16BIT(pWriteCoil->wRegAddr);
        *pValue   = SWAP_16BIT(pWriteCoil->wRegValue);
        return 0;
    }

    return -1;
}
/************************************************************************
函数功能：  mb_protocol_unpacket10h
            MODBUS RTU 写命令解析函数
输入参数：  pBuff    报文所在的缓冲区（一般设置缓冲不大于300）
            bufflen  报文长度
输出参数：  pRegAddr 写命令的寄存器起始地址
            pRegNum  写命令的寄存器个数
            pValue   写命令的内容所在位置的地址
返回值  ：  0:解析成功  其他：解析失败
说明    ：
************************************************************************/
static int mb_protocol_unpacket10h(uint8_t *pBuff, uint16_t  bufflen, uint16_t *pRegAddr, uint8_t *pRegNum, uint8_t **pValue)
{
    uint16_t           wTpCrc, wCalCrc;
    uint8_t            pkgsize;
    LPModBusWriteREG10H    pWriteReg;

    pWriteReg = (LPModBusWriteREG10H)pBuff;
    if(pWriteReg->BytesNum != (SWAP_16BIT(pWriteReg->wRegNum) * 2))
    {
        return -1;
    }

    pkgsize = sizeof(ModBusWriteREG10H)+ pWriteReg->BytesNum;
    wTpCrc = MB_ALIGN_PT_16BIT_READ(&pWriteReg->SlaveAddr + pkgsize);
    wTpCrc = SWAP_16BIT(wTpCrc); // CRC byte order is little endian
    wCalCrc = crc16(&pWriteReg->SlaveAddr, pkgsize);
    if(wTpCrc == wCalCrc)
    {
        *pRegAddr = SWAP_16BIT(pWriteReg->wRegAddr);
        *pRegNum = SWAP_16BIT(pWriteReg->wRegNum);
        *pValue =  (uint8_t *)(&pWriteReg->SlaveAddr + sizeof(ModBusWriteREG10H));
        return 0;
    }
    return -1;
}

/************************************************************************
函数功能：  mb_protocol_unpacket06h
            MODBUS RTU 写命令解析函数
输入参数：  pBuff    报文所在的缓冲区（一般设置缓冲不大于300）
            bufflen  报文长度
输出参数：  pRegAddr 写命令的寄存器起始地址
            pRegNum  写命令的寄存器个数
            pValue   写命令的内容所在位置的地址
返回值  ：  0:解析成功  其他：解析失败
说明    ：
************************************************************************/
static int mb_protocol_unpacket06h(uint8_t *pBuff, uint16_t  bufflen, uint16_t *pRegAddr, uint8_t *pRegNum, uint8_t **pValue)
{
    uint16_t           wTpCrc, wCalCrc;
    uint8_t            pkgsize;
    LPModBusWriteREG06H    pWriteReg;

    pWriteReg = (LPModBusWriteREG06H)pBuff;

    pkgsize = sizeof(ModBusWriteREG06H) + 2;

    wTpCrc = MB_ALIGN_PT_16BIT_READ(&pWriteReg->SlaveAddr + pkgsize);

    wTpCrc = SWAP_16BIT(wTpCrc); // CRC byte order is little endian
    wCalCrc = crc16(&pWriteReg->SlaveAddr, pkgsize);
   
    if(wTpCrc == wCalCrc)
    {
        *pRegAddr = SWAP_16BIT(pWriteReg->wRegAddr);
        *pRegNum = 1;
        *pValue =  (uint8_t *)(&pWriteReg->SlaveAddr + sizeof(ModBusWriteREG06H));
        return 0;
    }
    return -1;
}
/************************************************************************
函数功能：  mb_protocol_unpacket44h
            MODBUS RTU 写命令解析函数
输入参数：  pBuff    报文所在的缓冲区（一般设置缓冲不大于300）
            bufflen  报文长度
输出参数：  pRegAddr 写命令的寄存器起始地址
            pRegNum  写命令的寄存器个数
            pValue   写命令的内容所在位置的地址
返回值  ：  0:解析成功  其他：解析失败
说明    ：
************************************************************************/
static int mb_protocol_unpacket44h(uint8_t *pBuff, uint16_t  bufflen)
{
    uint16_t                    wTpCrc, wCalCrc;
    uint16_t                     pkgsize;
    LPModBusGetFileInfo44H      pGetFileInfo;
    struct mb_update_ctx_t      update_ctx;

    pGetFileInfo = (LPModBusGetFileInfo44H)pBuff;

    if(pGetFileInfo->devtype != MB_FILE_DEV_TYPE)
        return -1;

    pkgsize = sizeof(ModBusGetFileInfo44H);
    wCalCrc = crc16(&pGetFileInfo->SlaveAddr, pkgsize);
    wTpCrc  = MB_ALIGN_PT_16BIT_READ(&pGetFileInfo->SlaveAddr + pkgsize);
    wTpCrc = SWAP_16BIT(wTpCrc); // CRC byte order is little endian

    if(wTpCrc == wCalCrc)
    {
        /*
            Get new firmware info.
        */
        update_ctx.filesize = pGetFileInfo->filesize;
        update_ctx.filecrc  = pGetFileInfo->filecrc;
        update_ctx.curoffset = 0;
        mb_file_info_upate(&update_ctx);

        return 0;
    }

    return -1;
}


/************************************************************************
函数功能：  mb_protocol_unpacket45h
            MODBUS RTU 写命令解析函数
输入参数：  pBuff    报文所在的缓冲区（一般设置缓冲不大于300）
            bufflen  报文长度
输出参数：  pRegAddr 写命令的寄存器起始地址
            pRegNum  写命令的寄存器个数
            pValue   写命令的内容所在位置的地址
返回值  ：  0:解析成功  其他：解析失败
说明    ：
************************************************************************/
static int mb_protocol_unpacket45h(uint8_t *pBuff, uint16_t bufflen, uint8_t *request_id)
{
    uint16_t                    wTpCrc, wCalCrc;
    uint16_t                     pkgsize;
    LPModBusSndFile45H          pSndFile;
    struct mb_update_ctx_t      update_ctx;

    pSndFile = (LPModBusSndFile45H)pBuff;
    if(pSndFile->devtype != MB_FILE_DEV_TYPE)
        return -1;

    if(pSndFile->packsize > 1024)
        return -1;

    pkgsize = sizeof(ModBusSndFile45H) + pSndFile->packsize;

    wCalCrc = crc16(&pSndFile->SlaveAddr, pkgsize);
    wTpCrc  = MB_ALIGN_PT_16BIT_READ(&pSndFile->SlaveAddr + pkgsize);
    wTpCrc = SWAP_16BIT(wTpCrc); // CRC byte order is little endian
    if(wTpCrc == wCalCrc)
    {
        update_ctx.filesize  = pSndFile->filesize;
        update_ctx.filecrc   = pSndFile->filecrc;
        update_ctx.curoffset = pSndFile->curoffset;
        mb_file_pkg_download(&update_ctx, &pSndFile->SlaveAddr + sizeof(ModBusSndFile45H), pSndFile->packsize);
        mb_file_pkg_upgrade(&update_ctx);
        *request_id = pSndFile->request_id;
        return 0;
    }

    return -1;
}

/************************************************************************
函数功能：  mb_protocol_unpacket46h
            MODBUS RTU 写命令解析函数
输入参数：  pBuff    报文所在的缓冲区（一般设置缓冲不大于300）
            bufflen  报文长度
输出参数：  pRegAddr 写命令的寄存器起始地址
            pRegNum  写命令的寄存器个数
            pValue   写命令的内容所在位置的地址
返回值  ：  0:解析成功  其他：解析失败
说明    ：
************************************************************************/
static int mb_protocol_unpacket46h(uint8_t *pBuff, uint16_t  bufflen)
{
    uint16_t                    wTpCrc, wCalCrc;
    uint16_t                    pkgsize;
    LPModBusGetFile46H          pGetFile;
    struct mb_config_ctx_t      config_ctx;

    pGetFile = (LPModBusGetFile46H)pBuff;

    if(pGetFile->Cmd != _FC_GET_FILE_HANDLE)
               return -1;
    if(pGetFile->devtype != MB_FILE_DEV_TYPE)
        return -1;

    pkgsize = sizeof(ModBusGetFile46H);
    wCalCrc = crc16(&pGetFile->SlaveAddr, pkgsize);
    wTpCrc  = MB_ALIGN_PT_16BIT_READ(&pGetFile->SlaveAddr + pkgsize);
    wTpCrc = SWAP_16BIT(wTpCrc); // CRC byte order is little endian

    if(wTpCrc == wCalCrc)
    {
        config_ctx.curoffset = pGetFile->curoffset;
        config_ctx.packsize = SWAP_16BIT(MB_ALIGN_PT_16BIT_READ(&pGetFile->packsize));
        /* 文件大小使用配置软件下发的字段 */
        if(config_ctx.filesize <= 0) 
        {
            config_ctx.filesize = pGetFile->filesize; 
        }
        mb_file_config(&config_ctx);

        return 0;
    }

    return -1;
}


/************************************************************************
函数功能：  mb_protocol_unpacket47h
            MODBUS RTU 写命令解析函数
输入参数：  pBuff    报文所在的缓冲区（一般设置缓冲不大于300）
            bufflen  报文长度
输出参数：  pRegAddr 写命令的寄存器起始地址
            pRegNum  写命令的寄存器个数
            pValue   写命令的内容所在位置的地址
返回值  ：  0:解析成功  其他：解析失败
说明    ：
************************************************************************/
static int mb_protocol_unpacket47h(uint8_t *pBuff, uint16_t bufflen, uint8_t *request_id)
{
    uint16_t                    wTpCrc, wCalCrc;
    uint16_t                    pkgsize;
    LPModBusSndFile47H          pSndFile;
    struct mb_config_ctx_t      config_ctx;

    pSndFile = (LPModBusSndFile47H)pBuff;
    if(pSndFile->devtype != MB_FILE_DEV_TYPE)
        return -1;

    if(pSndFile->packsize > 1024)
        return -1;

    pkgsize = sizeof(ModBusSndFile47H) + pSndFile->packsize;

    wCalCrc = crc16(&pSndFile->SlaveAddr, pkgsize);
    wTpCrc  = MB_ALIGN_PT_16BIT_READ(&pSndFile->SlaveAddr + pkgsize);
    wTpCrc = SWAP_16BIT(wTpCrc); // CRC byte order is little endian
    if(wTpCrc == wCalCrc)
    {
        config_ctx.filesize  = pSndFile->filesize;
        config_ctx.filecrc   = pSndFile->filecrc;
        config_ctx.curoffset = pSndFile->curoffset;
        mb_file_download(&config_ctx, &pSndFile->SlaveAddr + sizeof(ModBusSndFile47H), pSndFile->packsize);        
        *request_id = pSndFile->request_id;

        struct mb_config_ctx_t *pLocalCtx;
        pLocalCtx = mb_file_config_getctx();
        if(pLocalCtx->curoffset == pLocalCtx->filesize)
        {
            return 1;
        }
        return 0;
    }
    return -1;
}
/************************************************************************
函数功能：  mb_protocol_unpacket48h
            MODBUS RTU 写命令解析函数
输入参数：  pBuff    报文所在的缓冲区（一般设置缓冲不大于300）
            bufflen  报文长度
输出参数：  pRegAddr 写命令的寄存器起始地址
            pRegNum  写命令的寄存器个数
            pValue   写命令的内容所在位置的地址
返回值  ：  0:解析成功  其他：解析失败
说明    ：
************************************************************************/
static int mb_protocol_unpacket48h(uint8_t *pBuff, uint16_t  bufflen)
{
    uint16_t                    wTpCrc, wCalCrc;
    uint16_t                    pkgsize;
    LPModBusShell48H            pShell;
    uint8_t                     *pValue;
    pShell = (LPModBusShell48H)pBuff;

    if(pShell->devtype != MB_FILE_DEV_TYPE)
        return -1;

    pkgsize = sizeof(ModBusShell48H);
    wCalCrc = crc16(&pShell->SlaveAddr, pkgsize);
    wTpCrc  = MB_ALIGN_PT_16BIT_READ(&pShell->SlaveAddr + pkgsize);
    wTpCrc = SWAP_16BIT(wTpCrc); // CRC byte order is little endian
    if(wTpCrc == wCalCrc)
    {
        mb_shell_command((uint8_t *)(&pShell->SlaveAddr+sizeof(ModBusShell48H)),pShell->packsize,pValue);
        return 0;
    }

    return -1;
}

/*************************************************************************/
/** Function Name:                                                      **/
/** Purpose:                                                            **/
/** Params:            buff报文内容                                     **/
/** Return:            len报文长度                                      **/
/** Notice:            flag RX或TX.                                     **/
/*************************************************************************/
static int mb_protocol_unpacket49h(uint8_t *pBuff, uint16_t bufflen, uint8_t *request_id)
{
    uint16_t                    wTpCrc, wCalCrc;
    uint16_t                     pkgsize;
    LPModBusDebugMsg49H          pDebugMsginfo;
    struct mb_update_ctx_t      update_ctx;

    pDebugMsginfo = (LPModBusDebugMsg49H)pBuff;
    pkgsize = sizeof(ModBusDebugMsg49H);

    wCalCrc = crc16(&pDebugMsginfo->SlaveAddr, pkgsize);
    wTpCrc  = MB_ALIGN_PT_16BIT_READ(&pDebugMsginfo->SlaveAddr + pkgsize);
    wTpCrc = SWAP_16BIT(wTpCrc); // CRC byte order is little endian
    if(wTpCrc == wCalCrc)
    {
        log_work_set(pDebugMsginfo->DebugState,pDebugMsginfo->ChannelNo,pDebugMsginfo->DevNo);
        return pDebugMsginfo->ReadLen;// 返回上位机想读取的长度
    }

    return -1;
}

/************************************************************************
函数功能：  mb_protocol_unpacket4Ah
            MODBUS RTU 卸载APP解析函数
输入参数：  pBuff    报文所在的缓冲区（一般设置缓冲不大于300）
            bufflen  报文长度
返回值  ：  0:解析成功  其他：解析失败
说明    ：
************************************************************************/
static int mb_protocol_unpacket4Ah(uint8_t *pBuff, uint16_t bufflen)
{
    uint16_t              wTpCrc;
    uint16_t              wCalCrc;
    LPModBusReadREG4AH    pReadHoldReg;
    
    pReadHoldReg = (LPModBusReadREG4AH)pBuff;
    /*
          armv5 byte alignment

    */
    
    wTpCrc = MB_ALIGN_PT_16BIT_READ(&pReadHoldReg->SlaveAddr + 5 + pReadHoldReg->app_name_len);
    wTpCrc = SWAP_16BIT(wTpCrc); // CRC byte order is little endian
    wCalCrc = crc16(&pReadHoldReg->SlaveAddr, 5 + pReadHoldReg->app_name_len);
    
    if(wTpCrc == wCalCrc)
    {
        return 0;
    }
    return -1;
}

/************************************************************************
函数功能：  mb_protocol_crc_check
            MODBUS RTU 接收到的报文进行CRC严格校验
输入参数：  pBuff    报文所在的缓冲区（一般设置缓冲不大于300）
            bufflen  报文长度
输出参数：
返回值  ：  0:CRC 正确  其他：CRC错误
说明    ：
************************************************************************/
static int mb_protocol_crc_check(uint8_t *pBuff, uint16_t  bufflen)
{
    uint16_t  wTpCrc, wCalCrc;

    wTpCrc = MB_ALIGN_PT_16BIT_READ(pBuff + bufflen - CRC16_SIZE);
    wTpCrc = SWAP_16BIT(wTpCrc); // CRC byte order is little endian
    wCalCrc = crc16(pBuff, bufflen - CRC16_SIZE);
    if(wTpCrc == wCalCrc)
    {
        return 0;
    }
    return -1;
}




/*********************************************************************************************************
** Function name:       mb_addr_check
** Descriptions:        地址检测是否匹配
** input parameters:    addr      地址
** output parameters:   None
** Returned value:      0 匹配    其他:不匹配
*********************************************************************************************************/
static int mb_addr_check(struct mb_comm_ctx_t *pCtx, uint8_t addr)
{
    uint8_t address;
    uint8_t address1;

    /*  */
    address = pCtx->pvmem->db_measurebase[0].db_cfg.user.base.Addr.value;
    address1 = pCtx->pvmem->db_measurebase[1].db_cfg.user.base.Addr.value;
    if((address == addr) || (address1 == addr) || (MB_COM_BROADCAST_ADDR == addr) || (MB_COM_ADMIN_ADDR == addr))
    {
        return 0;
    }

    return -1;
}

/*********************************************************************************************************
** Function name:       modbu_msg_readreg_process
** Descriptions:        modbus 读取寄存器值消息处理函数
** input parameters:    transAddr 设备地址
**                      funcode   功能码
**                      pRecvBuff 接收到的数据
**                      recvLen   接收到的数据长度
** output parameters:   None
** Returned value:      0:正常    其他:异常
*********************************************************************************************************/
static int mb_read_process_03H(struct mb_comm_ctx_t *pCtx, uint8_t *pRecvBuff, uint16_t recvLen)
{
    int ret;
    uint16_t regAddr;
    uint8_t regNum, sectionCnt, index;
    uint8_t  *pdata_u8;
    struct remap_Modbus_node *pdata;
    uint16_t *pdata_block;
    LPModBusHead pModbusHead;
    const ST_REG_4XXXX_ATTR *pSectionAttr;
    const ST_REG_4XXXX_ATTR_BLOCK    *pSectionBlockAttr;

    pModbusHead = (LPModBusHead)pRecvBuff;

    /*
        boardcast just can be used for write function.
    */
    if (pModbusHead->SlaveAddr == MB_COM_BROADCAST_ADDR)
        return -1;

    /*
        unpacket the Message with 03H code
    */
    ret = mb_protocol_unpacket03h(pRecvBuff, recvLen, &regAddr, &regNum);
    if (ret != 0)
    {
        return -1;
    }
   
    /*
        find the register from the REG TABLES
    */
    sectionCnt = mb_config_getSectionCnt(pCtx);
    for (index = 0; index < sectionCnt; index++)
    {
        ret = mb_config_getSectionAttr(pCtx, index, &pSectionAttr);

        /*
                判断是否在合法边界
        */
        if ((regAddr >= pSectionAttr->reg_begin) && (regAddr + regNum <= pSectionAttr->reg_begin + pSectionAttr->reg_len) && (pSectionAttr->write_enable & MB_READ_ENABLE))
        {
            break;
        }
    }

    if (index == sectionCnt)
    {
        // //非法数据地址，返回地址错误异常码02
        // pCtx->sendLen = mb_protocol_packet_exceptmsg(&pCtx->sendBuff[0], pModbusHead->SlaveAddr, pModbusHead->Cmd, MB_ILLEGAL_DATA_ADDRESS);
        // ret = -1;

        /* 
            can not be found in const map table
            we search the re-map table;  
        */    
        sectionCnt = mb_config_getBlockSectionCnt(pCtx);
        for (index = 0; index < sectionCnt; index++)
        {
            ret = mb_config_getBlockSectionAttr(pCtx, index, &pSectionBlockAttr);
            /*
                判断是否在合法边界
            */
            if ((regAddr >= pSectionBlockAttr->reg_begin) && ((regAddr + regNum) <= (pSectionBlockAttr->reg_begin + pSectionBlockAttr->reg_len)) && (pSectionBlockAttr->write_enable & MB_READ_ENABLE))
            {
                break;
            }
        }

        if (index == sectionCnt)
        {
            //非法数据地址，返回地址错误异常码02
            pCtx->sendLen = mb_protocol_packet_exceptmsg(&pCtx->sendBuff[0], pModbusHead->SlaveAddr, pModbusHead->Cmd, MB_ILLEGAL_DATA_ADDRESS);
            ret = -1;
        }
        else
        {
            //组块报文
            pdata_block = pSectionBlockAttr->p_data->node + (regAddr - pSectionBlockAttr->reg_begin);

            pCtx->sendLen = mb_protocol_block_packet03h(&pCtx->sendBuff[0], pModbusHead->SlaveAddr, pSectionBlockAttr->p_data, pdata_block, regNum);
        }
    }    
    else
    {
        /* 03H 数据打包前的回调。一般用于触发型的数据更新 */
        if(pSectionAttr->readhandle)  
            pSectionAttr->readhandle(pCtx, regAddr, regNum, 0);

        /* MB_TYPE_CMD 只写命令不支持读功能*/
        if(pSectionAttr->p_data == NULL)
            return -1;
        pdata = pSectionAttr->p_data + (regAddr - pSectionAttr->reg_begin);
        pCtx->sendLen = mb_protocol_packet03h(&pCtx->sendBuff[0], pModbusHead->SlaveAddr, pdata, regNum);
        ret = 0;
    }

    if (pCtx->sendLen > 0)
    {
        pCtx->pcombase->write(pCtx, &pCtx->sendBuff[0], pCtx->sendLen);
    }
    return ret;
}

/*********************************************************************************************************
** Function name:       modbu_msg_readreg_process
** Descriptions:        modbus 读取寄存器值消息处理函数
** input parameters:    transAddr 设备地址
**                      funcode   功能码
**                      pRecvBuff 接收到的数据
**                      recvLen   接收到的数据长度
** output parameters:   None
** Returned value:      0:正常    其他:异常
*********************************************************************************************************/
static int mb_read_process_04H(struct mb_comm_ctx_t *pCtx, uint8_t *pRecvBuff, uint16_t recvLen)
{
    int ret;
    uint16_t regAddr;
    uint8_t regNum, sectionCnt, index;
    uint8_t  *pdata_u8;
    struct remap_Modbus_node *pdata;
    uint16_t *pdata_block;
    LPModBusHead pModbusHead;
    const ST_REG_4XXXX_ATTR *pSectionAttr;
    const ST_REG_4XXXX_ATTR_BLOCK    *pSectionBlockAttr;

    pModbusHead = (LPModBusHead)pRecvBuff;

    /*
        boardcast just can be used for write function.
    */
    if (pModbusHead->SlaveAddr == MB_COM_BROADCAST_ADDR)
        return -1;

    /*
        unpacket the Message with 03H code
    */
    ret = mb_protocol_unpacket04h(pRecvBuff, recvLen, &regAddr, &regNum);
    if (ret != 0)
    {
        return -1;
    }
   
    /*
        set the register section selector
    */

    mb_config_setSectionSelector(pCtx, pModbusHead->SlaveAddr);


    /*
        find the register from the REG TABLES
    */

    sectionCnt = mb_config_getSectionCnt(pCtx);

    for (index = 0; index < sectionCnt; index++)
    {
        ret = mb_config_getSectionAttr(pCtx, index, &pSectionAttr);

        /*
                判断是否在合法边界
        */
        if ((regAddr >= pSectionAttr->reg_begin) && (regAddr + regNum <= pSectionAttr->reg_begin + pSectionAttr->reg_len) && (pSectionAttr->write_enable & MB_READ_ENABLE))
        {
            break;
        }
    }

    if (index == sectionCnt)
    {
        // //非法数据地址，返回地址错误异常码02
        // pCtx->sendLen = mb_protocol_packet_exceptmsg(&pCtx->sendBuff[0], pModbusHead->SlaveAddr, pModbusHead->Cmd, MB_ILLEGAL_DATA_ADDRESS);
        // ret = -1;

        /* 
            can not be found in const map table
            we search the re-map table;  
        */    
        sectionCnt = mb_config_getBlockSectionCnt(pCtx);
        for (index = 0; index < sectionCnt; index++)
        {
            ret = mb_config_getBlockSectionAttr(pCtx, index, &pSectionBlockAttr);
            /*
                判断是否在合法边界
            */
            if ((regAddr >= pSectionBlockAttr->reg_begin) && ((regAddr + regNum) <= (pSectionBlockAttr->reg_begin + pSectionBlockAttr->reg_len)) && (pSectionBlockAttr->write_enable & MB_READ_ENABLE))
            {
                break;
            }
        }

        if (index == sectionCnt)
        {
            //非法数据地址，返回地址错误异常码02
            pCtx->sendLen = mb_protocol_packet_exceptmsg(&pCtx->sendBuff[0], pModbusHead->SlaveAddr, pModbusHead->Cmd, MB_ILLEGAL_DATA_ADDRESS);
            ret = -1;
        }
        else
        {
            //组块报文
            pdata_block = pSectionBlockAttr->p_data->node + (regAddr - pSectionBlockAttr->reg_begin);

            pCtx->sendLen = mb_protocol_block_packet04h(&pCtx->sendBuff[0], pModbusHead->SlaveAddr, pSectionBlockAttr->p_data, pdata_block, regNum);
        }
    }    
    else
    {
        /* 03H 数据打包前的回调。一般用于触发型的数据更新 */
        if(pSectionAttr->readhandle)  
            pSectionAttr->readhandle(pCtx, regAddr, regNum, 0);
        /* MB_TYPE_CMD 只写命令不支持读功能*/
        if(pSectionAttr->p_data == NULL)
            return -1;

            pdata = pSectionAttr->p_data + (regAddr - pSectionAttr->reg_begin);

            pCtx->sendLen = mb_protocol_packet04h(&pCtx->sendBuff[0], pModbusHead->SlaveAddr, pdata, regNum);

            ret = 0;
    }

    if (pCtx->sendLen > 0)
    {
        pCtx->pcombase->write(pCtx, &pCtx->sendBuff[0], pCtx->sendLen);
    }
    return ret;
}


/*********************************************************************************************************
** Function name:       mb_process_05H
** Descriptions:        modbus 写继电器消息处理函数
** input parameters:    transAddr 设备地址
**                      pRecvBuff 接收到的数据
**                      recvLen   接收到的数据长度
** output parameters:   None
** Returned value:      0:正常    其他:异常
*********************************************************************************************************/
static int mb_process_05H(struct mb_comm_ctx_t *pCtx, uint8_t *pRecvBuff, uint16_t recvLen)
{
    LPModBusHead pModbusHead;
    uint16_t regAddr;
    uint16_t regValue;
    int ret = -1;

    pModbusHead = (LPModBusHead)pRecvBuff;
    /*
        unpacket the Message with 10H code
    */
    ret = mb_protocol_unpacket05h(pRecvBuff, recvLen, &regAddr, &regValue);

    if (ret != 0)
    {
        //格式不正确
        return -1;
    }

    /* handle the write coil */
    ret = mb_config_writecoil_handle(pCtx, regAddr, regValue);
    if(ret == -1)
    {
        pCtx->sendLen = mb_protocol_packet_exceptmsg(&pCtx->sendBuff[0], pModbusHead->SlaveAddr, pModbusHead->Cmd, MB_ILLEGAL_DATA_VALUE);
            goto __end;
    }
    else if(ret == -2)
    {
        pCtx->sendLen = mb_protocol_packet_exceptmsg(&pCtx->sendBuff[0], pModbusHead->SlaveAddr, pModbusHead->Cmd, MB_ILLEGAL_DATA_ADDRESS);
            goto __end;
    }    

    pCtx->sendLen = mb_protocol_packet05h(&pCtx->sendBuff[0], pModbusHead->SlaveAddr, regAddr, regValue);
    ret = 0;

__end:
    if ((pModbusHead->SlaveAddr != MB_COM_BROADCAST_ADDR) && (pCtx->sendLen > 0))
    {
        pCtx->pcombase->write(pCtx, &pCtx->sendBuff[0], pCtx->sendLen);
    }
    return ret;
}
/*********************************************************************************************************
** Function name:       mb_write_process_10H
** Descriptions:        modbus 写寄存器值消息处理函数
** input parameters:    transAddr 设备地址
**                      pRecvBuff 接收到的数据
**                      recvLen   接收到的数据长度
** output parameters:   None
** Returned value:      0:正常    其他:异常
*********************************************************************************************************/
static int mb_write_process_10H(struct mb_comm_ctx_t *pCtx, uint8_t *pRecvBuff, uint16_t recvLen)
{
    uint8_t regNum, realRegNum, sectionCnt, index;
    uint16_t regAddr;
    uint16_t password;
    uint8_t *pValue;
    LPModBusHead pModbusHead;
    int ret = -1,i;
    const ST_REG_4XXXX_ATTR *pSectionAttr;
    const ST_REG_4XXXX_ATTR_BLOCK *pSectionBlockAttr;

    pModbusHead = (LPModBusHead)pRecvBuff;
    /*
        unpacket the Message with 10H code
    */
    ret = mb_protocol_unpacket10h(pRecvBuff, recvLen, &regAddr, &regNum, &pValue);
    if (ret != 0)
    {
        //格式不正确
        return -1;
    }

    //对写寄存器个数为0（带密码时寄存器为0会被减为255）作处理
    if (0 == regNum)
    {
        pCtx->sendLen = mb_protocol_packet_exceptmsg(&pCtx->sendBuff[0], pModbusHead->SlaveAddr, pModbusHead->Cmd, MB_ILLEGAL_DATA_VALUE);
        ret = -1;
        goto __end;
    }

    /*
        find the register from the REG TABLES
    */
    sectionCnt = mb_config_getSectionCnt(pCtx);
    for (index = 0; index < sectionCnt; index++)
    {
        ret = mb_config_getSectionAttr(pCtx, index, &pSectionAttr);
        if (ret == 0)
        {
            //如果是带密码的，真实的写入个数减1
            realRegNum = (pSectionAttr->write_enable & MB_PASSWORD_ENABLE) ? (regNum - 1) : regNum;
            /*
                判断是否在合法边界
            */
            if ((regAddr >= pSectionAttr->reg_begin) && (regAddr + realRegNum <= pSectionAttr->reg_begin + pSectionAttr->reg_len) && (pSectionAttr->write_enable & MB_WRITE_ENABLE))
            {
                break;
            }
        }
    }
    if (index == sectionCnt)
    {
        /*
            单寄存器映射表找不到，从块映射寄存器找
        */
        sectionCnt = mb_config_getBlockSectionCnt(pCtx);
        for (index = 0; index < sectionCnt; index++)
        {
            ret = mb_config_getBlockSectionAttr(pCtx, index, &pSectionBlockAttr);
            if (ret == 0)
            {
                //如果是带密码的，真实的写入个数减1
                realRegNum = (pSectionBlockAttr->write_enable & MB_PASSWORD_ENABLE) ? (regNum - 1) : regNum;
                /*
                    判断是否在合法边界
                */
                if ((regAddr >= pSectionBlockAttr->reg_begin) && (regAddr + realRegNum <= pSectionBlockAttr->reg_begin + pSectionBlockAttr->reg_len) && (pSectionBlockAttr->write_enable & MB_WRITE_ENABLE))
                {
                    break;
                }
            }
        }
        if (index == sectionCnt)
        {
            /* 块映射 和单体映射表都找不到，处理特殊的生产测试 */
            ret = mb_process_userdef(pCtx, regAddr, regNum, pValue);   //用户自定义特殊寄存器
            if(ret == 0)
            {
                pCtx->sendLen = mb_protocol_packet10h(&pCtx->sendBuff[0], pModbusHead->SlaveAddr, regAddr, regNum);
                ret = 0;
                goto __end;
            }
            else
            {
                /* 块映射 和单体映射表都找不到，回复异常 */
                //非法数据地址，返回地址错误异常码02
                pCtx->sendLen = mb_protocol_packet_exceptmsg(&pCtx->sendBuff[0], pModbusHead->SlaveAddr, pModbusHead->Cmd, MB_ILLEGAL_DATA_ADDRESS);
                ret = -1;
                goto __end;
            }
        }
        else
        {
            if (pSectionBlockAttr->write_enable & MB_PASSWORD_ENABLE)
            {
                //需要密码
                password = MB_ALIGN_PT_16BIT_READ(pValue);
                if (password != MB_WRITE_PASSWORD)
                {
                    //密码错误，返回非法数据值异常码03
                    pCtx->sendLen = mb_protocol_packet_exceptmsg(&pCtx->sendBuff[0], pModbusHead->SlaveAddr, pModbusHead->Cmd, MB_ILLEGAL_DATA_VALUE);
                    ret = -1;
                    goto __end;
                }
            }
            //如果是带密码的，真实的数据向后挪动一个寄存器
            pValue = (pSectionBlockAttr->write_enable & MB_PASSWORD_ENABLE) ? (pValue + 2) : pValue;
            ret = mb_config_block_set(pCtx, pSectionBlockAttr, regAddr, realRegNum, pValue);
            if (ret < 0)
            {
                pCtx->sendLen = mb_protocol_packet_exceptmsg(&pCtx->sendBuff[0], pModbusHead->SlaveAddr, pModbusHead->Cmd, MB_ILLEGAL_DATA_VALUE);
                goto __end;
            }
            pCtx->sendLen = mb_protocol_packet10h(&pCtx->sendBuff[0], pModbusHead->SlaveAddr, regAddr, regNum);
            ret = 0;
        }
    }
    else
    {
        if (pSectionAttr->write_enable & MB_PASSWORD_ENABLE)
        {
            //需要密码
            password = MB_ALIGN_PT_16BIT_READ(pValue);
            if (password != MB_WRITE_PASSWORD)
            {
                //密码错误，返回非法数据值异常码03
                pCtx->sendLen = mb_protocol_packet_exceptmsg(&pCtx->sendBuff[0], pModbusHead->SlaveAddr, pModbusHead->Cmd, MB_ILLEGAL_DATA_VALUE);
                ret = -1;
                goto __end;
            }
        }
        //如果是带密码的，真实的数据向后挪动一个寄存器
        pValue = (pSectionAttr->write_enable & MB_PASSWORD_ENABLE) ? (pValue + 2) : pValue;
        ret = mb_config_set(pCtx, pSectionAttr, regAddr, realRegNum, pValue);
        if (ret < 0)
        {
            pCtx->sendLen = mb_protocol_packet_exceptmsg(&pCtx->sendBuff[0], pModbusHead->SlaveAddr, pModbusHead->Cmd, MB_ILLEGAL_DATA_VALUE);
            goto __end;
        }
        pCtx->sendLen = mb_protocol_packet10h(&pCtx->sendBuff[0], pModbusHead->SlaveAddr, regAddr, regNum);
        ret = 0;
    }
__end:
    if ((pModbusHead->SlaveAddr != MB_COM_BROADCAST_ADDR) && (pCtx->sendLen > 0))
    {
        pCtx->pcombase->write(pCtx, &pCtx->sendBuff[0], pCtx->sendLen);
    }
    return ret;
}

/*********************************************************************************************************
** Function name:       mb_write_process_06H
** Descriptions:        modbus 写寄存器值消息处理函数
** input parameters:    transAddr 设备地址
**                      pRecvBuff 接收到的数据
**                      recvLen   接收到的数据长度
** output parameters:   None
** Returned value:      0:正常    其他:异常
*********************************************************************************************************/
static int mb_write_process_06H(struct mb_comm_ctx_t *pCtx, uint8_t *pRecvBuff, uint16_t recvLen)
{
    uint8_t regNum, realRegNum, sectionCnt, index;
    uint16_t regAddr;
    uint16_t password;
    uint8_t *pValue;
    LPModBusHead pModbusHead;
    int ret = -1,i;
    const ST_REG_4XXXX_ATTR *pSectionAttr;

    pModbusHead = (LPModBusHead)pRecvBuff;
    /*
        unpacket the Message with 10H code
    */

    ret = mb_protocol_unpacket06h(pRecvBuff, recvLen, &regAddr, &regNum, &pValue);

    if (ret != 0)
    {
        //格式不正确
        return -1;
    }
    /*
        find the register from the REG TABLES
    */
    sectionCnt = mb_config_getSectionCnt(pCtx);
    for (index = 0; index < sectionCnt; index++)
    {
        ret = mb_config_getSectionAttr(pCtx, index, &pSectionAttr);
        if (ret == 0)
        {
            //如果是带密码的，真实的写入个数减1
            realRegNum = (pSectionAttr->write_enable & MB_PASSWORD_ENABLE) ? (regNum - 1) : regNum;
            /*
                判断是否在合法边界
            */
            if ((regAddr >= pSectionAttr->reg_begin) && (regAddr + realRegNum <= pSectionAttr->reg_begin + pSectionAttr->reg_len) && (pSectionAttr->write_enable & MB_WRITE_ENABLE))
            {
                break;
            }
        }
    }
    if (index == sectionCnt)
    {
        ret = mb_process_userdef(pCtx, regAddr, regNum, pValue);   //用户自定义特殊寄存器
        if(ret == 0)
        {
            pCtx->sendLen = mb_protocol_packet06h(&pCtx->sendBuff[0], pModbusHead->SlaveAddr, regAddr, regNum);
            ret = 0;
            goto __end;
        }
        else
        {
            //非法数据地址，返回地址错误异常码02
            pCtx->sendLen = mb_protocol_packet_exceptmsg(&pCtx->sendBuff[0], pModbusHead->SlaveAddr, pModbusHead->Cmd, MB_ILLEGAL_DATA_ADDRESS);
            ret = -1;
            goto __end;
        }
    }
    else
    {
        if (pSectionAttr->write_enable & MB_PASSWORD_ENABLE)
        {
            //需要密码
            password = MB_ALIGN_PT_16BIT_READ(pValue);
            if (password != MB_WRITE_PASSWORD)
            {
                //密码错误，返回非法数据值异常码03
                pCtx->sendLen = mb_protocol_packet_exceptmsg(&pCtx->sendBuff[0], pModbusHead->SlaveAddr, pModbusHead->Cmd, MB_ILLEGAL_DATA_VALUE);
                ret = -1;
                goto __end;
            }
        }
        //如果是带密码的，真实的数据向后挪动一个寄存器
        pValue = (pSectionAttr->write_enable & MB_PASSWORD_ENABLE) ? (pValue + 2) : pValue;
        ret = mb_config_set(pCtx, pSectionAttr, regAddr, realRegNum, pValue);
        if (ret < 0)
        {
            pCtx->sendLen = mb_protocol_packet_exceptmsg(&pCtx->sendBuff[0], pModbusHead->SlaveAddr, pModbusHead->Cmd, MB_ILLEGAL_DATA_VALUE);
            goto __end;
        }
        pCtx->sendLen = mb_protocol_packet06h(&pCtx->sendBuff[0], pModbusHead->SlaveAddr, regAddr, regNum);
        ret = 0;
    }
__end:
    if ((pModbusHead->SlaveAddr != MB_COM_BROADCAST_ADDR) && (pCtx->sendLen > 0))
    {
        pCtx->pcombase->write(pCtx, &pCtx->sendBuff[0], pCtx->sendLen);
    }
    return ret;
}


/*********************************************************************************************************
** Function name:       mb_process_44H
** Descriptions:        modbus 写寄存器值消息处理函数
** input parameters:    transAddr 设备地址
**                      pRecvBuff 接收到的数据
**                      recvLen   接收到的数据长度
** output parameters:   None
** Returned value:      0:正常    其他:异常
*********************************************************************************************************/
static int mb_process_44H(struct mb_comm_ctx_t *pCtx, uint8_t *pRecvBuff, uint16_t recvLen)
{
    LPModBusHead pModbusHead;
    int ret = -1;

    pModbusHead = (LPModBusHead)pRecvBuff;
    /*
        unpacket the Message with 10H code
    */
    ret = mb_protocol_unpacket44h(pRecvBuff, recvLen);
    if (ret != 0)
    {
        //格式不正确
        return -1;
    }
    pCtx->sendLen = mb_protocol_packet44h(&pCtx->sendBuff[0], pModbusHead->SlaveAddr);
    ret = 0;

__end:
    if ((pModbusHead->SlaveAddr != MB_COM_BROADCAST_ADDR) && (pCtx->sendLen > 0))
    {
        pCtx->pcombase->write(pCtx, &pCtx->sendBuff[0], pCtx->sendLen);
    }
    return ret;
}

/*********************************************************************************************************
** Function name:       mb_process_45H
** Descriptions:        modbus 写寄存器值消息处理函数
** input parameters:    transAddr 设备地址
**                      pRecvBuff 接收到的数据
**                      recvLen   接收到的数据长度
** output parameters:   None
** Returned value:      0:正常    其他:异常
*********************************************************************************************************/
static int mb_process_45H(struct mb_comm_ctx_t *pCtx, uint8_t *pRecvBuff, uint16_t recvLen)
{
    uint8_t address, request_id;
    LPModBusHead pModbusHead;
    int ret = -1;

    pModbusHead = (LPModBusHead)pRecvBuff;
    /*
        unpacket the Message with 10H code
    */
    ret = mb_protocol_unpacket45h(pRecvBuff, recvLen, &request_id);
    if (ret != 0)
    {
        //格式不正确或无需回复
        return -1;
    }
    pCtx->sendLen = mb_protocol_packet45h(&pCtx->sendBuff[0], pModbusHead->SlaveAddr);
    ret = 0;

__end:
    address = pCtx->pvmem->db_measurebase[0].db_cfg.user.base.Addr.value;
    if (((pModbusHead->SlaveAddr != MB_COM_BROADCAST_ADDR) && (pCtx->sendLen > 0)) ||
        ((pModbusHead->SlaveAddr == MB_COM_BROADCAST_ADDR) && (pCtx->sendLen > 0) && (request_id == address)))
    {
        pCtx->pcombase->write(pCtx, &pCtx->sendBuff[0], pCtx->sendLen);
    }
    return ret;
}


/*********************************************************************************************************
** Function name:       mb_process_46H
** Descriptions:        modbus 写寄存器值消息处理函数
** input parameters:    transAddr 设备地址
**                      pRecvBuff 接收到的数据
**                      recvLen   接收到的数据长度
** output parameters:   None
** Returned value:      0:正常    其他:异常
*********************************************************************************************************/
static int mb_process_46H(struct mb_comm_ctx_t *pCtx, uint8_t *pRecvBuff, uint16_t recvLen)
{
    LPModBusHead pModbusHead;
    int ret = -1;
    
    pModbusHead = (LPModBusHead)pRecvBuff;
    /*
        unpacket the Message with 10H code
    */
    ret = mb_protocol_unpacket46h(pRecvBuff, recvLen);

    if (ret != 0)
    {
        //格式不正确
        return -1;
    }

    pCtx->sendLen = mb_protocol_packet46h(&pCtx->sendBuff[0], pModbusHead->SlaveAddr);
    ret = 0;
    
__end:
    if ((pModbusHead->SlaveAddr != MB_COM_BROADCAST_ADDR) && (pCtx->sendLen > 0))
    {
        pCtx->pcombase->write(pCtx, &pCtx->sendBuff[0], pCtx->sendLen);
    }
    return ret;
}

/*********************************************************************************************************
** Function name:       mb_process_47H
** Descriptions:        modbus 写寄存器值消息处理函数
** input parameters:    transAddr 设备地址
**                      pRecvBuff 接收到的数据
**                      recvLen   接收到的数据长度
** output parameters:   None
** Returned value:      0:正常    其他:异常
*********************************************************************************************************/
static int mb_process_47H(struct mb_comm_ctx_t *pCtx, uint8_t *pRecvBuff, uint16_t recvLen)
{
    uint8_t address, request_id;
    LPModBusHead pModbusHead;
    int ret = -1;

    pModbusHead = (LPModBusHead)pRecvBuff;
    /*
        unpacket the Message with 10H code
    */
    ret = mb_protocol_unpacket47h(pRecvBuff, recvLen, &request_id);

    if (ret == -1)
    {
        //格式不正确或无需回复
        return -1;
    }
    pCtx->sendLen = mb_protocol_packet47h(&pCtx->sendBuff[0], pModbusHead->SlaveAddr);

    if(ret == 1)
    {
        mb_file_update();    
    }
    ret = 0;  

__end:
    address = pCtx->pvmem->db_measurebase[0].db_cfg.user.base.Addr.value;
    if (((pModbusHead->SlaveAddr != MB_COM_BROADCAST_ADDR) && (pCtx->sendLen > 0)) ||
        ((pModbusHead->SlaveAddr == MB_COM_BROADCAST_ADDR) && (pCtx->sendLen > 0) && (request_id == address)))
    {
        pCtx->pcombase->write(pCtx, &pCtx->sendBuff[0], pCtx->sendLen);
    }
    return ret;
}


/*********************************************************************************************************
** Function name:       mb_process_48H
** Descriptions:        modbus 写寄存器值消息处理函数
** input parameters:    transAddr 设备地址
**                      pRecvBuff 接收到的数据
**                      recvLen   接收到的数据长度
** output parameters:   None
** Returned value:      0:正常    其他:异常
*********************************************************************************************************/
static int mb_process_48H(struct mb_comm_ctx_t *pCtx, uint8_t *pRecvBuff, uint16_t recvLen)
{
    uint8_t address;
    LPModBusHead pModbusHead;
    int ret = -1;

    pModbusHead = (LPModBusHead)pRecvBuff;
    /*
        unpacket the Message with 10H code
    */
    ret = mb_protocol_unpacket48h(pRecvBuff, recvLen);
    if (ret != 0)
    {
        //格式不正确或无需回复
        return -1;
    }
    pCtx->sendLen = mb_protocol_packet48h(&pCtx->sendBuff[0], pModbusHead->SlaveAddr);
    ret = 0;

__end:
    address = pCtx->pvmem->db_measurebase[0].db_cfg.user.base.Addr.value;
    if (((pModbusHead->SlaveAddr != MB_COM_BROADCAST_ADDR) && (pCtx->sendLen > 0)) ||
        ((pModbusHead->SlaveAddr == MB_COM_BROADCAST_ADDR) && (pCtx->sendLen > 0)))
    {
        pCtx->pcombase->write(pCtx, &pCtx->sendBuff[0], pCtx->sendLen);
    }
    return ret;
}
/*********************************************************************************************************
** Function name:       mb_process_49H
** Descriptions:        modbus 写寄存器值消息处理函数
** input parameters:    transAddr 设备地址
**                      pRecvBuff 接收到的数据
**                      recvLen   接收到的数据长度
** output parameters:   None
** Returned value:      0:正常    其他:异常
*********************************************************************************************************/
static int mb_process_49H(struct mb_comm_ctx_t *pCtx, uint8_t *pRecvBuff, uint16_t recvLen)
{
    uint8_t address, request_id;
    LPModBusHead pModbusHead;
    int ret = -1,readlen = -1,actlen = -1;
    char pMsgBuff[1024] = {0};

    pModbusHead = (LPModBusHead)pRecvBuff;
    /*
        unpacket the Message with 10H code
    */
    readlen = mb_protocol_unpacket49h(pRecvBuff, recvLen, &request_id);
    if (readlen < 0)
    {
        //格式不正确或无需回复
        return -1;
    }

    actlen = log_read(&pMsgBuff[0],readlen);
    pCtx->sendLen = mb_protocol_packet49h(&pCtx->sendBuff[0], pModbusHead->SlaveAddr,&pMsgBuff[0],actlen);

    ret = 0;

__end:

    address = pCtx->pvmem->db_measurebase[0].db_cfg.user.base.Addr.value;
    if (((pModbusHead->SlaveAddr != MB_COM_BROADCAST_ADDR) && (pCtx->sendLen > 0)) ||
        ((pModbusHead->SlaveAddr == MB_COM_BROADCAST_ADDR) && (pCtx->sendLen > 0) && (request_id == address)))
    {
        pCtx->pcombase->write(pCtx, &pCtx->sendBuff[0], pCtx->sendLen);
    }
;
    return ret;
}
/*********************************************************************************************************
** Function name:       mb_process_4AH
** Descriptions:        modbus 写寄存器值消息处理函数
** input parameters:    transAddr 设备地址
**                      pRecvBuff 接收到的数据
**                      recvLen   接收到的数据长度
** output parameters:   None
** Returned value:      0:正常    其他:异常
*********************************************************************************************************/
static int mb_process_4AH(struct mb_comm_ctx_t *pCtx, uint8_t *pRecvBuff, uint16_t recvLen)
{
    uint8_t address, request_id;
    LPModBusReadREG4AH pModbusHead;
    int                   ret = -1;
    int                   rat;
    char uninstall[96];
    char buf[256];
    int                   result = 1;
    pModbusHead = (LPModBusReadREG4AH)pRecvBuff;

    ret = mb_protocol_unpacket4Ah(pRecvBuff, recvLen);
    if (ret < 0)
    {
        //格式不正确或无需回复
        return -1;
    }

    snprintf(uninstall, sizeof(uninstall) - 1, "opkg remove %s", pModbusHead->app_name);
    rat = executeCMD(uninstall,buf,sizeof(buf));
    if(rat < 0)
    {
        result = 0;
    }
    else
    {
        if(strstr(buf,"No"))
        {
            result = 0;
        }
        else
        {
            result = 1;
        }
    }

    pCtx->sendLen = mb_protocol_packet4Ah(&pCtx->sendBuff[0],pModbusHead->SlaveAddr, result);
    pCtx->pcombase->write(pCtx, &pCtx->sendBuff[0], pCtx->sendLen);

    return ret;
}

/*********************************************************************************************************
** Function name:       mb_except_msg_process
** Descriptions:        modbus 异常消息处理函数
** input parameters:    transAddr 设备地址
**                      pRecvBuff 接收到的数据
**                      recvLen   接收到的数据长度
** output parameters:   None
** Returned value:      0:正常    其他:异常
*********************************************************************************************************/
static int mb_except_msg_process(struct mb_comm_ctx_t *pCtx, uint8_t *pRecvBuff, uint16_t recvLen)
{
    int ret;
    LPModBusHead pModbusHead;

    pModbusHead = (LPModBusHead)pRecvBuff;

    /*
        boardcast just can be used for write function.
    */
    if (pModbusHead->SlaveAddr == MB_COM_BROADCAST_ADDR)
        return -1;

    ret = mb_protocol_crc_check(pRecvBuff, recvLen);
    if (0 != ret)
    {
        //CRC也校验错误，丢掉不处理
        return -1;
    }
    else
    {
        //功能码不支持，但是CRC校验正确，回复错误码-不支持的功能码 1
        pCtx->sendLen = mb_protocol_packet_exceptmsg(&pCtx->sendBuff[0], pModbusHead->SlaveAddr, pModbusHead->Cmd, MB_ILLEGAL_FUNCTION);
        if (pCtx->sendLen > 0)
        {
            pCtx->pcombase->write(pCtx, &pCtx->sendBuff[0], pCtx->sendLen);
        }
        return 0;
    }
}

/*********************************************************************************************************
** Function name:       mb_msg_process
** Descriptions:        modbus 消息处理函数
** input parameters:    pRecvBuff 接收到的数据
**                      recvLen   接收到的数据长度
** output parameters:   None
** Returned value:      0:正常    其他:异常
*********************************************************************************************************/
int mb_msg_process_rtu(struct mb_comm_ctx_t *pCtx, uint8_t *pRecvBuff, uint16_t recvLen)
{
    int ret = -1;
    LPModBusHead pModbusHead;

    pModbusHead = (LPModBusHead)pRecvBuff;
    /* */
    if (0 != mb_addr_check(pCtx, pModbusHead->SlaveAddr))
    {
        return -1;
    }

    switch (pModbusHead->Cmd)
    {
    case _FC_READ_HOLDING_REGISTERS:
        ret = mb_read_process_03H(pCtx, pRecvBuff, recvLen);
        break;
    case _FC_READ_INPUT_REGISTERS:
        ret = mb_read_process_04H(pCtx, pRecvBuff, recvLen);
        break;        
    case _FC_WRITE_COIL_REGISTERS:
        ret = mb_process_05H(pCtx, pRecvBuff, recvLen);
        break;
    case _FC_WRITE_MULTIPLE_REGISTERS:
        ret = mb_write_process_10H(pCtx, pRecvBuff, recvLen);
        break;
    case _FC_WRITE_SINGLE_REGISTERS:
        ret = mb_write_process_06H(pCtx, pRecvBuff, recvLen);
        break;    
    case _FC_GET_FILE_INFO:
        ret = mb_process_44H(pCtx, pRecvBuff, recvLen);
        break;
    case _FC_SND_FILE_PKG:
        ret = mb_process_45H(pCtx, pRecvBuff, recvLen);
        break;
    case _FC_GET_FILE_HANDLE:
        ret = mb_process_46H(pCtx, pRecvBuff, recvLen);
        break;
    case _FC_SND_FILE_HANDLE:
        ret = mb_process_47H(pCtx, pRecvBuff, recvLen);
        break;    
    case _FC_SHELL_COMMAND:
        ret = mb_process_48H(pCtx, pRecvBuff, recvLen);
        break; 
    case _FC_DEBUG_MESSAGE:
        ret = mb_process_49H(pCtx, pRecvBuff, recvLen);
        break; 
    case _FC_UNINSTALL_APP:
        ret = mb_process_4AH(pCtx, pRecvBuff, recvLen);
        break; 
    default:
        ret = mb_except_msg_process(pCtx, pRecvBuff, recvLen);
        break;
    }
    return ret;
}
