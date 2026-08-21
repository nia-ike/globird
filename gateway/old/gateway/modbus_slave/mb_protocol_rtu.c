/*
* Copyright (c) 
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

#include "list.h"
#include "db_struct.h"

#include "mb_protocol.h"
#include "mb_cfg.h"
#include "mb_com_baseclass.h"
#include "mb_datatype.h"
#include "mb_updatefile.h"
#include "mb_configfile.h"
#include "mb_datacache_file.h"
#include "mb_edge_script_file.h"
#include "mb_edge_alert_query.h"
#include "mb_diagnose.h"
#include "crc16.h"

#include "dev_log.h"

#include "db_node_fops.h"

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

typedef struct tagGetFile4EH
{
    uint8_t     SlaveAddr;
    uint8_t     Cmd;
    uint8_t     devtype;
    uint32_t    filesize;
    uint16_t    filecrc;
    uint32_t    curoffset;
    uint8_t     request_id;
    uint16_t    packsize;
}ModBusGetFile4EH, *LPModBusGetFile4EH;

typedef struct tagGetFile4EH_ack
{
    uint8_t     SlaveAddr;
    uint8_t     Cmd;
    uint8_t     devtype;
    uint32_t    filesize;
    uint16_t    filecrc;
    uint32_t    curoffset;
    uint8_t     id;
    uint16_t    packsize;
    uint8_t     reserved[2];
}ModBusGetFile4EH_ack, *LPModBusGetFile4EH_ack;

typedef struct tagSndFile4DH
{
    uint8_t     SlaveAddr;
    uint8_t     Cmd;
    uint8_t     devtype;
    uint32_t    filesize;
    uint16_t    filecrc;
    uint32_t    curoffset;
    uint8_t     request_id;
    uint16_t    packsize;
}ModBusSndFile4DH, *LPModBusSndFile4DH;

typedef struct tagSndFile4DH_ack
{
    uint8_t     SlaveAddr;
    uint8_t     Cmd;
    uint8_t     devtype;
    uint32_t    filesize;
    uint16_t    filecrc;
    uint32_t    curoffset;
    uint8_t     id;
    uint8_t     reserved[4];
}ModBusSndFile4DH_ack, *LPModBusSndFile4DH_ack;

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
    char        app_name[32];    // app名
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

static int executeCMD(const char *cmd, char *result, int len)
{
    FILE *ptr;
    int ret = -1;

    // Check input parameters
    if (!cmd || !result || len <= 0) {
        return -1;
    }

    // Clear the result buffer
    memset((void*)result, 0, len);
    
    if ((ptr = popen(cmd, "r")) != NULL) {
        // Read command output, ensuring we don't overflow the buffer
        if (fgets(result, len, ptr) != NULL) {
            // Ensure null termination
            result[len - 1] = '\0';
            ret = 0;  // Success
        }
        
        pclose(ptr);
        ptr = NULL;
    }

    return ret;
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
static int mb_protocol_packet03h(struct mb_comm_ctx_t *pCtx, uint8_t devAddr, uint16_t regNo, uint8_t regNum)
{
    int                             i, j, index, n, m, ret;
    uint16_t                        crc;
    uint8_t                         pkgsize;
    uint32_t                         type;
    uint16_t                        *pValue;
    double                          f64val;
    int64_t                         i64val;
    uint16_t                        regValue;
    struct config_chnl_node_attr    *pnode_attr; 
    struct config_chnl_working_attr *pCfg_chnlattr = container_of(pCtx, struct config_chnl_working_attr,  mb_comm_ctx);

    LPModBusReadREG03H_ack      pReadReg;

    pReadReg                = (LPModBusReadREG03H_ack)(pCtx->sendBuff);

    pReadReg->SlaveAddr = devAddr;
    pReadReg->Cmd       = _FC_READ_HOLDING_REGISTERS;
    pReadReg->BytesNum  = 0;

    for(i=0; i<regNum;)
    {
        /* 在转发映射表中检索到 寄存器号是 regNo 的点属性 */
        index = mb_get_nodeattr_by_regno(pCfg_chnlattr->node_attr, regNo, 0, pCfg_chnlattr->node_num);
        if(index < 0)
            goto _error;

        pnode_attr = &pCfg_chnlattr->node_attr[index];

        /* 获取当前转发点属性中，从数据类型获取占用的寄存器号 */
        n = Modbus_DataTypeToRegNum(pnode_attr->datatype);
        if(n < 0)
            goto _error;

        for(j=0; j<n; j++)
        {
            /* 指向03H响应报文的数据payload起始地址 */
            pValue = (uint16_t *)(pCtx->sendBuff + sizeof(ModBusReadREG03H_ack) + pReadReg->BytesNum);

            /**/
            if(DB_CONVENT_ID_2_CHNLNO(pnode_attr->db_link) != 193)
            {
                type = DB_CONVENT_ID_2_TYPE(pnode_attr->db_link);
                if((type == DB_TYPE_AI) || (type == DB_TYPE_DI) || (type == DB_TYPE_ACC))
                {
                    ret = DataMgmtFops.readpoint_by_id(pnode_attr->db_link, &f64val);
                    if(ret < 0)
                        goto _error;
                }
                else 
                {
                    // DataMgmtFops 中并无 特定 DO AO的数据点单元，因此无需从DataMgmtFops读点
                    f64val = 0;
                }

                m = Modbus_DoubleToReg16(pnode_attr->datatype, j, &regValue, &f64val);
                if(m < 0)
                    goto _error;
            }
            else 
            {
                type = DB_CONVENT_ID_2_TYPE(pnode_attr->db_link);
                if((type == DB_TYPE_AI) || (type == DB_TYPE_DI) || (type == DB_TYPE_ACC))
                {
                    ret = DataMgmtFops.readpoint_int64_by_id(pnode_attr->db_link, &i64val);
                    if(ret < 0)
                        goto _error;
                }
                else 
                {
                    // DataMgmtFops 中并无 特定 DO AO的数据点单元，因此无需从DataMgmtFops读点
                    f64val = 0;
                }

                f64val = i64val;
                m = Modbus_DoubleToReg16(pnode_attr->datatype, j, &regValue, &f64val);
                if(m < 0)
                    goto _error;
            }

            /* 写入发送报文 */
            MB_ALIGN_PT_16BIT_WRITE(pValue, regValue);

            regNo += 1;
            i += 1;
            pReadReg->BytesNum += 2;
        }
    }

    pkgsize = sizeof(ModBusReadREG03H_ack) + pReadReg->BytesNum;

    crc = crc16(&pReadReg->SlaveAddr, pkgsize);
    crc = SWAP_16BIT(crc); // CRC byte order is little endian
    MB_ALIGN_PT_16BIT_WRITE(pCtx->sendBuff + pkgsize, crc);

    return pkgsize + CRC16_SIZE;

_error:
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
static uint8_t mb_protocol_packet10h(struct mb_comm_ctx_t *pCtx, uint8_t *pBuff, uint8_t devAddr, uint16_t regAddr, uint8_t regNum, uint16_t *pValue)
{
    int                             index, n;
    uint8_t                         chnlno;
    uint8_t                         pkgsize;
    double                          f64val;
    struct config_chnl_node_attr    *pnode_attr; 
    struct config_chnl_working_attr *pCfg_chnlattr = container_of(pCtx, struct config_chnl_working_attr,  mb_comm_ctx);
    
    uint16_t            crc;
    LPModBusWriteREG10H_ack  pWriteReg;

    pWriteReg = (LPModBusWriteREG10H_ack)pBuff;

    // to do the write operation
    struct db_spl_channel_ctr_t       *pctrl;
    struct db_spl_realtime_data_tbl_t *pDB = db_DataTblObj(NULL);

    // 仅支持 1 到 2个寄存器
    if(regNum > 2) 
        goto _error;

    //----
    /* 在转发映射表中检索到 寄存器号是 regAddr 的点属性 */
    index = mb_get_nodeattr_by_regno(pCfg_chnlattr->node_attr, regAddr, 0, pCfg_chnlattr->node_num);
    if(index < 0)
        goto _error;    

    pnode_attr = &pCfg_chnlattr->node_attr[index];

    /* 获取当前转发点属性中，根据数据类型 把寄存器的值转换成 需要类型的下发值 */
    chnlno = DB_CONVENT_ID_2_CHNLNO(pnode_attr->db_link) & 0x3f;

    /* 获取空闲控制信息块（主池满后用 ctrl_extend_region） */
    pctrl = db_ctrl_alloc_idle(pDB, chnlno);
    if (pctrl == NULL)
        goto _error;

    if(pnode_attr->datattr == 0) // AI。 转发到写命令，就是06H  10H
    {
        if(regNum == 2)
        {
            n = Modbus_RegDatatoDouble(pnode_attr->datatype, 0, pValue, &f64val);
            if(n < 0)
                goto _error;

            pctrl->funcode = _FC_WRITE_MULTIPLE_REGISTERS;
        } 
        else 
        {
            Modbus_RegDatatoDouble(DATATYPE_DATA_U16_AB, 0, pValue, &f64val);
            pctrl->funcode = _FC_WRITE_SINGLE_REGISTERS;
        }
    }
    else if((pnode_attr->datattr == 1) || (pnode_attr->datattr == 3))// DI , 转发到写命令，就是 05H
    {
        n = Modbus_RegDatatoDouble(pnode_attr->datatype, 0, pValue, &f64val);
            if(n < 0)
                goto _error;

        pctrl->funcode = _FC_WRITE_COIL_REGISTERS;
    }
    

    pctrl->is_active = 1;
    pctrl->db_linkid = pnode_attr->db_link;
    pctrl->value     = f64val;
    // endof 
    
    pWriteReg->SlaveAddr = devAddr;
    pWriteReg->Cmd       = _FC_WRITE_MULTIPLE_REGISTERS;
    pWriteReg->wRegAddr  = SWAP_16BIT(regAddr);
    pWriteReg->wRegNum   = SWAP_16BIT(regNum);

    crc = crc16(&pWriteReg->SlaveAddr, sizeof(ModBusWriteREG10H_ack));
    crc = SWAP_16BIT(crc); // CRC byte order is little endian
    MB_ALIGN_PT_16BIT_WRITE(pBuff + sizeof(ModBusWriteREG10H_ack), crc);

    return sizeof(ModBusWriteREG10H_ack)+ CRC16_SIZE;

_error:
    return -1;
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
    struct mb_config_ctx_t *pConfig_ctx;
    uint8_t                 devtype = mb_file46_get_devtype();

    if (devtype == MB_FILE_DATACACHE_TYPE)
    {
        pConfig_ctx = mb_datacache_file_getctx();
        pConfig_ctx->filesize = create_down_datacache_file(pConfig_ctx, 0);
    }
    else
    {
        pConfig_ctx = mb_file_config_getctx();
        pConfig_ctx->filesize = create_downconfig_file(pConfig_ctx, 0);
        devtype = MB_FILE_DEV_TYPE;
    }
    pGetFile = (LPModBusGetFile46H_ack)pBuff;
    pGetFile->SlaveAddr = devAddr;
    pGetFile->Cmd       = _FC_GET_FILE_HANDLE;
    pGetFile->devtype   = devtype;
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
            pGetFile->packsize = (devtype == MB_FILE_DATACACHE_TYPE) ? MB_DATACACHE_PACKSIZE_DEF : 512;
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

    /* Hard cap: never write past sendBuff (MB_COM_TXBUFF_SIZE). */
    {
        uint16_t max_payload = (uint16_t)(MB_COM_TXBUFF_SIZE
            - sizeof(ModBusGetFile46H_ack)
            - CRC16_SIZE);
        if (devtype == MB_FILE_DATACACHE_TYPE && max_payload > MB_DATACACHE_PACKSIZE_MAX)
            max_payload = MB_DATACACHE_PACKSIZE_MAX;
        if (pGetFile->packsize > max_payload)
            pGetFile->packsize = max_payload;
    }

    if (devtype == MB_FILE_DATACACHE_TYPE)
        mb_datacache_file_upload(pConfig_ctx, &pGetFile->SlaveAddr + sizeof(ModBusGetFile46H_ack), pGetFile->packsize);
    else
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
        if (devtype == MB_FILE_DATACACHE_TYPE)
            unlink(MB_DATACACHE_DL);
        else
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

static uint16_t mb_protocol_packet4eh(uint8_t *pBuff, uint8_t devAddr)
{
    uint16_t                crc;
    LPModBusGetFile4EH_ack  pGetFile;
    struct mb_config_ctx_t *pConfig_ctx = mb_edge_script_getctx();

    pConfig_ctx->filesize = mb_edge_script_create_download_file(pConfig_ctx);
    pGetFile = (LPModBusGetFile4EH_ack)pBuff;
    pGetFile->SlaveAddr = devAddr;
    pGetFile->Cmd       = _FC_GET_EDGE_SCRIPT_HANDLE;
    pGetFile->devtype   = MB_FILE_DEV_TYPE;
    pGetFile->filesize  = pConfig_ctx->filesize;
    pGetFile->curoffset = pConfig_ctx->curoffset;
    if(pConfig_ctx->packsize == 0)
    {
        if(pConfig_ctx->filesize - pConfig_ctx->curoffset < 1024)
            pGetFile->packsize = pConfig_ctx->filesize - pConfig_ctx->curoffset;
        else
            pGetFile->packsize = 512;
    }
    else
    {
        if(pConfig_ctx->curoffset + pConfig_ctx->packsize > pConfig_ctx->filesize)
        {
            if(pConfig_ctx->curoffset < pConfig_ctx->filesize)
                pGetFile->packsize = pConfig_ctx->filesize - pConfig_ctx->curoffset;
            else
                pGetFile->packsize = 0;
        }
        else
            pGetFile->packsize = pConfig_ctx->packsize;
    }
    mb_edge_script_file_upload(pConfig_ctx, &pGetFile->SlaveAddr + sizeof(ModBusGetFile4EH_ack), pGetFile->packsize);

    crc = crc16(&pGetFile->SlaveAddr + sizeof(ModBusGetFile4EH_ack), pGetFile->packsize);
    pGetFile->filecrc = SWAP_16BIT(crc);
    pGetFile->id        = devAddr;
    crc = crc16(&pGetFile->SlaveAddr, pGetFile->packsize + sizeof(ModBusGetFile4EH_ack));
    crc = SWAP_16BIT(crc);
    MB_ALIGN_PT_16BIT_WRITE(pBuff + sizeof(ModBusGetFile4EH_ack) + pGetFile->packsize, crc);

    if((pConfig_ctx->curoffset + pGetFile->packsize) >= (pConfig_ctx->filesize))
        unlink(MB_EDGE_SCRIPT_FILE);

    return sizeof(ModBusGetFile4EH_ack) + CRC16_SIZE + pGetFile->packsize;
}

static uint8_t mb_protocol_packet4dh(uint8_t *pBuff, uint8_t devAddr)
{
    uint16_t                crc;
    LPModBusSndFile4DH_ack  pSndFile;
    struct mb_config_ctx_t *pConfig_ctx = mb_edge_script_getctx();

    pSndFile = (LPModBusSndFile4DH_ack)pBuff;
    pSndFile->SlaveAddr = devAddr;
    pSndFile->Cmd       = _FC_SND_EDGE_SCRIPT_HANDLE;
    pSndFile->devtype   = MB_FILE_DEV_TYPE;
    pSndFile->filesize  = pConfig_ctx->filesize;
    pSndFile->filecrc   = pConfig_ctx->filecrc;
    pSndFile->curoffset = pConfig_ctx->curoffset;
    pSndFile->id        = devAddr;

    crc = crc16(&pSndFile->SlaveAddr, sizeof(ModBusSndFile4DH_ack));
    crc = SWAP_16BIT(crc);
    MB_ALIGN_PT_16BIT_WRITE(pBuff + sizeof(ModBusSndFile4DH_ack), crc);

    return sizeof(ModBusSndFile4DH_ack) + CRC16_SIZE;
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
static uint16_t mb_protocol_packet49h(uint8_t *pBuff, uint8_t devAddr,char *MsgBuff, int ReadLen)
{
    uint16_t            crc;
    LPModBusDebugMsg49H_ack  pDebugMsg;

    pDebugMsg = (LPModBusDebugMsg49H_ack)pBuff;
    pDebugMsg->SlaveAddr = devAddr;
    pDebugMsg->Cmd       = _FC_DEBUG_MESSAGE;
    pDebugMsg->UnreadLen = log_get_unread_length();
    pDebugMsg->Reserved  = 0;
    pDebugMsg->DataLen   = ReadLen;
    
    if(pDebugMsg->DataLen > (MB_COM_RXBUFF_SIZE - sizeof(ModBusDebugMsg49H_ack)))
    {
        pDebugMsg->DataLen = (MB_COM_RXBUFF_SIZE - sizeof(ModBusDebugMsg49H_ack));
    }
    else 
    {
        pDebugMsg->DataLen = pDebugMsg->DataLen;
    }
    memcpy((pBuff + sizeof(ModBusDebugMsg49H_ack)), MsgBuff, pDebugMsg->DataLen);

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
static int mb_protocol_unpacket10h(uint8_t *pBuff, uint16_t  bufflen, uint16_t *pRegAddr, uint8_t *pRegNum, uint16_t *pValue)
{
    int                         i;
    uint16_t                    val;
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
        for(i=0;i<(*pRegNum); i++)
        {
            val = *(uint16_t *)(&pWriteReg->SlaveAddr + sizeof(ModBusWriteREG10H) + i * 2);
            pValue[i] = SWAP_16BIT(val);

        }
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

    pkgsize = sizeof(ModBusGetFile46H);
    wCalCrc = crc16(&pGetFile->SlaveAddr, pkgsize);
    wTpCrc  = MB_ALIGN_PT_16BIT_READ(&pGetFile->SlaveAddr + pkgsize);
    wTpCrc = SWAP_16BIT(wTpCrc);

    if(wTpCrc == wCalCrc)
    {
        config_ctx.curoffset = pGetFile->curoffset;
        config_ctx.packsize = SWAP_16BIT(MB_ALIGN_PT_16BIT_READ(&pGetFile->packsize));
        if(config_ctx.filesize <= 0)
            config_ctx.filesize = pGetFile->filesize;

        if (pGetFile->devtype == MB_FILE_DATACACHE_TYPE)
        {
            if (config_ctx.packsize > MB_DATACACHE_PACKSIZE_MAX)
                return -1;
            mb_file46_set_devtype(MB_FILE_DATACACHE_TYPE);
            mb_datacache_file_config(&config_ctx);
        }
        else if (pGetFile->devtype == MB_FILE_DEV_TYPE)
        {
            mb_file46_set_devtype(MB_FILE_DEV_TYPE);
            mb_file_config(&config_ctx);
        }
        else
        {
            return -1;
        }

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

static int mb_protocol_unpacket4eh(uint8_t *pBuff, uint16_t bufflen)
{
    uint16_t                    wTpCrc, wCalCrc;
    uint16_t                    pkgsize;
    LPModBusGetFile4EH          pGetFile;
    struct mb_config_ctx_t      config_ctx;

    pGetFile = (LPModBusGetFile4EH)pBuff;

    if(pGetFile->Cmd != _FC_GET_EDGE_SCRIPT_HANDLE)
        return -1;
    if(pGetFile->devtype != MB_FILE_DEV_TYPE)
        return -1;

    pkgsize = sizeof(ModBusGetFile4EH);
    wCalCrc = crc16(&pGetFile->SlaveAddr, pkgsize);
    wTpCrc  = MB_ALIGN_PT_16BIT_READ(&pGetFile->SlaveAddr + pkgsize);
    wTpCrc = SWAP_16BIT(wTpCrc);
    if(wTpCrc == wCalCrc)
    {
        config_ctx.curoffset = pGetFile->curoffset;
        config_ctx.packsize = SWAP_16BIT(MB_ALIGN_PT_16BIT_READ(&pGetFile->packsize));
        if(config_ctx.filesize <= 0)
            config_ctx.filesize = pGetFile->filesize;
        mb_edge_script_file_config(&config_ctx);
        return 0;
    }

    return -1;
}

static int mb_protocol_unpacket4dh(uint8_t *pBuff, uint16_t bufflen, uint8_t *request_id)
{
    uint16_t                    wTpCrc, wCalCrc;
    uint16_t                    pkgsize;
    LPModBusSndFile4DH          pSndFile;
    struct mb_config_ctx_t      config_ctx;

    pSndFile = (LPModBusSndFile4DH)pBuff;
    if(pSndFile->devtype != MB_FILE_DEV_TYPE)
        return -1;

    if(pSndFile->packsize > 1024)
        return -1;

    pkgsize = sizeof(ModBusSndFile4DH) + pSndFile->packsize;

    wCalCrc = crc16(&pSndFile->SlaveAddr, pkgsize);
    wTpCrc  = MB_ALIGN_PT_16BIT_READ(&pSndFile->SlaveAddr + pkgsize);
    wTpCrc = SWAP_16BIT(wTpCrc);
    if(wTpCrc == wCalCrc)
    {
        config_ctx.filesize  = pSndFile->filesize;
        config_ctx.filecrc   = pSndFile->filecrc;
        config_ctx.curoffset = pSndFile->curoffset;
        mb_edge_script_file_download(&config_ctx, &pSndFile->SlaveAddr + sizeof(ModBusSndFile4DH), pSndFile->packsize);
        *request_id = pSndFile->request_id;

        {
            struct mb_config_ctx_t *pLocalCtx = mb_edge_script_getctx();
            if(pLocalCtx->curoffset == pLocalCtx->filesize)
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
    char                        result[1024];
    pShell = (LPModBusShell48H)pBuff;

    if(pShell->devtype != MB_FILE_DEV_TYPE)
        return -1;

    pkgsize = sizeof(ModBusShell48H);
    wCalCrc = crc16(&pShell->SlaveAddr, pkgsize);
    wTpCrc  = MB_ALIGN_PT_16BIT_READ(&pShell->SlaveAddr + pkgsize);
    wTpCrc = SWAP_16BIT(wTpCrc); // CRC byte order is little endian
    if(wTpCrc == wCalCrc)
    {
        mb_shell_command((uint8_t *)(&pShell->SlaveAddr+sizeof(ModBusShell48H)),pShell->packsize, result, sizeof(result));
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

    pDebugMsginfo = (LPModBusDebugMsg49H)pBuff;
    pkgsize = sizeof(ModBusDebugMsg49H);

    wCalCrc = crc16(&pDebugMsginfo->SlaveAddr, pkgsize);
    wTpCrc  = MB_ALIGN_PT_16BIT_READ(&pDebugMsginfo->SlaveAddr + pkgsize);
    wTpCrc = SWAP_16BIT(wTpCrc); // CRC byte order is little endian
    if(wTpCrc == wCalCrc)
    {
        log_work_set(pDebugMsginfo->DebugState,pDebugMsginfo->ChannelNo,pDebugMsginfo->DevNo, pDebugMsginfo->app_name);
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

    /*  */
    address = pCtx->rs485_addr;
    if((address == addr) || (MB_COM_BROADCAST_ADDR == addr) || (MB_COM_ADMIN_ADDR == addr))
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
    uint8_t regNum;
    LPModBusHead pModbusHead;

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

    pCtx->sendLen = mb_protocol_packet03h(pCtx, pModbusHead->SlaveAddr, regAddr, regNum);
    if(pCtx->sendLen < 0)
    {
        //非法数据地址，返回地址错误异常码02
        pCtx->sendLen = mb_protocol_packet_exceptmsg(&pCtx->sendBuff[0], pModbusHead->SlaveAddr, pModbusHead->Cmd, MB_ILLEGAL_DATA_ADDRESS);
        ret = -1;
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
    //ret = mb_config_writecoil_handle(pCtx, regAddr, regValue);
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
    uint8_t regNum;
    uint16_t regAddr;
    uint16_t value[125];
    LPModBusHead pModbusHead;
    int ret = -1;

    pModbusHead = (LPModBusHead)pRecvBuff;
    /*
        unpacket the Message with 10H code
    */
    ret = mb_protocol_unpacket10h(pRecvBuff, recvLen, &regAddr, &regNum, &value[0]);
    if (ret != 0)
    {
        //格式不正确
        pCtx->sendLen = mb_protocol_packet_exceptmsg(&pCtx->sendBuff[0], pModbusHead->SlaveAddr, pModbusHead->Cmd, MB_ILLEGAL_DATA_VALUE);
        ret = -1;
        goto __end;
    }

    //对写寄存器个数为0（带密码时寄存器为0会被减为255）作处理
    if (0 == regNum)
    {
        pCtx->sendLen = mb_protocol_packet_exceptmsg(&pCtx->sendBuff[0], pModbusHead->SlaveAddr, pModbusHead->Cmd, MB_ILLEGAL_DATA_VALUE);
        ret = -1;
        goto __end;
    }

    // to do write 
    pCtx->sendLen = mb_protocol_packet10h(pCtx, &pCtx->sendBuff[0], pModbusHead->SlaveAddr, regAddr, regNum, value);

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
    uint8_t regNum, realRegNum;
    uint16_t regAddr;
    uint16_t password;
    uint8_t *pValue;
    LPModBusHead pModbusHead;
    int ret = -1;
    
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

    address = request_id;
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

    address = request_id;
    if (((pModbusHead->SlaveAddr != MB_COM_BROADCAST_ADDR) && (pCtx->sendLen > 0)) ||
        ((pModbusHead->SlaveAddr == MB_COM_BROADCAST_ADDR) && (pCtx->sendLen > 0) && (request_id == address)))
    {
        pCtx->pcombase->write(pCtx, &pCtx->sendBuff[0], pCtx->sendLen);
    }
    return ret;
}

static int mb_process_4EH(struct mb_comm_ctx_t *pCtx, uint8_t *pRecvBuff, uint16_t recvLen)
{
    LPModBusHead pModbusHead;
    int ret = -1;

    pModbusHead = (LPModBusHead)pRecvBuff;
    ret = mb_protocol_unpacket4eh(pRecvBuff, recvLen);

    if (ret != 0)
        return -1;

    pCtx->sendLen = mb_protocol_packet4eh(&pCtx->sendBuff[0], pModbusHead->SlaveAddr);
    ret = 0;

    if ((pModbusHead->SlaveAddr != MB_COM_BROADCAST_ADDR) && (pCtx->sendLen > 0))
        pCtx->pcombase->write(pCtx, &pCtx->sendBuff[0], pCtx->sendLen);
    return ret;
}

static int mb_process_4DH(struct mb_comm_ctx_t *pCtx, uint8_t *pRecvBuff, uint16_t recvLen)
{
    uint8_t address, request_id;
    LPModBusHead pModbusHead;
    int ret = -1;

    pModbusHead = (LPModBusHead)pRecvBuff;
    ret = mb_protocol_unpacket4dh(pRecvBuff, recvLen, &request_id);

    if (ret == -1)
        return -1;

    pCtx->sendLen = mb_protocol_packet4dh(&pCtx->sendBuff[0], pModbusHead->SlaveAddr);

    if(ret == 1)
        mb_edge_script_file_update();

    ret = 0;

    address = request_id;
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
    pCtx->sendLen = mb_protocol_packet49h(&pCtx->sendBuff[0], pModbusHead->SlaveAddr,&pMsgBuff[0], actlen);

    ret = 0;

    address = request_id;
    if (((pModbusHead->SlaveAddr != MB_COM_BROADCAST_ADDR) && (pCtx->sendLen > 0)) ||
        ((pModbusHead->SlaveAddr == MB_COM_BROADCAST_ADDR) && (pCtx->sendLen > 0) && (request_id == address)))
    {
        pCtx->pcombase->write(pCtx, &pCtx->sendBuff[0], pCtx->sendLen);
    }
;
    return ret;
}

/*********************************************************************************************************
** Function name:       mb_process_4FH
** Descriptions:        edge_alert 告警记录查询
*********************************************************************************************************/
static int mb_process_4FH(struct mb_comm_ctx_t *pCtx, uint8_t *pRecvBuff, uint16_t recvLen)
{
    LPModBusHead pModbusHead;
    uint16_t resp_pdu_len = 0;
    uint16_t crc;
    int ret;

    pModbusHead = (LPModBusHead)pRecvBuff;

    if (recvLen < CRC16_SIZE + 4)
        return -1;

    ret = mb_edge_alert_query_handle(pModbusHead->SlaveAddr,
                                     pRecvBuff,
                                     recvLen - CRC16_SIZE,
                                     pCtx->sendBuff,
                                     MB_COM_TXBUFF_SIZE - CRC16_SIZE,
                                     &resp_pdu_len);
    if (ret < 0)
        return -1;

    crc = crc16(&pCtx->sendBuff[0], resp_pdu_len);
    MB_ALIGN_PT_16BIT_WRITE(pCtx->sendBuff + resp_pdu_len, crc);
    pCtx->sendLen = resp_pdu_len + CRC16_SIZE;

    if (((pModbusHead->SlaveAddr != MB_COM_BROADCAST_ADDR) && (pCtx->sendLen > 0)) ||
        ((pModbusHead->SlaveAddr == MB_COM_BROADCAST_ADDR) && (pCtx->sendLen > 0)))
    {
        pCtx->pcombase->write(pCtx, &pCtx->sendBuff[0], pCtx->sendLen);
    }

    return 0;
}

/*********************************************************************************************************
** Function name:       mb_process_50H
** Descriptions:        现场诊断 0x50
*********************************************************************************************************/
static int mb_process_50H(struct mb_comm_ctx_t *pCtx, uint8_t *pRecvBuff, uint16_t recvLen)
{
    LPModBusHead pModbusHead;
    uint16_t resp_pdu_len = 0;
    uint16_t crc;
    int ret;

    pModbusHead = (LPModBusHead)pRecvBuff;

    if (recvLen < CRC16_SIZE + 4)
        return -1;

    ret = mb_diagnose_handle(pModbusHead->SlaveAddr,
                             pRecvBuff,
                             recvLen - CRC16_SIZE,
                             pCtx->sendBuff,
                             MB_COM_TXBUFF_SIZE - CRC16_SIZE,
                             &resp_pdu_len);
    if (ret < 0)
        return -1;

    crc = crc16(&pCtx->sendBuff[0], resp_pdu_len);
    MB_ALIGN_PT_16BIT_WRITE(pCtx->sendBuff + resp_pdu_len, crc);
    pCtx->sendLen = resp_pdu_len + CRC16_SIZE;

    if (((pModbusHead->SlaveAddr != MB_COM_BROADCAST_ADDR) && (pCtx->sendLen > 0)) ||
        ((pModbusHead->SlaveAddr == MB_COM_BROADCAST_ADDR) && (pCtx->sendLen > 0)))
    {
        pCtx->pcombase->write(pCtx, &pCtx->sendBuff[0], pCtx->sendLen);
    }

    return 0;
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
    uint8_t request_id;
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
    rat = executeCMD(uninstall, buf, sizeof(buf));
    if(rat < 0)
    {
        result = 0;
    }
    else
    {
        if(strstr(buf, "No"))
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
    case _FC_SND_EDGE_SCRIPT_HANDLE:
        ret = mb_process_4DH(pCtx, pRecvBuff, recvLen);
        break;
    case _FC_GET_EDGE_SCRIPT_HANDLE:
        ret = mb_process_4EH(pCtx, pRecvBuff, recvLen);
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
    case _FC_ALARM_QUERY:
        ret = mb_process_4FH(pCtx, pRecvBuff, recvLen);
        break;
    case _FC_DIAGNOSE:
        ret = mb_process_50H(pCtx, pRecvBuff, recvLen);
        break;
    default:
        ret = mb_except_msg_process(pCtx, pRecvBuff, recvLen);
        break;
    }
    return ret;
}
