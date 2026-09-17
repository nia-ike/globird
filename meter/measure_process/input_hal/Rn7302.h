/*************************************************
  Copyright (C), 2003-2017, pilot Co., Ltd.
  File name:      Rn7302.h
  Author:       longzh    Version: 1.0       Date: 2017-6-9
  Description:  Rn7302 ����
  Others:         
  History:              
*************************************************/
#ifndef __RN7302_H__
#define __RN7302_H__
#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/msg.h>


#ifdef  __cplusplus
extern "C" {
#endif

/* С��ģʽCPU ���ú� */
#define __LITTLE_ENDIAN__             1 

//���߷�ʽ
#define P3W3 1
#define P3W4 0


enum
{
//    MSR_TYPE_RN8306_PHASE_ADJUST_HAND = 0, //手动相序调整，电流通道切换
//    MSR_TYPE_RN8306_WIRINGMODE,            //接线模式
//    MSR_TYPE_RN8306_PLUSE,                 //脉冲输出
//    MSR_TYPE_RN8306_USER_ALL,              //用户系数：相序调整，接线模式，脉冲输出
//    MSR_TYPE_RN8306_SOFTWARE_RST,            //软件复位
//    MSR_TYPE_RN8306_PHASE_ADJUST_AUTO_PROCESS,     //自动相序调整，电流通道切换
//    MSR_TYPE_RN8306_PHASE_ADJUST_AUTO_RESULT,     //自动相序调整，电流通道切换
    MSR_TYPE_RN7302_ADJUST_RST,            //校表系数复位
//    MSR_TYPE_RN8306_SYS_ALL,                    //校表系数更改

    MSR_TYPE_RN7302_REINIT,                     //重新初始化
    MSR_TYPE_RN7302_GET_INIT_STATE,             //获取初始化状态
};

//===============================�Ĵ�����ַ=============================================================================
//--    ����RN7302�Ĵ�����С��һ�£��Ĵ�����ַ��Ĵ�����С�� ���ڶ�ȡRN7302�Ĵ���ֵ�� 
//        �����������£����磺
//            RN7302_REG_UAWAV            REG�����Ĵ�����ַ
//            RN7302_LEN_UAWAV            LEN�����Ĵ�������
//          ����һ�£�ֻ��REG ��LEN���𣬷������
/*
    bank 0.
*/
#define  RN7302_REG_UAWAV       0x00            /* A 相电压采样数据 */
#define  RN7302_REG_UBWAV       0x01            /* B 相电压采样数据 */
#define  RN7302_REG_UCWAV       0x02            /* C 相电压采样数据 */
#define  RN7302_REG_IAWAV       0x03            /* A 相电流采样数据 */
#define  RN7302_REG_IBWAV       0x04            /* B 相电流采样数据 */
#define  RN7302_REG_ICWAV       0x05            /* C 相电流采样数据 */
#define  RN7302_REG_INWAV       0x06            /* 零线电流采样数据 */
#define  RN7302_REG_UA          0x07            /* A 相电压 */
#define  RN7302_REG_UB          0x08            /* B 相电压 */
#define  RN7302_REG_UC          0x09            /* C 相电压 */
#define  RN7302_REG_USUM        0x0A            /*   相电压矢量和 */
#define  RN7302_REG_IA          0x0B            /* A 相电流 */
#define  RN7302_REG_IB          0x0C            /* B 相电流 */
#define  RN7302_REG_IC          0x0D            /* C 相电流 */
#define  RN7302_REG_IN          0x0E            /*   零线电流 */
/*    reserved    0x0F */                       
#define  RN7302_REG_ISUM        0x10            /*   相电流矢量和 */
                                                
#define  RN7302_REG_PA          0x14            /* A 相有功功率 */
#define  RN7302_REG_PB          0x15            /* B 相有功功率 */
#define  RN7302_REG_PC          0x16            /* C 相有功功率 */
#define  RN7302_REG_PT          0x17            /* 合相有功功率 */
#define  RN7302_REG_QA          0x18            /* A 相无功功率 */
#define  RN7302_REG_QB          0x19            /* B 相无功功率 */
#define  RN7302_REG_QC          0x1A            /* C 相无功功率 */
#define  RN7302_REG_QT          0x1B            /* 合相无功功率 */
#define  RN7302_REG_SA          0x1C            /* A 相视在功率 */
#define  RN7302_REG_SB          0x1D            /* B 相视在功率 */
#define  RN7302_REG_SC          0x1E            /* C 相视在功率 */
#define  RN7302_REG_STA         0x1F            /* 合相视在功率 */
#define  RN7302_REG_PFA         0x20            /* A 相功率因素 */
#define  RN7302_REG_PFB         0x21            /* B 相功率因素 */
#define  RN7302_REG_PFC         0x22            /* C 相功率因素 */
#define  RN7302_REG_PFTA        0x23            /* 合相功率因素 */
#define  RN7302_REG_PAFCnt      0x24            /* A 相有功快速脉冲计数 */
#define  RN7302_REG_PBFCnt      0x25            /* B 相有功快速脉冲计数 */
#define  RN7302_REG_PCFCnt      0x26            /* C 相有功快速脉冲计数 */
#define  RN7302_REG_PTFCnt      0x27            /* 合相有功快速脉冲计数 */
#define  RN7302_REG_QAFCnt      0x28            /* A 相无功快速脉冲计数 */
#define  RN7302_REG_QBFCnt      0x29            /* B 相无功快速脉冲计数 */
#define  RN7302_REG_QCFCnt      0x2A            /* C 相无功快速脉冲计数 */
#define  RN7302_REG_QTFCnt      0x2B            /* 合相无功快速脉冲计数 */
#define  RN7302_REG_SAFCnt      0x2C            /* A 相视在快速脉冲计数 */
#define  RN7302_REG_SBFCnt      0x2D            /* B 相视在快速脉冲计数 */
#define  RN7302_REG_SCFCnt      0x2E            /* C 相视在快速脉冲计数 */
#define  RN7302_REG_STFCnt      0x2F            /* 合相视在快速脉冲计数 */
/* Energy */                                /*  */
#define  RN7302_REG_EPA         0x30            /* A 相有功电能 */
#define  RN7302_REG_EPB         0x31            /* B 相有功电能 */
#define  RN7302_REG_EPC         0x32            /* C 相有功电能 */
#define  RN7302_REG_EPT         0x33            /* 合相有功电能 */
#define  RN7302_REG_POSEPA      0x34            /* A 相正向有功电能 */
#define  RN7302_REG_POSEPB      0x35            /* B 相正向有功电能 */
#define  RN7302_REG_POSEPC      0x36            /* C 相正向有功电能 */
#define  RN7302_REG_POSEPT      0x37            /* 合相正向有功电能 */
#define  RN7302_REG_NEGEPA      0x38            /* A 相反向有功电能 */
#define  RN7302_REG_NEGEPB      0x39            /* B 相反向有功电能 */
#define  RN7302_REG_NEGEPC      0x3A            /* C 相反向有功电能 */
#define  RN7302_REG_NEGEPT      0x3B            /* 合相反向有功电能 */
#define  RN7302_REG_EQA         0x3C            /* A 相无功电能 */
#define  RN7302_REG_EQB         0x3D            /* B 相无功电能 */
#define  RN7302_REG_EQC         0x3E            /* C 相无功电能 */
#define  RN7302_REG_EQT         0x3F            /* 合相无功电能 */
#define  RN7302_REG_POSEQA      0x40            /* A 相正向无功电能 */
#define  RN7302_REG_POSEQB      0x41            /* B 相正向无功电能 */
#define  RN7302_REG_POSEQC      0x42            /* C 相正向无功电能 */
#define  RN7302_REG_POSEQT      0x43            /* 合相正向无功电能 */
#define  RN7302_REG_NEGEQA      0x44            /* A 相反向无功电能 */
#define  RN7302_REG_NEGEQB      0x45            /* B 相反向无功电能 */
#define  RN7302_REG_NEGEQC      0x46            /* C 相反向无功电能 */
#define  RN7302_REG_NEGEQT      0x47            /* 合相反向无功电能 */
#define  RN7302_REG_ESA         0x48            /* A 相视在电能 */
#define  RN7302_REG_ESB         0x49            /* B 相视在电能 */
#define  RN7302_REG_ESC         0x4A            /* C 相视在电能 */
#define  RN7302_REG_EST         0x4B            /* 合相视在电能 */
#define  RN7302_REG_STV         0x4C            /* PQS 合相视在功率 */
#define  RN7302_REG_PFTV        0x4D            /* PQS 合相功率因数 */
#define  RN7302_REG_STFVCNT     0x4E            /* PQS 合相视在快速脉冲计数 */
#define  RN7302_REG_ESTV        0x4F            /* PQS 合相视在能量寄存器 */
                                                
/* BASE Wav */                                  
#define  RN7302_REG_YUA         0x50            /* A 相U基波相角 */
#define  RN7302_REG_YUB         0x51            /* B 相U基波相角 */
#define  RN7302_REG_YUC         0x52            /* C 相U基波相角 */
#define  RN7302_REG_YIA         0x53            /* A 相I基波相角 */
#define  RN7302_REG_YIB         0x54            /* B 相I基波相角 */
#define  RN7302_REG_YIC         0x55            /* C 相I基波相角 */
#define  RN7302_REG_YIN         0x56            /* N 相I基波相角 */
#define  RN7302_REG_UFREQ       0x57            /* 电压线频率 */
                                                
//modify by longzh--------------                
#define  RN7302_REG_FUA         0x58            /* A相基波电压有效值 */
#define  RN7302_REG_FUB         0x59            /* B相基波电压有效值 */
#define  RN7302_REG_FUC         0x5A            /* C相基波电压有效值 */
#define  RN7302_REG_FIA         0x5B            /* A相基波电流有效值 */
#define  RN7302_REG_FIB         0x5C            /* B相基波电流有效值 */
#define  RN7302_REG_FIC         0x5D            /* C相基波电流有效值 */

#define  RN7302_REG_FPA         0x5E            /* A相基波有功功率有效值 */
#define  RN7302_REG_FPB         0x5F            /* B相基波有功功率有效值 */
#define  RN7302_REG_FPC         0x60            /* C相基波有功功率有效值 */
#define  RN7302_REG_FPT         0x61            /* 合相基波有功功率有效值 */
#define  RN7302_REG_FQA         0x62            /* A相基波无功功率有效值 */
#define  RN7302_REG_FQB         0x63            /* B相基波无功功率有效值 */
#define  RN7302_REG_FQC         0x64            /* C相基波无功功率有效值 */
#define  RN7302_REG_FQT         0x65            /* 合相基波无功功率有效值 */
#define  RN7302_REG_FSA         0x66            /* A相基波视在功率有效值 */
#define  RN7302_REG_FSB         0x67            /* B相基波视在功率有效值 */
#define  RN7302_REG_FSC         0x68            /* C相基波视在功率有效值 */
#define  RN7302_REG_FSTA        0x69            /* RMS合相基波视在功率有效值 */
#define  RN7302_REG_FPFA        0x6A            /* A相基波功率因数有效值 */
#define  RN7302_REG_FPFB        0x6B            /* B相基波功率因数有效值 */
#define  RN7302_REG_FPFC        0x6C            /* C相基波功率因数有效值 */
#define  RN7302_REG_FPFTA       0x6D            /* RMS合相基波功率因数有效值 */
#define  RN7302_REG_FPAFCnt     0x6E            /* A相基波有功功率脉冲计数 */
#define  RN7302_REG_FPBFCnt     0x6F            /* B相基波有功功率脉冲计数 */
#define  RN7302_REG_FPCFCnt     0x70            /* C相基波有功功率脉冲计数 */
#define  RN7302_REG_FPTFCnt     0x71            /* 合相基波有功功率脉冲计数 */
#define  RN7302_REG_FQAFCnt     0x72            /* A相基波无功功率脉冲计数 */
#define  RN7302_REG_FQBFCnt     0x73            /* B相基波无功功率脉冲计数 */
#define  RN7302_REG_FQCFCnt     0x74            /* C相基波无功功率脉冲计数 */
#define  RN7302_REG_FQTFCnt     0x75            /* 合相基波无功功率脉冲计数 */
#define  RN7302_REG_FSAFCnt     0x76            /* A相基波视在功率脉冲计数 */
#define  RN7302_REG_FSBFCnt     0x77            /* B相基波视在功率脉冲计数 */
#define  RN7302_REG_FSCFCnt     0x78            /* C相基波视在功率脉冲计数 */
#define  RN7302_REG_FSTAFCnt    0x79            /* RMS合相基波视在功率脉冲计数 */

#define  RN7302_REG_FEPA        0x7A            /* A相基波有功电能 */
#define  RN7302_REG_FEPB        0x7B            /* B相基波有功电能 */
#define  RN7302_REG_FEPC        0x7C            /* C相基波有功电能 */
#define  RN7302_REG_FEPT        0x7D            /* 合相基波有功电能 */
#define  RN7302_REG_POSFEPA     0x7E            /* A相基波正向有功电能 */
#define  RN7302_REG_POSFEPB     0x7F            /* B相基波正向有功电能 */
#define  RN7302_REG_POSFEPC     0x80            /* C相基波正向有功电能 */
#define  RN7302_REG_POSFEPT     0x81            /* 合相基波正向有功电能 */
#define  RN7302_REG_NEGFEPA     0x82            /* A相基波反向有功电能 */
#define  RN7302_REG_NEGFEPB     0x83            /* B相基波反向有功电能 */
#define  RN7302_REG_NEGFEPC     0x84            /* C相基波反向有功电能 */
#define  RN7302_REG_NEGFEPT     0x85            /* 合相基波反向有功电能 */

#define  RN7302_REG_FEQA        0x86            /* A相基波无功电能 */
#define  RN7302_REG_FEQB        0x87            /* B相基波无功电能 */
#define  RN7302_REG_FEQC        0x88            /* C相基波无功电能 */
#define  RN7302_REG_FEQT        0x89            /* 合相基波无功电能 */
#define  RN7302_REG_POSFEQA     0x8A            /* A相基波正向无功电能 */
#define  RN7302_REG_POSFEQB     0x8B            /* B相基波正向无功电能 */
#define  RN7302_REG_POSFEQC     0x8C            /* C相基波正向无功电能 */
#define  RN7302_REG_POSFEQT     0x8D            /* 合相基波正向无功电能 */
#define  RN7302_REG_NEGFEQA     0x8E            /* A相基波反向无功电能 */
#define  RN7302_REG_NEGFEQB     0x8F            /* B相基波反向无功电能 */
#define  RN7302_REG_NEGFEQC     0x90            /* C相基波反向无功电能 */
#define  RN7302_REG_NEGFEQT     0x91            /* 合相基波反向无功电能 */

#define  RN7302_REG_FESA        0x92            /* A相基波视在电能 */
#define  RN7302_REG_FESB        0x93            /* B相基波视在电能 */
#define  RN7302_REG_FESC        0x94            /* C相基波视在电能 */
#define  RN7302_REG_FESTA       0x95            /* RMS合相基波视在电能 */

#define  RN7302_REG_HUa         0x96        /* A相基波电压有效值 */
#define  RN7302_REG_HUb         0x97        /* B相基波电压有效值 */
#define  RN7302_REG_HUc         0x98        /* C相基波电压有效值 */
#define  RN7302_REG_HIa         0x99        /* A相基波电流有效值 */
#define  RN7302_REG_HIb         0x9A        /* B相基波电流有效值 */
#define  RN7302_REG_HIc         0x9B        /* C相基波电流有效值 */


#define  RN7302_REG_FSTV        0x9C            /* PQS合相基波视在功率 */
#define  RN7302_REG_FPFTV       0x9D            /* PQS合相基波功率因数 */
#define  RN7302_REG_FSTVFCnt    0x9E            /* PQS合相基波视在功率 */
#define  RN7302_REG_FESTV       0x9F            /* PQS合相基波视在电能 */

//---------------------------------------------------------------------
//RN8306-V3 ?????

#define  RN7302_REG_UA_HW          0xA0            /* A 相电压半波有效值 */
#define  RN7302_REG_UB_HW          0xA1            /* B 相电压半波有效值 */
#define  RN7302_REG_UC_HW          0xA2            /* C 相电压半波有效值 */
#define  RN7302_REG_IA_HW          0xA3            /* A 相电流半波有效值 */
#define  RN7302_REG_IB_HW          0xA4            /* B 相电流半波有效值 */
#define  RN7302_REG_IC_HW          0xA5            /* C 相电流半波有效值 */
#define  RN7302_REG_UAPEAK_HW      0xA6            /* A 相电压半波峰值 */
#define  RN7302_REG_UBPEAK_HW      0xA7            /* B 相电压半波峰值 */
#define  RN7302_REG_UCPEAK_HW      0xA8            /* C 相电压半波峰值 */
#define  RN7302_REG_IN_HW          0xA9            /* 零线电流半波有效值 */
#define  RN7302_REG_FUA_WAVE       0xAA            /* A相电压基波瞬时采样波形 */
#define  RN7302_REG_FUB_WAVE       0xAB            /* B相电压基波瞬时采样波形 */
#define  RN7302_REG_FUC_WAVE       0xAC            /* C相电压基波瞬时采样波形 */
#define  RN7302_REG_FIA_WAVE       0xAD            /* A相电流基波瞬时采样波形 */
#define  RN7302_REG_FIB_WAVE       0xAE            /* B相电流基波瞬时采样波形 */
#define  RN7302_REG_FIC_WAVE       0xAF            /* C相电流基波瞬时采样波形 */
#define  RN7302_REG_FIN_WAVE       0xB0            /* 零线电流基波瞬时采样波形 */
#define  RN7302_REG_SPL_PA         0xB1            /* A相瞬时（半波）有功功率 */
#define  RN7302_REG_SPL_PB         0xB2            /* B相瞬时（半波）有功功率 */
#define  RN7302_REG_SPL_PC         0xB3            /* C相瞬时（半波）有功功率 */
#define  RN7302_REG_SPL_QA         0xB4            /* A相瞬时（半波）无功功率 */
#define  RN7302_REG_SPL_QB         0xB5            /* B相瞬时（半波）无功功率 */
#define  RN7302_REG_SPL_QC         0xB6            /* C相瞬时（半波）无功功率 */
#define  RN7302_REG_SPL_FPA        0xB7            /* A相瞬时（半波）基本有功功率 */
#define  RN7302_REG_SPL_FPB        0xB8            /* B相瞬时（半波）基本有功功率 */
#define  RN7302_REG_SPL_FPC        0xB9            /* C相瞬时（半波）基本有功功率 */
#define  RN7302_REG_SPL_FQA        0xBA            /* A相瞬时（半波）基本无功功率 */
#define  RN7302_REG_SPL_FQB        0xBB            /* B相瞬时（半波）基本无功功率 */
#define  RN7302_REG_SPL_FQC        0xBC            /* C相瞬时（半波）基本无功功率 */
				
				
#define  RN7302_REG_SAG_UAPEAK     0xC3            /* UA通道SAG峰值最小值寄存器 */
#define  RN7302_REG_SAG_UBPEAK     0xC4            /* UB通道SAG峰值最小值寄存器 */
#define  RN7302_REG_SAG_UCPEAK     0xC5            /* UC通道SAG峰值最小值寄存器 */


#define  RN7302_REG_UA_HW2         0xC6            /* A 相电压半波有效值2 */
#define  RN7302_REG_UB_HW2         0xC7            /* B 相电压半波有效值2 */
#define  RN7302_REG_UC_HW2         0xC8            /* C 相电压半波有效值2 */
#define  RN7302_REG_IA_HW2         0xC9            /* A 相电流半波有效值2 */
#define  RN7302_REG_IB_HW2         0xCA            /* B 相电流半波有效值2 */
#define  RN7302_REG_IC_HW2         0xCB            /* C 相电流半波有效值2 */
/*
*        bank 1 .  configure register
*/
#define  RN7302_REG_HFConst1             0x100  /* 高频脉冲计数寄存器 1 */
#define  RN7302_REG_HFConst2             0x101  /* 高频脉冲计数寄存器 2 */
#define  RN7302_REG_ISTART_PS            0x102  /* 有功视在启动电流阀值寄存器 */
#define  RN7302_REG_ISTART_Q             0x103  /* 无功启动电流阀值寄存器 */
#define  RN7302_REG_LOSTVOLTT            0x104  /* 失压阀值寄存器 */
#define  RN7302_REG_ZXOT                 0x105  /* 过零阀值寄存器 */
#define  RN7302_REG_PRTH1L               0x106  /* 相位分段校正电流阀值寄存器1 下限 */
#define  RN7302_REG_PRTH1H               0x107  /* 相位分段校正电流阀值寄存器1 上限 */
#define  RN7302_REG_PRTH2L               0x108  /* 相位分段校正电流阀值寄存器2 下限 */
#define  RN7302_REG_PRTH2H               0x109  /* 相位分段校正电流阀值寄存器2 上限 */
                                                
#define  RN7302_REG_IRegion3L            0x10a  /* 电流阀值寄存器3 下限 */
#define  RN7302_REG_IRegion3H            0x10b  /* 电流阀值寄存器3 上限 */
#define  RN7302_REG_PHSUA                0x10c  /* 采样通道Ua相位校正寄存器 */
#define  RN7302_REG_PHSUB                0x10d  /* 采样通道Ub相位校正寄存器 */
#define  RN7302_REG_PHSUC                0x10e  /* 采样通道Uc相位校正寄存器 */
#define  RN7302_REG_PHSIA                0x10f  /* 采样通道Ia相位校正寄存器 */
#define  RN7302_REG_PHSIB                0x110  /* 采样通道Ib相位校正寄存器 */
#define  RN7302_REG_PHSIC                0x111  /* 采样通道Ic相位校正寄存器 */
#define  RN7302_REG_PHSIN                0x112  /* 采样通道In相位校正寄存器 */
#define  RN7302_REG_GSUA                 0x113  /* 采样通道Ua通道增益寄存器 */
#define  RN7302_REG_GSUB                 0x114  /* 采样通道Ub通道增益寄存器 */
#define  RN7302_REG_GSUC                 0x115  /* 采样通道Uc通道增益寄存器 */
#define  RN7302_REG_GSIA                 0x116  /* 采样通道Ia通道增益寄存器 */
#define  RN7302_REG_GSIB                 0x117  /* 采样通道Ib通道增益寄存器 */
#define  RN7302_REG_GSIC                 0x118  /* 采样通道Ic通道增益寄存器 */
#define  RN7302_REG_GSIN                 0x119  /* 采样通道In通道增益寄存器 */
                                                
#define  RN7302_REG_DCOS_UA              0x11a  /* 采样通道Ua直流offset校正寄存器 */
#define  RN7302_REG_DCOS_UB              0x11b  /* 采样通道Ub直流offset校正寄存器 */
#define  RN7302_REG_DCOS_UC              0x11c  /* 采样通道Uc直流offset校正寄存器 */
#define  RN7302_REG_DCOS_IA              0x11d  /* 采样通道Ia直流offset校正寄存器 */
#define  RN7302_REG_DCOS_IB              0x11e  /* 采样通道Ib直流offset校正寄存器 */
#define  RN7302_REG_DCOS_IC              0x11f  /* 采样通道Ic直流offset校正寄存器 */
#define  RN7302_REG_DCOS_IN              0x120  /* 采样通道In直流offset校正寄存器 */
                                                
#define  RN7302_REG_UA_OS                0x121  /* Ua有效值offset校正寄存器 */
#define  RN7302_REG_UB_OS                0x122  /* Ub有效值offset校正寄存器 */
#define  RN7302_REG_UC_OS                0x123  /* Uc有效值offset校正寄存器 */
#define  RN7302_REG_IA_OS                0x124  /* Ia有效值offset校正寄存器 */
#define  RN7302_REG_IB_OS                0x125  /* Ib有效值offset校正寄存器 */
#define  RN7302_REG_IC_OS                0x126  /* Ic有效值offset校正寄存器 */
#define  RN7302_REG_IN_OS                0x127  /* In有效值offset校正寄存器 */
                                                
#define  RN7302_REG_GPA                  0x128  /* Pa增益寄存器 */
#define  RN7302_REG_GPB                  0x129  /* Pb增益寄存器 */
#define  RN7302_REG_GPC                  0x12a  /* Pc增益寄存器 */
#define  RN7302_REG_GQA                  0x12b  /* Qa增益寄存器 */
#define  RN7302_REG_GQB                  0x12c  /* Qb增益寄存器 */
#define  RN7302_REG_GQC                  0x12d  /* Qc增益寄存器 */
#define  RN7302_REG_GSA                  0x12e  /* Sa增益寄存器 */
#define  RN7302_REG_GSB                  0x12f  /* Sb增益寄存器 */
#define  RN7302_REG_GSC                  0x130  /* Sc增益寄存器 */
                                                
#define  RN7302_REG_PA_PHSL              0x131  /* Pa分段相位校正寄存器 */
#define  RN7302_REG_PB_PHSL              0x132  /* Pb分段相位校正寄存器 */
#define  RN7302_REG_PC_PHSL              0x133  /* Pc分段相位校正寄存器 */
#define  RN7302_REG_QA_PHSL              0x134  /* Qa分段相位校正寄存器 */
#define  RN7302_REG_QB_PHSL              0x135  /* Qb分段相位校正寄存器 */
#define  RN7302_REG_QC_PHSL              0x136  /* Qc分段相位校正寄存器 */
                                                
#define  RN7302_REG_PA_OS                0x137  /* Pa OFFSET寄存器 */
#define  RN7302_REG_PB_OS                0x138  /* Pb OFFSET寄存器 */
#define  RN7302_REG_PC_OS                0x139  /* Pc OFFSET寄存器 */
#define  RN7302_REG_QA_OS                0x13a  /* Qa OFFSET寄存器 */
#define  RN7302_REG_QB_OS                0x13b  /* Qb OFFSET寄存器 */
#define  RN7302_REG_QC_OS                0x13c  /* Qc OFFSET寄存器 */
                                                
#define  RN7302_REG_FUA_OS               0x13D  /* Ua 基波有效值offset寄存器 */
#define  RN7302_REG_FUB_OS               0x13E  /* Ub 基波有效值offset寄存器 */
#define  RN7302_REG_FUC_OS               0x13F  /* Uc 基波有效值offset寄存器 */
#define  RN7302_REG_FIA_OS               0x140  /* Ia 基波有效值offset寄存器 */
#define  RN7302_REG_FIB_OS               0x141  /* Ib 基波有效值offset寄存器 */
#define  RN7302_REG_FIC_OS               0x142  /* Ic 基波有效值offset寄存器 */
                                                
#define  RN7302_REG_GFPA                 0x143  /* Pa 基波增益寄存器 */
#define  RN7302_REG_GFPB                 0x144  /* Pb 基波增益寄存器 */
#define  RN7302_REG_GFPC                 0x145  /* Pc 基波增益寄存器 */
#define  RN7302_REG_GFQA                 0x146  /* Qa 基波增益寄存器 */
#define  RN7302_REG_GFQB                 0x147  /* Qb 基波增益寄存器 */
#define  RN7302_REG_GFQC                 0x148  /* Qc 基波增益寄存器 */
#define  RN7302_REG_GFSA                 0x149  /* Sa 基波增益寄存器 */
#define  RN7302_REG_GFSB                 0x14a  /* Sb 基波增益寄存器 */
#define  RN7302_REG_GFSC                 0x14b  /* Sc 基波增益寄存器 */
                                                
#define  RN7302_REG_FPA_PHS              0x14c  /* Pa 基波校正寄存器 */
#define  RN7302_REG_FPB_PHS              0x14d  /* Pb 基波校正寄存器 */
#define  RN7302_REG_FPC_PHS              0x14e  /* Pc 基波校正寄存器 */
#define  RN7302_REG_FQA_PHS              0x14F  /* Qa 基波校正寄存器 */
#define  RN7302_REG_FQB_PHS              0x150  /* Qb 基波校正寄存器 */
#define  RN7302_REG_FQC_PHS              0x151  /* Qc 基波校正寄存器 */
                                                
#define  RN7302_REG_FPA_OS               0x152  /* Pa 基波OFFSET寄存器 */
#define  RN7302_REG_FPB_OS               0x153  /* Pb 基波OFFSET寄存器 */
#define  RN7302_REG_FPC_OS               0x154  /* Pc 基波OFFSET寄存器 */
#define  RN7302_REG_FQA_OS               0x155  /* Qa 基波OFFSET寄存器 */
#define  RN7302_REG_FQB_OS               0x156  /* Qb 基波OFFSET寄存器 */
#define  RN7302_REG_FQC_OS               0x157  /* Qc 基波OFFSET寄存器 */
                                                
#define  RN7302_REG_SAGCFG               0x158  /* 电压暂降阀值 */
#define  RN7302_REG_OVLVL                0x159  /* 过压阀值 */
#define  RN7302_REG_OILVL                0x15a  /* 过流阀值 */
#define  RN7302_REG_UCHSEL               0x15D  /* 电压通道切换 */
                                                
#define  RN7302_REG_CFCFG                0x160  /* CF引脚配置寄存器 */
#define  RN7302_REG_EMMCFG               0x161  /* EMU配置寄存器 */
#define  RN7302_REG_EMUCON               0x162  /* EMU控制寄存器 */
#define  RN7302_REG_WSAVECON             0x163  /* 采样数据写混存控制寄存器 */
#define  RN7302_REG_EMMIE                0x164  /* 中断允许 */
#define  RN7302_REG_EMMIF                0x165  /* 中断标志 */
#define  RN7302_REG_PQSign               0x166  /* 有功无功功率方向寄存器 */
#define  RN7302_REG_Noload               0x167  /* 潜动启动状态 */
#define  RN7302_REG_IRegionS             0x168  /* 电流分区状态寄存器 */
#define  RN7302_REG_PHASES               0x169  /* 中断允许 */
#define  RN7302_REG_CheckSum1            0x16a  /* EMM 校表和配置寄存器校验和 */

#define  RN7302_REG_EMMIE2          	 0x16B   /* 中断允许2 */
#define  RN7302_REG_EMMIF2          	 0x16C   /* 中断标志2 */
#define  RN7302_REG_EMUCFG3         	 0x16D   /* EMU配置寄存器3 */
	                                         
#define  RN7302_REG_NVM1CFG         	 0x170   /* 高通功能配置 */
#define  RN7302_REG_SYSCFG          	 0x178   /* 系统配置寄存器 */
#define  RN7302_REG_WREN2           	 0x17F   /* 写使能寄存器2 */
#define  RN7302_REG_WREN                 0x180   /* 写使能寄存器 */
#define  RN7302_REG_WMSW                 0x181   /* 工作模式切换寄存器 */
#define  RN7302_REG_SOFTRST              0x182    /* 软件复位寄存器 */
#define  RN7302_REG_ADCCFG               0x183   /* EMM 校表和配置寄存器校验和 */
#define  RN7302_REG_TFEEC_EN             0x184   /* 通道切换写使能寄存器 */
#define  RN7302_REG_ICHSEL               0x185   /* 电流通道切换寄存器 */
#define  RN7302_REG_MODSEL               0x186   /* 三相四线/三相三线模式选择寄存器 */
                                                
#define  RN7302_REG_SYSSR                0x18a   /* 系统状态寄存器 */
#define  RN7302_REG_CheckSum2            0x18B   /* 系统配置寄存器校验和 */
#define  RN7302_REG_RData                0x18c   /* 上一次SPI读出的数据 */
#define  RN7302_REG_WData                0x18d   /* 上一次SPI写入的数据 */
#define  RN7302_REG_LRBufAddr            0x18e   /* 最后一次读波形缓存的地址 */
#define  RN7302_REG_DeviceID        	 0x18F   /* RN8306 Device ID */

#define  RN7302_REG_ZXOTU                0x190   /* 电压夹角及测频阀值 */
#define  RN7302_REG_AUOTDC_EN            0x191   /* 直流 offset 自动校正使能 */
#define  RN7302_REG_ZXOTCFG              0x192   /* 过零计算配置及标志寄存器 */
#define  RN7302_REG_DMA_WAVECON          0x193   /* 同步采样通道配置寄存器 */
#define  RN7302_REG_IN_WaveEN            0x194   /* IN 波形缓存使能 */

#define  RN7302_REG_D2FCFG      		 0x195   /* 自定义功率 EMM 配置和状态寄存器*/
#define  RN7302_REG_HFCONST3      		 0x196   /* 自定义功率 高频脉冲计数寄存器 3*/
		   
#define  RN7302_REG_OI2_LVL         	 0x198   /* 过流幅值阀值配置寄存器 */
#define  RN7302_REG_OI2_CNTT        	 0x199   /* 过流时间阀值配置寄存器 */
#define  RN7302_REG_EMMCFG2         	 0x19A   /* EMU配置寄存器2 */
#define  RN7302_REG_OV2_LVL         	 0x19C   /* 过压幅值阀值配置寄存器 */
#define  RN7302_REG_OV2_CNTT        	 0x19D   /* 过压时间阀值配置寄存器 */
		   
#define  RN7302_REG_CHECKSUM3       	 0x1AE   /* RA9302B新增寄存器校验和寄存器0xFFFC07FB */


#define  RN7302_REG_PA_PHSM              0x1B0  /* A 相有功分段相位校正寄存器 中段 */
#define  RN7302_REG_PA_PHSH              0x1B1  /* A 相有功分段相位校正寄存器 高段 */
#define  RN7302_REG_PB_PHSM              0x1B2  /* B 相有功分段相位校正寄存器 中段 */
#define  RN7302_REG_PB_PHSH              0x1B3  /* B 相有功分段相位校正寄存器 高段 */
#define  RN7302_REG_PC_PHSM              0x1B4  /* C 相有功分段相位校正寄存器 中段 */
#define  RN7302_REG_PC_PHSH              0x1B5  /* C 相有功分段相位校正寄存器 高段 */                            
#define  RN7302_REG_QA_PHSM              0x1B6  /* A 相无功分段相位校正寄存器 中段 */
#define  RN7302_REG_QA_PHSH              0x1B7  /* A 相无功分段相位校正寄存器 高段 */
#define  RN7302_REG_QB_PHSM              0x1B8  /* B 相无功分段相位校正寄存器 中段 */
#define  RN7302_REG_QB_PHSH              0x1B9  /* B 相无功分段相位校正寄存器 高段 */
#define  RN7302_REG_QC_PHSM              0x1BA  /* C 相无功分段相位校正寄存器 中段 */
#define  RN7302_REG_QC_PHSH              0x1BB  /* C 相无功分段相位校正寄存器 高段 */

#define  RN7302_REG_D2FP00      	     0x1B0   /* 自定义功率 PA_PHS 作为低段，该位为中段*/
#define  RN7302_REG_D2FP01      	     0x1B1   /* 自定义功率 PA_PHS 作为低段，该位为中段*/
#define  RN7302_REG_D2FP02      	     0x1B2   /* 自定义功率 PA_PHS 作为低段，该位为中段*/
#define  RN7302_REG_D2FP03      	     0x1B3   /* 自定义功率 PA_PHS 作为低段，该位为中段*/
#define  RN7302_REG_D2FP04      	     0x1B4   /* 自定义功率 PA_PHS 作为低段，该位为中段*/
#define  RN7302_REG_D2FP05      	     0x1B5   /* 自定义功率 PA_PHS 作为低段，该位为中段*/
#define  RN7302_REG_D2FP06      	     0x1B6   /* 自定义功率 PA_PHS 作为低段，该位为中段*/
#define  RN7302_REG_D2FP07      	     0x1B7   /* 自定义功率 PA_PHS 作为低段，该位为中段*/
#define  RN7302_REG_D2FP08      	     0x1B8   /* 自定义功率 PA_PHS 作为低段，该位为中段*/
#define  RN7302_REG_D2FP09      	     0x1B9   /* 自定义功率 PA_PHS 作为低段，该位为中段*/
#define  RN7302_REG_D2FP10      	     0x1BA   /* 自定义功率 PA_PHS 作为低段，该位为中段*/
#define  RN7302_REG_D2FP11      	     0x1BB   /* 自定义功率 PA_PHS 作为低段，该位为中段*/

#define  RN7302_REG_D2FE00      	     0x1BC   /* 自定义功率 PA_PHS 作为低段，该位为中段*/
#define  RN7302_REG_D2FE01      	     0x1BD   /* 自定义功率 PA_PHS 作为低段，该位为中段*/
#define  RN7302_REG_D2FE02      	     0x1BE   /* 自定义功率 PA_PHS 作为低段，该位为中段*/
#define  RN7302_REG_D2FE03      	     0x1BF   /* 自定义功率 PA_PHS 作为低段，该位为中段*/
#define  RN7302_REG_D2FE04      	     0x1C0   /* 自定义功率 PA_PHS 作为低段，该位为中段*/
#define  RN7302_REG_D2FE05      	     0x1C1   /* 自定义功率 PA_PHS 作为低段，该位为中段*/
#define  RN7302_REG_D2FE06      	     0x1C2   /* 自定义功率 PA_PHS 作为低段，该位为中段*/
#define  RN7302_REG_D2FE07      	     0x1C3   /* 自定义功率 PA_PHS 作为低段，该位为中段*/
#define  RN7302_REG_D2FE08      	     0x1C4   /* 自定义功率 PA_PHS 作为低段，该位为中段*/
#define  RN7302_REG_D2FE09      	     0x1C5   /* 自定义功率 PA_PHS 作为低段，该位为中段*/
#define  RN7302_REG_D2FE10      	     0x1C6   /* 自定义功率 PA_PHS 作为低段，该位为中段*/
#define  RN7302_REG_D2FE11      	     0x1C7   /* 自定义功率 PA_PHS 作为低段，该位为中段*/
#define  RN7302_REG_HSDC_CTL             0x1C9   /* 波形缓存输出高速 SPI 接口（HSDC）的配置寄存器 */
#define  RN7302_REG_HSDC_CRC             0x1CA   /* HSDC 接口  crc */
#define  RN7302_REG_DMA_WAVECON2  	     0x1D1   /* 同步采样通道配置寄存器 2*/
#define  RN7302_REG_PHS_UA_WAVE   	     0x1D2   /* 同步采样 UA 通道相位校正寄存器*/
#define  RN7302_REG_PHS_UB_WAVE          0x1D3   /* 同步采样 UB 通道相位校正寄存器*/
#define  RN7302_REG_PHS_UC_WAVE          0x1D4   /* 同步采样 UC 通道相位校正寄存器*/
#define  RN7302_REG_PHS_IA_WAVE          0x1D5   /* 同步采样 IA 通道相位校正寄存器*/
#define  RN7302_REG_PHS_IB_WAVE          0x1D6   /* 同步采样 IB 通道相位校正寄存器*/
#define  RN7302_REG_PHS_IC_WAVE          0x1D7   /* 同步采样 IC 通道相位校正寄存器*/
#define  RN7302_REG_PHS_IN_WAVE          0x1D8   /* 同步采样 IN 通道相位校正寄存器*/
#define  RN7302_REG_GSUA_WAVE            0x1D9   /* 同步采样 UA 通道增益校正寄存器*/
#define  RN7302_REG_GSUB_WAVE            0x1DA   /* 同步采样 UB 通道增益校正寄存器*/
#define  RN7302_REG_GSUC_WAVE            0x1DB   /* 同步采样 UC 通道增益校正寄存器*/
#define  RN7302_REG_GSIA_WAVE            0x1DC   /* 同步采样 IA 通道增益校正寄存器*/
#define  RN7302_REG_GSIB_WAVE            0x1DD   /* 同步采样 IB 通道增益校正寄存器*/
#define  RN7302_REG_GSIC_WAVE            0x1DE   /* 同步采样 IC 通道增益校正寄存器*/
#define  RN7302_REG_GSIN_WAVE            0x1DF   /* 同步采样 IN 通道增益校正寄存器*/
#define  RN7302_REG_DMA_WAVECON3         0x1E5   /* 波形采样通道配置寄存器 3*/
#define  RN7302_REG_LVDCFG               0x1E7   /* 掉电检测阈值配置寄存器*/
#define  RN7302_REG_DeviceID2            0x1FF   /* DeviceID2*/


#define  RN7302_REG_UA_BUFBASE           0x200  /* A ���ѹ���λ��� */
#define  RN7302_REG_UB_BUFBASE           0x280  /* B ���ѹ���λ��� */
#define  RN7302_REG_UC_BUFBASE           0x300  /* C ���ѹ���λ��� */
#define  RN7302_REG_IA_BUFBASE           0x380  /* A ��������λ��� */
#define  RN7302_REG_IB_BUFBASE           0x400  /* B ��������λ��� */
#define  RN7302_REG_IC_BUFBASE           0x480  /* C ��������λ��� */

#define  RN7302_REG_PA_HW          0x600            /* A 相有功半波有效值 */
#define  RN7302_REG_PB_HW          0x601            /* B 相有功半波有效值 */
#define  RN7302_REG_PC_HW          0x602            /* C 相有功半波有效值 */
#define  RN7302_REG_QA_HW          0x603            /* A 相无功半波有效值 */
#define  RN7302_REG_QB_HW          0x604            /* B 相无功半波有效值 */
#define  RN7302_REG_QC_HW          0x605            /* C 相无功半波有效值 */

//===============================�Ĵ�����С=============================================================================
/*
    bank 0.
*/
#define  RN7302_LEN_UAWAV       0x03            /* A ���ѹ�������� */
#define  RN7302_LEN_UBWAV       0x03            /* B ���ѹ�������� */
#define  RN7302_LEN_UCWAV       0x03            /* C ���ѹ�������� */
#define  RN7302_LEN_IAWAV       0x03            /* A ������������� */
#define  RN7302_LEN_IBWAV       0x03            /* B ������������� */
#define  RN7302_LEN_ICWAV       0x03            /* C ������������� */
#define  RN7302_LEN_INWAV       0x03            /* ���ߵ����������� */
#define  RN7302_LEN_UA          0x04            /* A ���ѹ */
#define  RN7302_LEN_UB          0x04            /* B ���ѹ */
#define  RN7302_LEN_UC          0x04            /* C ���ѹ */
#define  RN7302_LEN_USUM        0x04            /*   ���ѹʸ���� */
#define  RN7302_LEN_IA          0x04            /* A ����� */
#define  RN7302_LEN_IB          0x04            /* B ����� */
#define  RN7302_LEN_IC          0x04            /* C ����� */
#define  RN7302_LEN_IN          0x04            /*   ���ߵ��� */
/*    reserved    0x0F */                       
#define  RN7302_LEN_ISUM        0x04            /*   �����ʸ���� */
                                                
#define  RN7302_LEN_PA          0x04            /* A ���й����� */
#define  RN7302_LEN_PB          0x04            /* B ���й����� */
#define  RN7302_LEN_PC          0x04            /* C ���й����� */
#define  RN7302_LEN_PT          0x04            /* �����й����� */
#define  RN7302_LEN_QA          0x04            /* A ���޹����� */
#define  RN7302_LEN_QB          0x04            /* B ���޹����� */
#define  RN7302_LEN_QC          0x04            /* C ���޹����� */
#define  RN7302_LEN_QT          0x04            /* �����޹����� */
#define  RN7302_LEN_SA          0x04            /* A �����ڹ��� */
#define  RN7302_LEN_SB          0x04            /* B �����ڹ��� */
#define  RN7302_LEN_SC          0x04            /* C �����ڹ��� */
#define  RN7302_LEN_STA         0x04            /* �������ڹ��� */
#define  RN7302_LEN_PFA         0x03            /* A �๦������ */
#define  RN7302_LEN_PFB         0x03            /* B �๦������ */
#define  RN7302_LEN_PFC         0x03            /* C �๦������ */
#define  RN7302_LEN_PFTA        0x03            /* ���๦������ */
#define  RN7302_LEN_PAFCnt      0x03            /* A ���й������������ */
#define  RN7302_LEN_PBFCnt      0x03            /* B ���й������������ */
#define  RN7302_LEN_PCFCnt      0x03            /* C ���й������������ */
#define  RN7302_LEN_PTFCnt      0x03            /* �����й������������ */
#define  RN7302_LEN_QAFCnt      0x03            /* A ���޹������������ */
#define  RN7302_LEN_QBFCnt      0x03            /* B ���޹������������ */
#define  RN7302_LEN_QCFCnt      0x03            /* C ���޹������������ */
#define  RN7302_LEN_QTFCnt      0x03            /* �����޹������������ */
#define  RN7302_LEN_SAFCnt      0x03            /* A �����ڿ���������� */
#define  RN7302_LEN_SBFCnt      0x03            /* B �����ڿ���������� */
#define  RN7302_LEN_SCFCnt      0x03            /* C �����ڿ���������� */
#define  RN7302_LEN_STFCnt      0x03            /* �������ڿ���������� */
/* Energy */                                    /*  */
#define  RN7302_LEN_EPA         0x03            /* A ���й����� */
#define  RN7302_LEN_EPB         0x03            /* B ���й����� */
#define  RN7302_LEN_EPC         0x03            /* C ���й����� */
#define  RN7302_LEN_EPT         0x03            /* �����й����� */
#define  RN7302_LEN_POSEPA      0x03            /* A �������й����� */
#define  RN7302_LEN_POSEPB      0x03            /* B �������й����� */
#define  RN7302_LEN_POSEPC      0x03            /* C �������й����� */
#define  RN7302_LEN_POSEPT      0x03            /* ���������й����� */
#define  RN7302_LEN_NEGEPA      0x03            /* A �෴���й����� */
#define  RN7302_LEN_NEGEPB      0x03            /* B �෴���й����� */
#define  RN7302_LEN_NEGEPC      0x03            /* C �෴���й����� */
#define  RN7302_LEN_NEGEPT      0x03            /* ���෴���й����� */
#define  RN7302_LEN_EQA         0x03            /* A ���޹����� */
#define  RN7302_LEN_EQB         0x03            /* B ���޹����� */
#define  RN7302_LEN_EQC         0x03            /* C ���޹����� */
#define  RN7302_LEN_EQT         0x03            /* �����޹����� */
#define  RN7302_LEN_POSEQA      0x03            /* A �������޹����� */
#define  RN7302_LEN_POSEQB      0x03            /* B �������޹����� */
#define  RN7302_LEN_POSEQC      0x03            /* C �������޹����� */
#define  RN7302_LEN_POSEQT      0x03            /* ���������޹����� */
#define  RN7302_LEN_NEGEQA      0x03            /* A �෴���޹����� */
#define  RN7302_LEN_NEGEQB      0x03            /* B �෴���޹����� */
#define  RN7302_LEN_NEGEQC      0x03            /* C �෴���޹����� */
#define  RN7302_LEN_NEGEQT      0x03            /* ���෴���޹����� */
#define  RN7302_LEN_ESA         0x03            /* A �����ڵ��� */
#define  RN7302_LEN_ESB         0x03            /* B �����ڵ��� */
#define  RN7302_LEN_ESC         0x03            /* C �����ڵ��� */
#define  RN7302_LEN_EST         0x03            /* �������ڵ��� */
#define  RN7302_LEN_STV         0x04            /* PQS �������ڹ��� */
#define  RN7302_LEN_PFTV        0x03            /* PQS ���๦������ */
#define  RN7302_LEN_STFVCNT     0x03            /* PQS �������ڿ���������� */
#define  RN7302_LEN_ESTV        0x03            /* PQS �������������Ĵ��� */
                                                
/* BASE Wav */                                  
#define  RN7302_LEN_YUA         0x03            /* A ��U������� */
#define  RN7302_LEN_YUB         0x03            /* B ��U������� */
#define  RN7302_LEN_YUC         0x03            /* C ��U������� */
#define  RN7302_LEN_YIA         0x03            /* A ��I������� */
#define  RN7302_LEN_YIB         0x03            /* B ��I������� */
#define  RN7302_LEN_YIC         0x03            /* C ��I������� */
#define  RN7302_LEN_YIN         0x03            /* N ��I������� */
#define  RN7302_LEN_UFREQ       0x03            /* ��ѹ��Ƶ�� */
                                                
//modify by longzh--------------                
#define  RN7302_LEN_FUA         0x04            /* A�������ѹ��Чֵ */
#define  RN7302_LEN_FUB         0x04            /* B�������ѹ��Чֵ */
#define  RN7302_LEN_FUC         0x04            /* C�������ѹ��Чֵ */
#define  RN7302_LEN_FIA         0x04            /* A�����������Чֵ */
#define  RN7302_LEN_FIB         0x04            /* B�����������Чֵ */
#define  RN7302_LEN_FIC         0x04            /* C�����������Чֵ */
#define  RN7302_LEN_FPA         0x04 
#define  RN7302_LEN_FPB         0x04 
#define  RN7302_LEN_FPC         0x04 
#define  RN7302_LEN_FPT         0x04 
#define  RN7302_LEN_FQA         0x04 
#define  RN7302_LEN_FQB         0x04 
#define  RN7302_LEN_FQC         0x04 
#define  RN7302_LEN_FQT         0x04 
#define  RN7302_LEN_FSA         0x04 
#define  RN7302_LEN_FSB         0x04 
#define  RN7302_LEN_FSC         0x04 
#define  RN7302_LEN_FSTA        0x04 
#define  RN7302_LEN_FPFA        0x03 
#define  RN7302_LEN_FPFB        0x03 
#define  RN7302_LEN_FPFC        0x03 
#define  RN7302_LEN_FPFTA       0x03 

#define  RN7302_LEN_FPAFCnt     0x03 
#define  RN7302_LEN_FPBFCnt     0x03 
#define  RN7302_LEN_FPCFCnt     0x03 
#define  RN7302_LEN_FPTFCnt     0x03 
#define  RN7302_LEN_FQAFCnt     0x03 
#define  RN7302_LEN_FQBFCnt     0x03 
#define  RN7302_LEN_FQCFCnt     0x03 
#define  RN7302_LEN_FQTFCnt     0x03 
#define  RN7302_LEN_FSAFCnt     0x03 
#define  RN7302_LEN_FSBFCnt     0x03 
#define  RN7302_LEN_FSCFCnt     0x03 
#define  RN7302_LEN_FSTAFCnt    0x03 
				
#define  RN7302_LEN_FEPA        0x03 
#define  RN7302_LEN_FEPB        0x03 
#define  RN7302_LEN_FEPC        0x03 
#define  RN7302_LEN_FEPT        0x03 
#define  RN7302_LEN_POSFEPA     0x03 
#define  RN7302_LEN_POSFEPB     0x03 
#define  RN7302_LEN_POSFEPC     0x03 
#define  RN7302_LEN_POSFEPT     0x03 
#define  RN7302_LEN_NEGFEPA     0x03 
#define  RN7302_LEN_NEGFEPB     0x03 
#define  RN7302_LEN_NEGFEPC     0x03 
#define  RN7302_LEN_NEGFEPT     0x03 
				
#define  RN7302_LEN_FEQA        0x03 
#define  RN7302_LEN_FEQB        0x03 
#define  RN7302_LEN_FEQC        0x03 
#define  RN7302_LEN_FEQT        0x03 
#define  RN7302_LEN_POSFEQA     0x03 
#define  RN7302_LEN_POSFEQB     0x03 
#define  RN7302_LEN_POSFEQC     0x03 
#define  RN7302_LEN_POSFEQT     0x03 
#define  RN7302_LEN_NEGFEQA     0x03 
#define  RN7302_LEN_NEGFEQB     0x03 
#define  RN7302_LEN_NEGFEQC     0x03 
#define  RN7302_LEN_NEGFEQT     0x03 
				
#define  RN7302_LEN_FESA        0x03 
#define  RN7302_LEN_FESB        0x03 
#define  RN7302_LEN_FESC        0x03 
#define  RN7302_LEN_FESTA       0x03 

#define  RN7302_LEN_HUa         0x04            /* A�������ѹ��Чֵ */
#define  RN7302_LEN_HUb         0x04            /* B�������ѹ��Чֵ */
#define  RN7302_LEN_HUc         0x04            /* C�������ѹ��Чֵ */
#define  RN7302_LEN_HIa         0x04            /* A�����������Чֵ */
#define  RN7302_LEN_HIb         0x04            /* B�����������Чֵ */
#define  RN7302_LEN_HIc         0x04            /* C�����������Чֵ */


#define  RN7302_LEN_FSTV        0x04
#define  RN7302_LEN_FPFTV       0x03
#define  RN7302_LEN_FSTVFCnt    0x03
#define  RN7302_LEN_FESTV       0x03


//---------------------------------------------------------------------
//RN8306-V3 ?????

#define  RN7302_LEN_UA_HW        0x04              
#define  RN7302_LEN_UB_HW        0x04              
#define  RN7302_LEN_UC_HW        0x04              
#define  RN7302_LEN_IA_HW        0x04              
#define  RN7302_LEN_IB_HW        0x04              
#define  RN7302_LEN_IC_HW        0x04              
#define  RN7302_LEN_UAPEAK_HW    0x02              
#define  RN7302_LEN_UBPEAK_HW    0x02              
#define  RN7302_LEN_UCPEAK_HW    0x02              
#define  RN7302_LEN_IN_HW        0x04              
#define  RN7302_LEN_FUA_WAVE     0x03              
#define  RN7302_LEN_FUB_WAVE     0x03              
#define  RN7302_LEN_FUC_WAVE     0x03              
#define  RN7302_LEN_FIA_WAVE     0x03              
#define  RN7302_LEN_FIB_WAVE     0x03              
#define  RN7302_LEN_FIC_WAVE     0x03              
#define  RN7302_LEN_FIN_WAVE     0x03              
#define  RN7302_LEN_SPL_PA       0x04              
#define  RN7302_LEN_SPL_PB       0x04              
#define  RN7302_LEN_SPL_PC       0x04              
#define  RN7302_LEN_SPL_QA       0x04              
#define  RN7302_LEN_SPL_QB       0x04              
#define  RN7302_LEN_SPL_QC       0x04              
#define  RN7302_LEN_SPL_FPA      0x04              
#define  RN7302_LEN_SPL_FPB      0x04              
#define  RN7302_LEN_SPL_FPC      0x04              
#define  RN7302_LEN_SPL_FQA      0x04              
#define  RN7302_LEN_SPL_FQB      0x04              
#define  RN7302_LEN_SPL_FQC      0x04              
				
				
#define  RN7302_LEN_SAG_UAPEAK   0x02              
#define  RN7302_LEN_SAG_UBPEAK   0x02              
#define  RN7302_LEN_SAG_UCPEAK   0x02              


/*
*        bank 1 .  configure register
*/
#define  RN7302_LEN_HFConst1             0x02    /* ��Ƶ��������Ĵ��� 1 */
#define  RN7302_LEN_HFConst2             0x02    /* ��Ƶ��������Ĵ��� 2 */
#define  RN7302_LEN_ISTART_PS            0x02    /* �й���������������ֵ�Ĵ��� */
#define  RN7302_LEN_ISTART_Q             0x02    /* �޹�����������ֵ�Ĵ��� */
#define  RN7302_LEN_LOSTVOLTT            0x02    /* ʧѹ��ֵ�Ĵ��� */
#define  RN7302_LEN_ZXOT                 0x02    /* ���㷧ֵ�Ĵ��� */
#define  RN7302_LEN_PRTH1L               0x02    /* ��λ�ֶ�У��������ֵ�Ĵ���1 ���� */
#define  RN7302_LEN_PRTH1H               0x02    /* ��λ�ֶ�У��������ֵ�Ĵ���1 ���� */
#define  RN7302_LEN_PRTH2L               0x02    /* ��λ�ֶ�У��������ֵ�Ĵ���2 ���� */
#define  RN7302_LEN_PRTH2H               0x02    /* ��λ�ֶ�У��������ֵ�Ĵ���2 ���� */
                                                 
#define  RN7302_LEN_IRegion3L            0x02    /* ������ֵ�Ĵ���3 ���� */
#define  RN7302_LEN_IRegion3H            0x02    /* ������ֵ�Ĵ���3 ���� */

#define  RN7302_LEN_PHSUA                0x01    /* ����ͨ��Ua��λУ���Ĵ��� */
#define  RN7302_LEN_PHSUB                0x01    /* ����ͨ��Ub��λУ���Ĵ��� */
#define  RN7302_LEN_PHSUC                0x01    /* ����ͨ��Uc��λУ���Ĵ��� */
#define  RN7302_LEN_PHSIA                0x03    /* ����ͨ��Ia��λУ���Ĵ��� */
#define  RN7302_LEN_PHSIB                0x03    /* ����ͨ��Ib��λУ���Ĵ��� */
#define  RN7302_LEN_PHSIC                0x03    /* ����ͨ��Ic��λУ���Ĵ��� */
#define  RN7302_LEN_PHSIN                0x01    /* ����ͨ��In��λУ���Ĵ��� */

#define  RN7302_LEN_EX_PHSUA             0x04    /* ����ͨ��Ua��λУ���Ĵ��� */
#define  RN7302_LEN_EX_PHSUB             0x04    /* ����ͨ��Ub��λУ���Ĵ��� */
#define  RN7302_LEN_EX_PHSUC             0x04    /* ����ͨ��Uc��λУ���Ĵ��� */
#define  RN7302_LEN_EX_PHSIA             0x04    /* ����ͨ��Ia��λУ���Ĵ��� */
#define  RN7302_LEN_EX_PHSIB             0x04    /* ����ͨ��Ib��λУ���Ĵ��� */
#define  RN7302_LEN_EX_PHSIC             0x04    /* ����ͨ��Ic��λУ���Ĵ��� */
#define  RN7302_LEN_EX_PHSIN             0x04    /* ����ͨ��In��λУ���Ĵ��� */

#define  RN7302_LEN_GSUA                 0x02    /* ����ͨ��Uaͨ������Ĵ��� */
#define  RN7302_LEN_GSUB                 0x02    /* ����ͨ��Ubͨ������Ĵ��� */
#define  RN7302_LEN_GSUC                 0x02    /* ����ͨ��Ucͨ������Ĵ��� */
#define  RN7302_LEN_GSIA                 0x02    /* ����ͨ��Iaͨ������Ĵ��� */
#define  RN7302_LEN_GSIB                 0x02    /* ����ͨ��Ibͨ������Ĵ��� */
#define  RN7302_LEN_GSIC                 0x02    /* ����ͨ��Icͨ������Ĵ��� */
#define  RN7302_LEN_GSIN                 0x02    /* ����ͨ��Inͨ������Ĵ��� */
                                                 
#define  RN7302_LEN_DCOS_UA              0x02    /* ����ͨ��Uaֱ��offsetУ���Ĵ��� */
#define  RN7302_LEN_DCOS_UB              0x02    /* ����ͨ��Ubֱ��offsetУ���Ĵ��� */
#define  RN7302_LEN_DCOS_UC              0x02    /* ����ͨ��Ucֱ��offsetУ���Ĵ��� */
#define  RN7302_LEN_DCOS_IA              0x02    /* ����ͨ��Iaֱ��offsetУ���Ĵ��� */
#define  RN7302_LEN_DCOS_IB              0x02    /* ����ͨ��Ibֱ��offsetУ���Ĵ��� */
#define  RN7302_LEN_DCOS_IC              0x02    /* ����ͨ��Icֱ��offsetУ���Ĵ��� */
#define  RN7302_LEN_DCOS_IN              0x02    /* ����ͨ��Inֱ��offsetУ���Ĵ��� */
                                                 
#define  RN7302_LEN_UA_OS                0x02    /* Ua��ЧֵoffsetУ���Ĵ��� */
#define  RN7302_LEN_UB_OS                0x02    /* Ub��ЧֵoffsetУ���Ĵ��� */
#define  RN7302_LEN_UC_OS                0x02    /* Uc��ЧֵoffsetУ���Ĵ��� */
#define  RN7302_LEN_IA_OS                0x02    /* Ia��ЧֵoffsetУ���Ĵ��� */
#define  RN7302_LEN_IB_OS                0x02    /* Ib��ЧֵoffsetУ���Ĵ��� */
#define  RN7302_LEN_IC_OS                0x02    /* Ic��ЧֵoffsetУ���Ĵ��� */
#define  RN7302_LEN_IN_OS                0x02    /* In��ЧֵoffsetУ���Ĵ��� */
                                                 
#define  RN7302_LEN_GPA                  0x02    /* Pa����Ĵ��� */
#define  RN7302_LEN_GPB                  0x02    /* Pb����Ĵ��� */
#define  RN7302_LEN_GPC                  0x02    /* Pc����Ĵ��� */
#define  RN7302_LEN_GQA                  0x02    /* Qa����Ĵ��� */
#define  RN7302_LEN_GQB                  0x02    /* Qb����Ĵ��� */
#define  RN7302_LEN_GQC                  0x02    /* Qc����Ĵ��� */
#define  RN7302_LEN_GSA                  0x02    /* Sa����Ĵ��� */
#define  RN7302_LEN_GSB                  0x02    /* Sb����Ĵ��� */
#define  RN7302_LEN_GSC                  0x02    /* Sc����Ĵ��� */
                                                 
#define  RN7302_LEN_PA_PHSL              0x02    /* Pa�ֶ���λУ���Ĵ��� */
#define  RN7302_LEN_PB_PHSL              0x02    /* Pb�ֶ���λУ���Ĵ��� */
#define  RN7302_LEN_PC_PHSL              0x02    /* Pc�ֶ���λУ���Ĵ��� */
#define  RN7302_LEN_QA_PHSL              0x02    /* Qa�ֶ���λУ���Ĵ��� */
#define  RN7302_LEN_QB_PHSL              0x02    /* Qb�ֶ���λУ���Ĵ��� */
#define  RN7302_LEN_QC_PHSL              0x02    /* Qc�ֶ���λУ���Ĵ��� */
                                                 
#define  RN7302_LEN_PA_OS                0x02    /* Pa OFFSET�Ĵ��� */
#define  RN7302_LEN_PB_OS                0x02    /* Pb OFFSET�Ĵ��� */
#define  RN7302_LEN_PC_OS                0x02    /* Pc OFFSET�Ĵ��� */
#define  RN7302_LEN_QA_OS                0x02    /* Qa OFFSET�Ĵ��� */
#define  RN7302_LEN_QB_OS                0x02    /* Qb OFFSET�Ĵ��� */
#define  RN7302_LEN_QC_OS                0x02    /* Qc OFFSET�Ĵ��� */
                                                 
#define  RN7302_LEN_FUA_OS               0x02    /* Ua ������Чֵoffset�Ĵ��� */
#define  RN7302_LEN_FUB_OS               0x02    /* Ub ������Чֵoffset�Ĵ��� */
#define  RN7302_LEN_FUC_OS               0x02    /* Uc ������Чֵoffset�Ĵ��� */
#define  RN7302_LEN_FIA_OS               0x02    /* Ia ������Чֵoffset�Ĵ��� */
#define  RN7302_LEN_FIB_OS               0x02    /* Ib ������Чֵoffset�Ĵ��� */
#define  RN7302_LEN_FIC_OS               0x02    /* Ic ������Чֵoffset�Ĵ��� */
                                                 
#define  RN7302_LEN_GFPA                 0x02    /* Pa ��������Ĵ��� */
#define  RN7302_LEN_GFPB                 0x02    /* Pb ��������Ĵ��� */
#define  RN7302_LEN_GFPC                 0x02    /* Pc ��������Ĵ��� */
#define  RN7302_LEN_GFQA                 0x02    /* Qa ��������Ĵ��� */
#define  RN7302_LEN_GFQB                 0x02    /* Qb ��������Ĵ��� */
#define  RN7302_LEN_GFQC                 0x02    /* Qc ��������Ĵ��� */
#define  RN7302_LEN_GFSA                 0x02    /* Sa ��������Ĵ��� */
#define  RN7302_LEN_GFSB                 0x02    /* Sb ��������Ĵ��� */
#define  RN7302_LEN_GFSC                 0x02    /* Sc ��������Ĵ��� */
                                                 
#define  RN7302_LEN_FPA_PHS              0x02    /* Pa ����У���Ĵ��� */
#define  RN7302_LEN_FPB_PHS              0x02    /* Pb ����У���Ĵ��� */
#define  RN7302_LEN_FPC_PHS              0x02    /* Pc ����У���Ĵ��� */
#define  RN7302_LEN_FQA_PHS              0x02    /* Qa ����У���Ĵ��� */
#define  RN7302_LEN_FQB_PHS              0x02    /* Qb ����У���Ĵ��� */
#define  RN7302_LEN_FQC_PHS              0x02    /* Qc ����У���Ĵ��� */
                                                 
#define  RN7302_LEN_FPA_OS               0x02    /* Pa ����OFFSET�Ĵ��� */
#define  RN7302_LEN_FPB_OS               0x02    /* Pb ����OFFSET�Ĵ��� */
#define  RN7302_LEN_FPC_OS               0x02    /* Pc ����OFFSET�Ĵ��� */
#define  RN7302_LEN_FQA_OS               0x02    /* Qa ����OFFSET�Ĵ��� */
#define  RN7302_LEN_FQB_OS               0x02    /* Qb ����OFFSET�Ĵ��� */
#define  RN7302_LEN_FQC_OS               0x02    /* Qc ����OFFSET�Ĵ��� */
                                                 
#define  RN7302_LEN_SAGCFG               0x03    /* ��ѹ�ݽ���ֵ */
#define  RN7302_LEN_OVLVL                0x02    /* ��ѹ��ֵ */
#define  RN7302_LEN_OILVL                0x02    /* ������ֵ */
#define  RN7302_LEN_UCHSEL               0x01    /* ��ѹͨ���л� */
                                                  
#define  RN7302_LEN_CFCFG                0x03    /* CF�������üĴ��� */
#define  RN7302_LEN_EMMCFG               0x03    /* EMU���üĴ��� */
#define  RN7302_LEN_EMUCON               0x03    /* EMU���ƼĴ��� */
#define  RN7302_LEN_WSAVECON             0x01    /* ��������д�����ƼĴ��� */
#define  RN7302_LEN_EMMIE                0x02    /* �ж����� */
#define  RN7302_LEN_EMMIF                0x02    /* �жϱ�־ */
#define  RN7302_LEN_PQSign               0x02    /* �й��޹����ʷ���Ĵ��� */
#define  RN7302_LEN_Noload               0x02    /* Ǳ������״̬ */
#define  RN7302_LEN_IRegionS             0x01    /* ��������״̬�Ĵ��� */
#define  RN7302_LEN_PHASES               0x02    /* �ж����� */
#define  RN7302_LEN_CheckSum1            0x03    /* EMM У�������üĴ���У��� */

#define  RN7302_LEN_EMMIE2          	 0x04   
#define  RN7302_LEN_EMMIF2          	 0x04   
#define  RN7302_LEN_EMUCFG3         	 0x04   

#define  RN7302_LEN_NVM1CFG         	 0x01   
#define  RN7302_LEN_SYSCFG          	 0x04   
#define  RN7302_LEN_WREN2           	 0x04  
#define  RN7302_LEN_WREN                 0x01    /* дʹ�ܼĴ��� */
#define  RN7302_LEN_WMSW                 0x01    /* ����ģʽ�л��Ĵ��� */
#define  RN7302_LEN_SOFTRST              0x01     /* ������λ�Ĵ��� */
#define  RN7302_LEN_ADCCFG               0x02    /* EMM У�������üĴ���У��� */
#define  RN7302_LEN_TFEEC_EN             0x01    /* ͨ���л�дʹ�ܼĴ��� */
#define  RN7302_LEN_ICHSEL               0x01    /* ����ͨ���л��Ĵ��� */
#define  RN7302_LEN_MODSEL               0x01    /* ��������/��������ģʽѡ��Ĵ��� */
                                                  
#define  RN7302_LEN_SYSSR                0x02    /* ϵͳ״̬�Ĵ��� */
#define  RN7302_LEN_RData                0x04    /* ��һ��SPI���������� */
#define  RN7302_LEN_WData                0x03    /* ��һ��SPIд������� */
#define  RN7302_LEN_LRBufAddr            0x02    /* ���һ�ζ����λ���ĵ�ַ */

#define  RN7302_LEN_DeviceID        	 0x03 

#define  RN7302_LEN_ZXOTU                0x02    /* ��ѹ�нǼ���Ƶ��ֵ */
#define  RN7302_LEN_AUOTDC_EN            0x02    /* ֱ�� offset �Զ�У��ʹ�� */
#define  RN7302_LEN_ZXOTCFG              0x02    /* ����������ü���־�Ĵ��� */
#define  RN7302_LEN_DMA_WAVECON          0x04   /* 同步采样通道配置寄存器 */
#define  RN7302_LEN_IN_WaveEN            0x02    /* IN ���λ���ʹ�� */

#define  RN7302_LEN_D2FCFG      		 0x02
#define  RN7302_LEN_HFCONST3      		 0x02
										 
#define  RN7302_LEN_OI2_LVL         	 0x02
#define  RN7302_LEN_OI2_CNTT        	 0x01
#define  RN7302_LEN_EMMCFG2         	 0x04
#define  RN7302_LEN_OV2_LVL         	 0x02
#define  RN7302_LEN_OV2_CNTT        	 0x02
										 
#define  RN7302_LEN_CHECKSUM3       	 0x04


#define  RN7302_LEN_PA_PHSM              0x03    /* A ���й��ֶ���λУ���Ĵ��� �ж� */
#define  RN7302_LEN_PA_PHSH              0x03    /* A ���й��ֶ���λУ���Ĵ��� �߶� */
#define  RN7302_LEN_PB_PHSM              0x03    /* B ���й��ֶ���λУ���Ĵ��� �ж� */
#define  RN7302_LEN_PB_PHSH              0x03    /* B ���й��ֶ���λУ���Ĵ��� �߶� */
#define  RN7302_LEN_PC_PHSM              0x03    /* C ���й��ֶ���λУ���Ĵ��� �ж� */
#define  RN7302_LEN_PC_PHSH              0x03    /* C ���й��ֶ���λУ���Ĵ��� �߶� */
                                                  
#define  RN7302_LEN_QA_PHSM              0x03    /* A ���޹��ֶ���λУ���Ĵ��� �ж� */
#define  RN7302_LEN_QA_PHSH              0x03    /* A ���޹��ֶ���λУ���Ĵ��� �߶� */
#define  RN7302_LEN_QB_PHSM              0x03    /* B ���޹��ֶ���λУ���Ĵ��� �ж� */
#define  RN7302_LEN_QB_PHSH              0x03    /* B ���޹��ֶ���λУ���Ĵ��� �߶� */
#define  RN7302_LEN_QC_PHSM              0x03    /* C ���޹��ֶ���λУ���Ĵ��� �ж� */
#define  RN7302_LEN_QC_PHSH              0x03    /* C ���޹��ֶ���λУ���Ĵ��� �߶� */


#define  RN7302_LEN_D2FP00      	     0x03   /* 自定义功率 PA_PHS 作为低段，该位为中段*/
#define  RN7302_LEN_D2FP01      	     0x03   /* 自定义功率 PA_PHS 作为低段，该位为中段*/
#define  RN7302_LEN_D2FP02      	     0x03   /* 自定义功率 PA_PHS 作为低段，该位为中段*/
#define  RN7302_LEN_D2FP03      	     0x03   /* 自定义功率 PA_PHS 作为低段，该位为中段*/
#define  RN7302_LEN_D2FP04      	     0x03   /* 自定义功率 PA_PHS 作为低段，该位为中段*/
#define  RN7302_LEN_D2FP05      	     0x03   /* 自定义功率 PA_PHS 作为低段，该位为中段*/
#define  RN7302_LEN_D2FP06      	     0x03   /* 自定义功率 PA_PHS 作为低段，该位为中段*/
#define  RN7302_LEN_D2FP07      	     0x03   /* 自定义功率 PA_PHS 作为低段，该位为中段*/
#define  RN7302_LEN_D2FP08      	     0x03   /* 自定义功率 PA_PHS 作为低段，该位为中段*/
#define  RN7302_LEN_D2FP09      	     0x03   /* 自定义功率 PA_PHS 作为低段，该位为中段*/
#define  RN7302_LEN_D2FP10      	     0x03   /* 自定义功率 PA_PHS 作为低段，该位为中段*/
#define  RN7302_LEN_D2FP11      	     0x03   /* 自定义功率 PA_PHS 作为低段，该位为中段*/
#define  RN7302_LEN_D2FE00      	     0x03   /* 自定义功率 PA_PHS 作为低段，该位为中段*/
#define  RN7302_LEN_D2FE01      	     0x03   /* 自定义功率 PA_PHS 作为低段，该位为中段*/
#define  RN7302_LEN_D2FE02      	     0x03   /* 自定义功率 PA_PHS 作为低段，该位为中段*/
#define  RN7302_LEN_D2FE03      	     0x03   /* 自定义功率 PA_PHS 作为低段，该位为中段*/
#define  RN7302_LEN_D2FE04      	     0x03   /* 自定义功率 PA_PHS 作为低段，该位为中段*/
#define  RN7302_LEN_D2FE05      	     0x03   /* 自定义功率 PA_PHS 作为低段，该位为中段*/
#define  RN7302_LEN_D2FE06      	     0x03   /* 自定义功率 PA_PHS 作为低段，该位为中段*/
#define  RN7302_LEN_D2FE07      	     0x03   /* 自定义功率 PA_PHS 作为低段，该位为中段*/
#define  RN7302_LEN_D2FE08      	     0x03   /* 自定义功率 PA_PHS 作为低段，该位为中段*/
#define  RN7302_LEN_D2FE09      	     0x03   /* 自定义功率 PA_PHS 作为低段，该位为中段*/
#define  RN7302_LEN_D2FE10      	     0x03   /* 自定义功率 PA_PHS 作为低段，该位为中段*/
#define  RN7302_LEN_D2FE11      	     0x03   /* 自定义功率 PA_PHS 作为低段，该位为中段*/
#define  RN7302_LEN_HSDC_CTL             0x04   /* 波形缓存输出高速 SPI 接口（HSDC）的配置寄存器 */
#define  RN7302_LEN_HSDC_CRC             0x04   /* HSDC 接口  crc */
#define  RN7302_LEN_DMA_WAVECON2  	     0x04   /* 同步采样通道配置寄存器 2*/
#define  RN7302_LEN_PHS_UA_WAVE   	     0x02   /* 同步采样 UA 通道相位校正寄存器*/
#define  RN7302_LEN_PHS_UB_WAVE          0x02   /* 同步采样 UB 通道相位校正寄存器*/
#define  RN7302_LEN_PHS_UC_WAVE          0x02   /* 同步采样 UC 通道相位校正寄存器*/
#define  RN7302_LEN_PHS_IA_WAVE          0x02   /* 同步采样 IA 通道相位校正寄存器*/
#define  RN7302_LEN_PHS_IB_WAVE          0x02   /* 同步采样 IB 通道相位校正寄存器*/
#define  RN7302_LEN_PHS_IC_WAVE          0x02   /* 同步采样 IC 通道相位校正寄存器*/
#define  RN7302_LEN_PHS_IN_WAVE          0x02   /* 同步采样 IN 通道相位校正寄存器*/
#define  RN7302_LEN_GSUA_WAVE            0x02   /* 同步采样 UA 通道增益校正寄存器*/
#define  RN7302_LEN_GSUB_WAVE            0x02   /* 同步采样 UB 通道增益校正寄存器*/
#define  RN7302_LEN_GSUC_WAVE            0x02   /* 同步采样 UC 通道增益校正寄存器*/
#define  RN7302_LEN_GSIA_WAVE            0x02   /* 同步采样 IA 通道增益校正寄存器*/
#define  RN7302_LEN_GSIB_WAVE            0x02   /* 同步采样 IB 通道增益校正寄存器*/
#define  RN7302_LEN_GSIC_WAVE            0x02   /* 同步采样 IC 通道增益校正寄存器*/
#define  RN7302_LEN_GSIN_WAVE            0x02   /* 同步采样 IN 通道增益校正寄存器*/
#define  RN7302_LEN_DMA_WAVECON3         0x04   /* 波形采样通道配置寄存器 3*/
#define  RN7302_LEN_LVDCFG               0x04   /* 掉电检测阈值配置寄存器*/
#define  RN7302_LEN_DeviceID2            0x03   /* DeviceID2*/


#define  RN7302_LEN_UA_BUFBASE           0x200  /* A ���ѹ���λ��� */
#define  RN7302_LEN_UB_BUFBASE           0x280  /* B ���ѹ���λ��� */
#define  RN7302_LEN_UC_BUFBASE           0x300  /* C ���ѹ���λ��� */
#define  RN7302_LEN_IA_BUFBASE           0x380  /* A ��������λ��� */
#define  RN7302_LEN_IB_BUFBASE           0x400  /* B ��������λ��� */
#define  RN7302_LEN_IC_BUFBASE           0x480  /* C ��������λ��� */




//===========================END OF �Ĵ�������=======================================================================================

#define  RN7302_REG_WMSW_GOSLM                 0x18   	/* ����ģʽ�л�EMM->SLM */
#define  RN7302_REG_WMSW_GOWMM                 0xA2   	/* ����ģʽ�л�SLM->EMM */
#define  RN7302_REG_TFEEC_EN_DEFAULT           0x00   	/* ͨ���л����ܹر� */
#define  RN7302_REG_TFEEC_EN_ENABLE            0x42   	/* ͨ���л��Ĵ���дʹ����Ч */
#define  RN7302_REG_ICHSEL_DEFAULT             0x00   	/* ����ͨ��Ĭ�ϲ��л� */
#define  RN7302_REG_ICHSEL_ENABLE              0x57   	
#define  RN7302_REG_UCHSEL_DEFAULT             0x00   /* �1�7�1�7�1�7�1�7�0�0�1�7�1�7�0�8�1�7�0�0�1�7�1�7�݄1�7 */
#define  RN7302_REG_MODSEL_3D                  0x33   	/* ��������ģʽ */
#define  RN7302_REG_MODSEL_4Y                  0x00   	/* ��������ģʽ */
#define  RN7302_REG_WSAVECON_WSCOMM            (1<<4)   /* �������������� */
#define  RN7302_REG_EMUCON_PRUN                (0x07)        /* EMU����C/B/A���й�����ʹ��λ */
#define  RN7302_REG_EMUCON_QRUN                (0x07<<4)     /* EMU����C/B/A���޹�����ʹ��λ */
#define  RN7302_REG_EMUCON_SRUN                (0x07<<8)     /* EMU����C/B/A�����ڼ���ʹ��λ */
#define  RN7302_REG_EMUCON_FPRUN               (0x07<<12)    /* EMU����C/B/A������й�����ʹ��λ */
#define  RN7302_REG_EMUCON_FQRUN               (0x07<<16)    /* EMU����C/B/A������޹�����ʹ��λ */
#define  RN7302_REG_EMUCON_FSRUN               (0x07<<20)    /* EMU����C/B/A��������ڼ���ʹ��λ */


#define  RN7302_REG_CFCFG_DEFAULT               0x043210     /* CF�������üĴ��� */
#define  RN7302_REG_CFCFG_CF1CFG                0x00     /* CF��������,BIT0-2,CF1����������λ,���ȫ���й����� */
#define  RN7302_REG_CFCFG_CF1CONST              (0<<3)   /* CF��������,BIT3,CF1��Ƶ����ѡ�� =0����HFCONST1Ϊ��Ƶ���峣��������塣=1,��HFCONST2Ϊ��Ƶ���峣��������塣 */
#define  RN7302_REG_CFCFG_CF2CFG                (1<<4)   /* CF��������,BIT4-6,CF2����������λ,���ȫ���޹����� */
#define  RN7302_REG_CFCFG_CF2CONST              (0<<7)   /* CF��������,BIT7,CF2��Ƶ����ѡ�� =0����HFCONST1Ϊ��Ƶ���峣��������塣=1,��HFCONST2Ϊ��Ƶ���峣��������塣 */
#define  RN7302_REG_CFCFG_CF3CFG                (1<<9)   /* CF��������,BIT8-10,CF3����������λ,���ȫ���������� */
#define  RN7302_REG_CFCFG_CF3CONST              (0<<11)   /* CF��������,BIT11,CF3��Ƶ����ѡ�� =0����HFCONST1Ϊ��Ƶ���峣��������塣=1,��HFCONST2Ϊ��Ƶ���峣��������塣 */
#define  RN7302_REG_CFCFG_RESERVED              (0x43<<12)   /* CF��������,BIT12-19,CF3��Ƶ����ѡ�� =0����HFCONST1Ϊ��Ƶ���峣��������塣=1,��HFCONST2Ϊ��Ƶ���峣��������塣 */
#define  RN7302_REG_CFCFG_CFSCFG                (0<<20)      /* CF��������,BIT20,����Ϊ�������������ʱ��Ч��=0,CFѡ��ĺ�����������ΪRMS�͡� =1,CFѡ��ĺ�����������ΪPQS�͡�*/
#define  RN7302_REG_CFCFG_CF1CFG_FP             0x043773  
#define  RN7302_REG_CFCFG_CF1CFG_FQ             0x043774  
#define  RN7302_REG_CFCFG_CF1CFG_P              0x043770  
#define  RN7302_REG_CFCFG_CF1CFG_Q              0x043771  
#define  RN7302_REG_CFCFG_DISABLE               0x043777  


#define  RN7302_REG_EMUCFG_DEFAULT              0x400000   /* EMU���üĴ��� */
#define  RN7302_REG_EMUCFG_TFEEC                0x420000   
#define  RN7302_REG_EMUCFG_HPFOFF               0x00   	  	/* EMU���üĴ���,BIT0-6,����ͨ��{IN,IC,IB,IA,UC,UB,UA}��ͨʹ��ѡ��λ��=0,ѡ���ͨʹ�ܣ�=1,ѡ���ͨ��ֹ�� */
#define  RN7302_REG_EMUCFG_NOLOADCFG            (0<<16)    /* EMU���üĴ���,BIT16,������ʽѡ��=0,ȫ��������ȫ��������Чֵ��������ֵ���Ƚ��жϡ�=1,ȫ�������Ի���������Чֵ��������ֵ���Ƚ� */
#define  RN7302_REG_EMUCFG_ISUMMOD              (0<<18)    /* EMU���üĴ���,BIT18,����ʸ���ͼ��㷽ʽѡ��=0,���ߵ�����������㣻=1,���ߵ����������㡣*/
#define  RN7302_REG_EMUCFG_EREGCAR              (0<<19)    /* EMU���üĴ���,BIT19,�����Ĵ�����ѡ��=0,���������͡�=1,�ۼ��͡�*/
#define  RN7302_REG_EMUCFG_EPADDMOD             (0<<20)    /* EMU���üĴ���,BIT20,�����й������ۼӷ�ʽѡ��=0,�������ۼӷ�ʽ��=1,����ֵ�ۼӷ�ʽ��*/
#define  RN7302_REG_EMUCFG_EQADDMOD             (0<<21)    /* EMU���üĴ���,BIT21,�����޹������ۼӷ�ʽѡ��=0,�������ۼӷ�ʽ��=1,����ֵ�ۼӷ�ʽ��*/
#define  RN7302_REG_EMUCFG_RESERVED             (1<<22)    /* EMU���üĴ���,BIT22-23,Ĭ��ֵ2��b01��*/


#define  RN7302_CMD_WR_EN                0xE5       		/*дʹ������*/
#define  RN7302_CMD_WR_DIS               0xDC       		/*д��������*/
/*  RN7302_REG_SOFTRST      software reset */
#define  RN7302_CMD_SOFTRST              0xFA
#define  RN7302_CMD_WREN2                0xE500       		
#define  RN7302_CMD_WRDIS2               0xDC00       		
#define  RN7302_REG_EMMCFG2_DEFAULT      0x04   			
#define  RN7302_REG_EMMCFG2_HW           0xE0000008     /* 配置基波/全波有功、无功的SPL复用半波数据 */
#define  RN7302_REG_EMUCFG3_HWEN         0x000001
#define  RN7302_REG_EMUCFG3_FREQ         0x200010       /* 配置夹角寄存器为1周波更新（约20ms） */

//----------------------------------
#define  RN7302_READ_REG(a)             a
#define  RN7302_WRITE_REG(a)            ((a) | 0x80)


//-----------------------------
#define  RN7302_REG_SIZE_MAX            (4)
/*
    WAV read burst
*/
#define  RN7302_BURST_READ_1BYTE        (0 << 2)
#define  RN7302_BURST_READ_4BYTE        (1 << 2)
#define  RN7302_BURST_READ_8BYTE        (2 << 2)
#define  RN7302_BURST_READ_16BYTE       (3 << 2)   

#define  RN7302_MAX_TX    8
#define  RN7302_MAX_RX    8
#define  RN7302_STARTUP_DELAY 1

/*  RN7302_REG_EMMIE        interrupt */
#define  RN7302_ISR_OVIIE           (1 << 15)
#define  RN7302_ISR_SAGIE           (1 << 14)
#define  RN7302_ISR_IREGION3IE      (1 << 13)
#define  RN7302_ISR_CF3IE           (1 << 10)
#define  RN7302_ISR_CF2IE           (1 << 9)
#define  RN7302_ISR_CF1IE           (1 << 8)
#define  RN7302_ISR_ZXINIE          (1 << 7)
#define  RN7302_ISR_ZXICIE          (1 << 6)
#define  RN7302_ISR_ZXIBIE          (1 << 5)
#define  RN7302_ISR_ZXIAIE          (1 << 4)
#define  RN7302_ISR_ZXUCIE          (1 << 3)
#define  RN7302_ISR_ZXUBIE          (1 << 2)
#define  RN7302_ISR_ZXUAIE          (1 << 1)
#define  RN7302_ISR_WAVUDIE         (1 << 0)

#define  RN7302_NUM_WAVSEL          5
#define  RN7302_NUM_PHSEL           3
#define  RN7302_NUM_WAVESRC         ( RN7302_NUM_WAVSEL *  RN7302_NUM_PHSEL)

#define  RN7302_PHASE_A             0
#define  RN7302_PHASE_B             1
#define  RN7302_PHASE_C             2
#define  RN7302_CURRENT             0
#define  RN7302_VOLTAGE             1
#define  RN7302_ACT_PWR             2
#define  RN7302_REACT_PWR           3
#define  RN7302_APP_PWR             4



#define MAX_DATA_SIZE  128 
#define EC    3600  


/* 
  通道相位校正 或者 功率相位校正法 选择 
    0 - 通道相位   推荐
    1 - 功率相位
*/
enum 
{
    PHASE_CHANNEL,
    PHASE_POWER,
};
#define PHASE_ADJUST_SEL              0     


enum 
{
    CURRENT_RATED_C5      =  5,              //  内置5A
    CURRENT_RATED_K100    =  100,            //  外置100A
    CURRENT_RATED_K5      =  (0x8000 | 5),   //  外置5A
};


#define ENERGY_MAXNUM       (99999999999)   // 99999999.999kWh  8+3电能用于存储
#define ENERGY_MAXNUM_8D1   (999999999)     // 99999999.9kWh    8+1电能用于大范围低精度电能通讯
#define ENERGY_MAXNUM_6D3   (999999999)     // 999999.999kWh    6+3电能用于小范围高精度电能通讯



// 5A     采样电阻使用15欧姆，内置2000:1互感器 的正确参数
#define C5_ISTART_VAL_5A       (542)


// 5A     采样电阻使用15欧姆，外置3000:1互感器 的正确参数
#define K5_ISTART_VAL_5A       (542)

// k100a
#define K100_ISTART_VAL_100A   (6250)
 



struct db_database_measure_base_type;

void Rn7302Init(struct db_database_measure_base_type *pstVmem);
int32_t Rn7302Adjust(struct db_database_measure_base_type *pstVmem);
int32_t Rn7302Adjust_Offset_I(struct db_database_measure_base_type *pstVmem);
int32_t Rn7302CheckProcess(void *pargs);
uint32_t Rn7302_Config(struct db_database_measure_base_type *pMeasureBase, int32_t flg, int32_t num);
void Rn7302HFConstSet(void *pargs);
void rn7302_waveform_init(void);
void Rn7302ADCCFGSet(void *pargs);
void Rn7302PHSIxSet(void *pargs);
int32_t Rn7302AdjustI0(struct db_database_measure_base_type *pstVmem);

#ifdef  __cplusplus
}
#endif

#endif

