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


#ifndef _MB_DLT645_07_COM_H_
#define _MB_DLT645_07_COM_H_

#include <stdint.h>
#include <stddef.h>


#include <stdio.h>
#include <string.h>
#include "mb_cfg.h"
#include "db_struct.h"



#define DLT645_OK			0
#define DLT645_ERR			(-1)

#define DLT645_ERR_NO_FIND				(DLT645_ERR - 1)
#define DLT645_ERR_TABLE_DATA			(DLT645_ERR - 2)
#define DLT645_ERR_PACKET				(DLT645_ERR - 3)
#define DLT645_ERR_COMMADDR				(DLT645_ERR - 4)
#define DLT645_PARAM_ERR                (DLT645_ERR - 5)


#define DLT645_COMM_ADDR_SIZE			6
#define DLT645_W_PASSWORD_SIZE			4
#define DLT645_C_EVENT_ID_SIZE			4


/*
    动态结构体。（结构体是外部指针，比如共享内存等应用时，只有结构体类型，传递指针是动态的，所以无法定义时初始化, 因此需要用偏移量）
    ABCDEFGH 为字节序 标识代号。     A 为高   H为低字节。  
*/

#define REG_OFFSET_OF_VMEM(x)                   (offsetof(struct db_database_type, x) / sizeof(uint16_t)) // 获取 当前X成员变量在 struct db_database_type 中的偏移量（以modbus单个寄存器大小为基本单元，即2bytes）


#define REG_LITTLE_EDIAN                    0
#define REG_BIG_EDIAN                       1


/*
    自定义映射表。预留0x40001 （0地址）用作用户自定义寄存器
    一般用于用户根据系统需要挑测点功能
*/
//struct remap_Modbus_custom_node remapCustom[256] = {0};

#pragma pack(1)
/*承载在Dlt645上数据的自定义格式*/
typedef struct
{/*备注说明:由于M0不支持寄数转换故为了避免出错所有结构体数据采用u8*/
    uint8_t	RegAddr[2];
    uint8_t RegNum;
    uint8_t RegDataLen;  
} ST_Dlt645_Udefine_Head;

typedef struct
{
    ST_Dlt645_Udefine_Head *pHead;
	uint8_t *pData;
} ST_Udefine_Data_Form;

typedef struct
{
    void *p_data;
    uint16_t reg_begin;
    uint16_t reg_len;
    uint8_t data_type;
    uint8_t write_enable;
} ST_Udefine_Reg_Table;

#pragma pack()

#define U8Buf_swap16(U8Buf)	(U8Buf[0] | (U8Buf[1]<<8))
#define U8Buf_swap32(U8Buf)	(U8Buf[0] | (U8Buf[1]<<8) |(U8Buf[2]<<16)| (U8Buf[3] << 24))
#define U8Buf_swap24(U8Buf)	(U8Buf[0] | (U8Buf[1]<<8) | (U8Buf[2]<<16))


typedef enum
{
    DLT645_RO = 0x01,       //只读
    DLT645_WO = 0x02,       //只写
    DLT645_RW_NP = 0x03,    //读写，不要密码
    DLT645_RW_P = 0x83      //读写，需要密码
} DLT645_WRITEPERMISSION;   //读写权限


typedef enum
{
	DLT645_U8 = 0,
	DLT645_U16,
	DLT645_U32,
	DLT645_S8,
	DLT645_S16,
	DLT645_S32,
	DLT645_P_ARRAY,		/*指针数组*/
	DLT645_U16_ARRAY,	/*数值数据*/
    DLT645_STATUS_FLAG,
	DLT645_USER_DEFINE_TYPE, /*用户自定义数值数据*/

} DLT645_RECORD_TYPE;    

/*变量编码表*/
typedef struct stNodeData
{
	DLT645_RECORD_TYPE RdType;
	uint8_t *pData;
	uint8_t RdSize;
}ST_NodeData;

#define DLT645_CONST_12_BYTE	12
#define DLT645_CONST_8_BYTE		8
#define DLT645_CONST_6_BYTE		6
#define DLT645_CONST_4_BYTE		4
#define DLT645_CONST_3_BYTE		3
#define DLT645_CONST_2_BYTE		2
#define DLT645_CONST_1_BYTE		1

/************************************************************************************************
    回调函数:       *Dlt645_W_callfunc
    功能描述:       回调函数实现DLT645写数据的功能，用户根据函数里的数据标识DataID，
	                把pInParam里的数据，设置到对应的数据项中去
	
	输入参数:     
	   <1> DLT645标准说明
		DataID:
				1）为《DLT645-2007多功能电能表通信协议数据标识编码表》里的数据标识;
				2）即为包格式里的----DIODI1DI2DI3（数据标识）
				3）数据标识能标记唯一的标量(如：DataID:0x04000e01;此数据标识表示正向有功功率上限值
 		
		pInParam: 
		         1）数据标识对应要刷新的数据;对应写帧格式里：DATA（真实数据）---采用BCD碥码
		         2）数据标识已经确定了数据的格式；参考《DLT645-2007多功能电能表通信协议数据标识编码表》
				 
		InLen  :   要刷新的数据的长度;对应写帧格式里：m(数据长度)
		
	  <2> 特殊功能说明：
	       DataID:特殊的数据标识码有如下：
				   DLT645_OPERATE_W_COMMADDR ：表示刷新通信地址；
				   DLT645_OPERATE_W_PASSWORD ：刷新密码
				   DLT645_OPERATE_REFLASH_BAUD_Z：表示刷新通信标记
				   DLT645_OPERATE_ADJUST_TIMER：广播对时命令
		           DLT645_OPERATE_FREEZE：冻结命令
				   DLT645_OPERATE_C_DEMAND ：清最大需量命令
				   DLT645_OPERATE_C_METER :电表清零命令
		           DLT645_OPERATE_C_EVENT :事情清零度命令
			
			pInParam: 
   			   1）DLT645_OPERATE_W_COMMADDR对应的pInParam入参数据为ST_W_CommAddr结构的数据；
               2）DLT645_OPERATE_W_PASSWORD对应的pInParam入参数据为ST_Password结构的数据；
			   3）DLT645_OPERATE_REFLASH_BAUD_Z对应的pInParam入参数据为ST_W_Baud结构的数据；
               4）DLT645_OPERATE_ADJUST_TIMER对应的pInParam入参数据为ST_Time_Calibration结构的数据；
			   5）DLT645_OPERATE_FREEZE对应的pInParam入参数据为ST_Freeze_Cmd结构的数据；
               6）DLT645_OPERATE_C_DEMAND对应的pInParam入参数据为NULL；
			   7）DLT645_OPERATE_C_METER对应的pInParam入参数据为NULL；
               8）DLT645_OPERATE_C_EVENT对应的pInParam入参数据为ST_C_Event_PARAM结构的数据；

		
    输出参数:     
    返 回 值:   0:表示成功 !0表示失败    
    说    明:   pInParam：可为NULL;
************************************************************************************************/
typedef int32_t (*Dlt645_W_callfunc)( uint32_t DataID, void *pInParam,uint16_t InLen);

/****************************************************************************************************
    函数名称:       *Dlt645_R_callfunc
    功能描述:       回调函数实现DLT645读数据的功能，用户根据函数里的数据标识DataID，获取对应的数据项，输出到pOutData
	                缓存里；
    输入参数:  输入参数:     
	   <1> DLT645标准说明
		   DataID:
				1）为《DLT645-2007多功能电能表通信协议数据标识编码表》里的数据标识;
				2）即为包格式里的----DIODI1DI2DI3（数据标识）
				3）数据标识能标记唯一的标量(如：DataID:0x00000000;此数据标识表示当前组合有功费率电能
 		
		pInParam: 这取决于InLen的长度；
		         1）InLen=0时没有pInParam=NULL；
		         2）InLen=1时pInParam的数据长度为1；值为N（负荷记录块数)
				 6）InLen=6时pInParam的数据长度为6；pInParam的组织形式为 “N（负荷记录块数）+分|时|日|月|年”
		InLen  :   要刷新的数据的长度;对应写帧格式里：m(数据长度)
		
		pOutData：
		        1）为设备读取出来数据存放缓存区；pOutData的格式参考《DLT645-2007多功能电能表通信协议数据标识编码表》
		        2） 格式一般用BCD编码；
				
		OutLen ：为此缓存的大小；用此判断是否越界;
		
    输出参数:     
    返 回 值:    读出来的数据长度
    说    明:   输出的数据要满足《DLT645-2007多功能电能表通信协议数据标识编码表》要求；采用BCD碥码
*******************************************************************************************************/

typedef uint16_t (*Dlt645_R_callfunc)( uint32_t DataID,const void *pInParam,uint16_t InLen,void *pOutData,uint16_t OutLen);

/************************************************************************
函数功能：Dlt645_analyze_packet_deal   分解数据处理
输入参数：pRevBuf  接收到的数据包；
          Plen 接收到的数据包长度
		  pAckPack 响应包的缓存指针
		  Buflen 响应包缓存的大小
输出参数：
返回值  :返回ACK包的长度；为0表示异常或不处理
说明    ：
************************************************************************/
uint16_t Dlt645_analyze_packet_deal(uint8_t *pRevBuf,uint16_t Revlen,uint8_t *pAckPack,uint16_t Buflen);


/***************************DLT645用户注册接口结构体*******************************************/
typedef struct st_user_cfg
{

	uint8_t CommAddr[DLT645_COMM_ADDR_SIZE];	/*通信地址*/
    uint8_t WPassword[DLT645_W_PASSWORD_SIZE];	/*通信密码*/
	Dlt645_W_callfunc pW_Fun;					/*写回调函数*/
	Dlt645_R_callfunc pR_Fun;					/*读回调函数*/
}ST_USER_CFG_INTF;

/*************************************************
    函数名称:       Dlt645_intf_user_cfg_table
    功能描述:       用户注册接口
    子 函 数:       
    调用限制:       无
    输入参数:      pUserTable :上层配置的变量表
    输出参数:       usSize，记录条数
    返 回 值:       
    说    明:       
*************************************************/
int32_t Dlt645_user_register_intf(ST_USER_CFG_INTF *pUserInfo);


typedef struct
{
	uint8_t DataDI3;	//数据标识D3
	uint8_t DataDI2;	//数据标识D2
    struct st_data_encode_table *p_data; //存储位置基址
	uint16_t reg_len;                 //寄存器个数
	int (*handle)(struct st_data_encode_table *, uint16_t , uint32_t *);
} ST_REG_ATTR;




/*变量编码表*/
typedef struct st_data_encode_table
{
	uint16_t DataId;		/*数据标识  DI1 DI0*/
	uint8_t WREnable;		/*读写权限*/
	DLT645_RECORD_TYPE RdType;
	uint8_t RdSize;			/*长度*/
    uint8_t Decimal;		/*dlt645显示放大倍数即小数位数*/
	uint8_t factor;			/*仪表计算因子 10^N*/
    uint32_t origin_offset;	// 偏移字节数 / sizeof(uint16_t)
    uint32_t offset; 		// 偏移
    uint16_t *node;  		// 该modbus寄存器指向 变量的指针, 用作读取值
}ST_DATA_ENCODE_TABLE;



/*********************************************************************************************************
** Function name:       DLT645_07_config_init
** Descriptions:        modbus rtu 协议配置初始化,主要是从EEPROM中加载配置，并判断配置是否正确
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void DLT645_07_config_init(void *vmem);

#endif

