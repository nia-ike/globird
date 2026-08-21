#ifndef _IEC101_DATA_H_
#define _IEC101_DATA_H_

#include <stdint.h>
#include <string.h>

enum eRecvFrameType
{// 接收报文类型
	eRecvFrameType_NULL = 0,//未知
	eRecvFrameType_Fixed_Length = 2,//固定长度报文
	eRecvFrameType_Variable_Length = 3,//可变长度报文
};

struct M_CTRL
{// 主站控制字节定义
	uint8_t FUN:4;	//功能码
	uint8_t FCV:1; 
	uint8_t FCB:1;
	uint8_t PRM:1;
	uint8_t Res:1;
};

struct S_CTRL
{// 从站控制字节定义
	uint8_t FUN:4; //功能码
	uint8_t DFC:1;
	uint8_t ACD:1;
	uint8_t PRM:1;
	uint8_t Res:1;
};

// 在监视方向的过程信息
#define M_SP_NA_1   1    //单点信息
#define M_SP_TA_1   2    //带时标单点信息
#define M_DP_NA_3   3    //双点信息
#define M_DP_TA_1   4    //带时标双点信息
#define M_ST_NA_1   5    //步位置信息
#define M_ST_TA_1   6    //带时标步位置信息
#define M_BO_NA_1   7    //32比特串
#define M_BO_TA_1   8    //带时标32比特串
#define M_ME_NA_1   9    //测量值，归一化值
#define M_ME_TA_1   10   //测量值，带时标归一化值
#define M_ME_NB_1   11   //测量值，标度化值
#define M_ME_TB_1   12   //测量值，带时标标度化值
#define M_ME_NC_1   13   //测量值，短浮点数
#define M_ME_TC_1   14   //测量值，带时标短浮点数
#define M_IT_NA_1   15   //累计量
#define M_IT_TA_1   16   //带时标累计量
#define M_EP_TA_1   17   //带时标继电保护装置事件
#define M_EP_TB_1   18   //带时标继电保护装置成组启动事件
#define M_EP_TC_1   19   //带时标继电保护装置成组输出电路信息
#define M_PS_NA_1   20   //具有状态变位检出的成组单点信息
#define M_ME_ND_1   21   //测量值，不带品质描述的归一化值
#define M_SP_TB_1   30   //带时标CP56Time2a的单点信息
#define M_DP_TB_1   31   //带时标CP56Time2a的双点信息
#define M_ST_TB_1   32   //带时标CP56Time2a的步位信息
#define M_BO_TB_1   33   //带时标CP56Time2a的32位串
#define M_ME_TD_1   34   //带时标CP56Time2a的归一化测量值
#define M_ME_TE_1   35   //带时标CP56Time2a的标度化值
#define M_ME_TF_1   36   //带时标CP56Time2a的短浮点数
#define M_IT_TB_1   37   //带时标CP56Time2a的累计值
#define M_EP_TD_1   38   //带时标CP56Time2a的继电保护装置事件
#define M_EP_TE_1   39   //带时标CP56Time2a的成组继电保护装置启动事件
#define M_EP_TF_1   40   //带时标CP56Time2a的继电保护装置成组输出电路信息

// 在控制方向的过程信息
#define C_SC_NA_1   45   //单命令
#define C_DC_NA_1   46   //双命令
#define C_RC_NA_1   47   //升降命令
#define C_SE_NA_1   48   //设定值命令，归一化值
#define C_SE_NB_1   49   //设定值命令，标度化值
#define C_SE_NC_1   50   //设定值命令，短浮点数
#define C_BO_NA_1   51   //32比特串
#define C_SC_TA_1   58   //带时标CP56Time2a的单命令
#define C_DC_TA_1   59   //带时标CP56Time2a的双命令
#define C_RC_TA_1   60   //带时标CP56Time2a的升降命令
#define C_SE_TA_1   61   //带时标CP56Time2a的设定值命令，归一化值
#define C_SE_TB_1   62   //带时标CP56Time2a的设定值命令，标度化值
#define C_SE_TC_1   63   //带时标CP56Time2a的设定值命令，短浮点数
#define C_BO_TA_1   64   //带时标CP56Time2a的32比特串

// 在监控方向的系统信息
#define M_EI_NA_1   70   //初始化结束

// 在控制方向的系统信息
#define C_IC_NA_1   100  //总召唤命令
#define C_CI_NA_1   101  //电能脉冲召唤命令
#define C_RD_NA_1   102  //读命令
#define C_CS_NA_1   103  //时钟同步命令
#define C_TS_NA_1   104  //测试命令
#define C_RP_NA_1   105  //复归进程命令
#define C_CD_NA_1   106  //延时传输命令
#define C_TS_TA_1   107  //带时标CP56Time2a的测试命令

// 在控制方向的参数命令
#define P_ME_NA_1   110  //测量值参数，归一化值
#define P_ME_NB_1   111  //测量值参数，标度化值
#define P_ME_NC_1   112  //测量值参数，短浮点数
#define P_AC_NA_1   113  //参数激活

// 文件传输
#define F_FR_NA_1   120  //文件准备好
#define F_SR_NA_1   121  //节已准备好
#define F_SC_NA_1   122  //召唤目录、选择文件、召唤文件、召唤节
#define F_LS_NA_1   123  //最后的节、最后的度
#define F_AF_NA_1   124  //确认文件、确认节
#define F_SG_NA_1   125  //段
#define F_DR_TA_1   126  //目录

#endif
