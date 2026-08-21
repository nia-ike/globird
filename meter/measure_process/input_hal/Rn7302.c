/*************************************************
  Copyright (C), 2003-2017, pilot Co., Ltd.
  File name:      Rn7302.h
  Author:       longzh    Version: 1.0       Date: 2017-6-9
  Description:  Rn7302 ����
  Others:         
  History:              
*************************************************/
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h> 
#include "math.h"
#include "Rn7302.h"
#include "Rn7302_read.h"
#include "Rn7302_adjust.h"
#include "Rn7302File.h"
#include <sys/time.h>
#include "debug.h"
#include "db_struct.h"
#include "algorithm_phasecheck.h"
#include "msr_process.h"
#include "algorithm_energy_remain.h"

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#define    READ_ADJUST_CNT_MAX  10
#define    PQ_DEAD_ZONE         5     //�й��޹���������
#define    VOLTAGE_TIMES        100
#define    CURRENT_TIMES        10000
#define    PI                   3.1415926
#define    PLUSE_CONSTANT       3600



static void Rn7302_SOFTWARE_RST(void *pargs);
static uint16_t Pluse_Checked; 
static uint16_t Pluse_Const;
static uint16_t Pluse_Mode;
static uint16_t Rn7302_Init_State = 0;

/*************************************************************************/
/** Function Name:     set_rn7302_init_state                            **/
/** Purpose:           防止RN7302初始化未完成时刷新电参量                 **/
/**                    导致电参量读取为0，造成告警误动                    **/
/** Params:            None.                                            **/
/** Return:            0:正在初始化 1:初始化完成                          **/
/** Notice:            None.                                            **/
/*************************************************************************/
static uint16_t set_rn7302_init_state(uint16_t state)
{
    if((0 != state) && (1 != state))
    {
        state = 1;
    }

    Rn7302_Init_State = state;

    return Rn7302_Init_State;
}



/**
 * @brief : This function will use to wait wakeup complete
 * @param : void *pargs
 * @return: None      
 * @note  : None
 */
static int rn7302_wait_wakeup_complete(void)
{
    #define RN7302_EMM     (0x01) 
    #define RN7302_SLM     (0x03)    
    #define TIMEOUT_WAIT   (0x06)
    int  timeout = 0;
    int  ret = -1;
    while (timeout < TIMEOUT_WAIT)
    {
        MSR_PORT_DELAY(500);
        int32_t tmp = RN7302_SLM;
        MeterChipReadReg(RN7302_REG_WMSW, RN7302_LEN_WMSW, &tmp); 
        if(RN7302_EMM == tmp)
        {
            ret = F_OK;
            break;
        }
        timeout++;
    } 
    return ret;
}

/*************************************************************************/
/** Function Name:                                                      **/
/** Purpose:                                                            **/
/** Params:            None.                                            **/
/** Return:            None.                                            **/
/** Notice:            None.                                            **/
/*************************************************************************/
static void Rn7302_CFCFG_Init(struct db_cfg_user_para *pCfgUser)
{
    int32_t tmp;

    // 脉冲灯独占，一直开着
    if(pCfgUser->pulseout.pulse[0].mode.value == 0)
    {
        if(get_ICHSEL_flag() == ICHSEL_OFF)
        {
            tmp = RN7302_REG_CFCFG_CF1CFG_P;      //全波有功脉冲
        }
        else
        {
            tmp = RN7302_REG_CFCFG_CF1CFG_FP;     //基波有功脉冲
        }
    }
    else
    {
        if(get_ICHSEL_flag() == ICHSEL_OFF)
        {
            tmp = RN7302_REG_CFCFG_CF1CFG_Q;      //全波无功脉冲
        }
        else
        {
            tmp = RN7302_REG_CFCFG_CF1CFG_FQ;     //基波无功脉冲
        }
    }
    MeterChipWriteReg(RN7302_REG_CFCFG, RN7302_LEN_CFCFG, tmp);  

}

/*************************************************************************/
/** Function Name:                                                      **/
/** Purpose:                                                            **/
/** Params:            None.                                            **/
/** Return:            None.                                            **/
/** Notice:            None.                                            **/
/*************************************************************************/
static void Rn7302_ICHSEL_Init(struct db_cfg_user_para *pCfgUser)
{
    int32_t tmp;
    uint16_t    phase[3]={0,0,0};

    phase[0] = pCfgUser->base.PhaseAdjust1.value;
    phase[1] = pCfgUser->base.PhaseAdjust2.value;
    phase[2] = pCfgUser->base.PhaseAdjust3.value;

    if((1 == phase[0]) && (2 == phase[1]) && (3 == phase[2]))//1、2、3分别对应ICHSEL电流切换的A/B/C的bit位
    {
        set_ICHSEL_flag(ICHSEL_OFF);

        //配置为有功无功为代数和,关闭通道切换配置位
        tmp = RN7302_REG_EMUCFG_DEFAULT ;
        MeterChipWriteReg(RN7302_REG_EMMCFG, RN7302_LEN_EMMCFG, tmp);

        //关闭电流通道切换写使能
        tmp = RN7302_REG_TFEEC_EN_DEFAULT;
        MeterChipWriteReg(RN7302_REG_TFEEC_EN, RN7302_LEN_TFEEC_EN, tmp);

        tmp = RN7302_REG_ICHSEL_DEFAULT;
        MeterChipWriteReg(RN7302_REG_ICHSEL, RN7302_LEN_ICHSEL, tmp);
    }
    else
    {
        set_ICHSEL_flag(ICHSEL_ON);

        //配置为有功无功为代数和,打开通道切换配置位
        tmp = RN7302_REG_EMUCFG_TFEEC;
        MeterChipWriteReg(RN7302_REG_EMMCFG, RN7302_LEN_EMMCFG, tmp);

        //打开电流通道切换写使能
        tmp = RN7302_REG_TFEEC_EN_ENABLE;
        MeterChipWriteReg(RN7302_REG_TFEEC_EN, RN7302_LEN_TFEEC_EN, tmp);
		MeterChipReadReg(RN7302_REG_TFEEC_EN, RN7302_LEN_TFEEC_EN, &tmp);

        tmp = ((phase[0] & 0x3F)) | ((phase[1] & 0x3F) << 2) | ((phase[2] & 0x3F) << 4);
        MeterChipWriteReg(RN7302_REG_ICHSEL, RN7302_LEN_ICHSEL, tmp);
		
		MeterChipReadReg(RN7302_REG_ICHSEL, RN7302_LEN_ICHSEL, &tmp);
    }
}

/*************************************************************************/
/** Function Name:                                                      **/
/** Purpose:           UA:0x0001   UB:0x0002    UC:0x0003               **/
/** Params:            None.                                            **/
/** Return:            None.                                            **/
/** Notice:            None.                                            **/
/*************************************************************************/
static void Rn7302_UCHSEL_Init(int16_t phase1, int16_t phase2, int16_t phase3)
{
    int tmp;

    if(((0 == phase1) && (0 == phase2) && (0 == phase3)) || ((1 == phase1) && (2 == phase2) && (3 == phase3))) //1、2、3分别对应ICHSEL电流切换的A/B/C的bit位
    {
        set_ICHSEL_flag(ICHSEL_OFF);
        tmp = RN7302_CMD_WR_EN;
        MeterChipWriteReg(RN7302_REG_WREN, RN7302_LEN_WREN, tmp); 

        tmp = RN7302_CMD_WREN2;
        MeterChipWriteReg(RN7302_REG_WREN2, RN7302_LEN_WREN2, tmp); 

        //配置为有功无功为代数和,关闭通道切换配置位
        tmp = RN7302_REG_EMUCFG_DEFAULT;
        MeterChipWriteReg(RN7302_REG_EMMCFG, RN7302_LEN_EMMCFG, tmp);

        //关闭电流通道切换写使能
        tmp = RN7302_REG_TFEEC_EN_DEFAULT;
        MeterChipWriteReg(RN7302_REG_TFEEC_EN, RN7302_LEN_TFEEC_EN, tmp);

        tmp = RN7302_REG_ICHSEL_DEFAULT;
        MeterChipWriteReg(RN7302_REG_ICHSEL, RN7302_LEN_ICHSEL, tmp);
        tmp = RN7302_REG_UCHSEL_DEFAULT;
        MeterChipWriteReg(RN7302_REG_UCHSEL, RN7302_LEN_UCHSEL, tmp);
    }
    else 
    {
        set_ICHSEL_flag(ICHSEL_ON);
        tmp = RN7302_CMD_WR_EN;
        MeterChipWriteReg(RN7302_REG_WREN, RN7302_LEN_WREN, tmp); 

        tmp = RN7302_CMD_WREN2;
        MeterChipWriteReg(RN7302_REG_WREN2, RN7302_LEN_WREN2, tmp); 

        //配置为有功无功为代数和,打开通道切换配置位
        tmp = RN7302_REG_EMUCFG_TFEEC;
        MeterChipWriteReg(RN7302_REG_EMMCFG, RN7302_LEN_EMMCFG, tmp);

        //打开电流通道切换写使能
        tmp = RN7302_REG_TFEEC_EN_ENABLE;
        MeterChipWriteReg(RN7302_REG_TFEEC_EN, RN7302_LEN_TFEEC_EN, tmp);

        tmp = ((phase1 & 0x3F)) | ((phase2 & 0x3F) << 2) | ((phase3 & 0x3F) << 4);
        MeterChipWriteReg(RN7302_REG_UCHSEL, RN7302_LEN_UCHSEL, tmp);

    }
}


/*************************************************************************/
/** Function Name:                                                      **/
/** Purpose:           IA:0x0001   IB:0x0002    IC:0x0003               **/
/**                   -IA:0x8001  -IB:0x8002   -IC:0x8003               **/
/** Params:            None.                                            **/
/** Return:            None.                                            **/
/** Notice:            None.                                            **/
/*************************************************************************/

static void Rn7302_CHSEL_Init(struct db_cfg_user_para *pCfgUser)
{
    int tmp;
    uint16_t    phase[3]={0,0,0};
    /*只有在63xx或者93xx模式下才有pv和hybird的电压相序调整
    63xx模式下，6301，6311,
    93xx模式下，9301，9311,,9303,9313,9333， 
    */

    phase[0] = pCfgUser->base.PhaseAdjust1.value;
    phase[1] = pCfgUser->base.PhaseAdjust2.value;
    phase[2] = pCfgUser->base.PhaseAdjust3.value;

    Rn7302_UCHSEL_Init(phase[0], phase[1], phase[2]);
}


/*************************************************************************/
/** Function Name: rn7302_waveform_init                                 **/
/** Purpose:  使能 RN8306 V3 HSDC 波形推送。本平台计量芯片为 RN8306,     **/
/**           HSDC(高速 SPI master)接 T113 SPI1 从机,由 spiwave 驱动    **/
/**           采集连续波形。(703 平台的 RN7302 无 HSDC, 不支持)          **/
/** Params:   None.                                                    **/
/** Return:   None.                                                    **/
/** Notice:   必须在每次 Rn7302Init() 后调用: SOFTRST 会将 HSDC_CTL     **/
/**           复位为 0x04(关闭), 需重新配置使能。                        **/
/*************************************************************************/
void rn7302_waveform_init(void)
{
    int32_t tmp;
    
    // 写使能位
    tmp = RN7302_CMD_WR_EN; 
    MeterChipWriteReg(RN7302_REG_WREN, RN7302_LEN_WREN, tmp); 
    
    // 8306 V3 版本专用
    // 波形输出
    /*
        1. WREN2    启动扩展位
        2. SYSCFG   锐能微例程设置。部分位手册未定义   
        3. HSDC_CTL EN=0 关闭(各参数仅在 EN=0 时可配)
        4. WSAVECON SWAVECFG=1 同步采样、WFreqSEL=0 128点/周波、BUFCFG=0
        5. WAVECON  DS_CFG=0 同步采样, 采样率由 WSAVECON 决定
        6. WAVECON3 分块缓存，连续缓存, 关闭
        7. HSDC_CTL 分两步(手册备注: 各参数仅在 EN=0 时可配):
             先 EN=0 写配置 0x0c0c, 再 EN=1 使能 0x0c0d
             = EN|CPHA|CHK_EN|BYTE_INV|EXT_EN (TASO700 产品验证配置):
             CPOL=0 CPHA=1, 4.096Mhz, 无延时连续,
             32位拓展数据(EXT_EN=1, 线上即 little-endian int32),
             字节反序(BYTE_INV=1), 32位累加和校验(CHK_SEL=0, 非CRC16)
             -> 帧长 = 6ch x 64点 x 4B + sum32 4B = 1540B, 每帧自校验
             (与 spiwave 驱动默认 gbe,frame-len = 1540 一致)
             注: CHK_SEL=1(CRC16)模式实测不按手册逐帧追加, 而是每64帧
                 集中吐128B块, 不可用; 0x020b 则是 CPOL/CPHA 写反
        8. WAVECON3 分块缓存，连续缓存, 开启
        9. WAVECON2 使能高通、通道直流OFFSET参与采样、DMA波形增益自动调整、不开启谐波衰减补偿功能
    */
    tmp = (RN7302_CMD_WR_EN << 8 ) | 0xaa;
    MeterChipWriteReg(RN7302_REG_WREN2, RN7302_LEN_WREN2, tmp);
    tmp = 0xEA009A00;
    MeterChipWriteReg(RN7302_REG_SYSCFG, RN7302_LEN_SYSCFG, tmp);
    tmp = 0x00000006;
    MeterChipWriteReg(RN7302_REG_HSDC_CTL, RN7302_LEN_HSDC_CTL, tmp);
    tmp = 0x000000A0;
    MeterChipWriteReg(RN7302_REG_WSAVECON, RN7302_LEN_WSAVECON, tmp);
    tmp = 0x00000000;
    MeterChipWriteReg(RN7302_REG_DMA_WAVECON, RN7302_LEN_DMA_WAVECON, tmp);
    tmp = 0x00000000;
    MeterChipWriteReg(RN7302_REG_DMA_WAVECON3, RN7302_LEN_DMA_WAVECON3, tmp);
    /* EN=0 时写配置: CPHA=1, CHK_EN=0, BYTE_INV, EXT_EN (无校验纯净流) */
    tmp = 0x00000c04;
    MeterChipWriteReg(RN7302_REG_HSDC_CTL, RN7302_LEN_HSDC_CTL, tmp);
    /* EN=1 使能 */
    tmp = 0x00000c05;
    MeterChipWriteReg(RN7302_REG_HSDC_CTL, RN7302_LEN_HSDC_CTL, tmp);

    tmp = 0x00000001;
    MeterChipWriteReg(RN7302_REG_DMA_WAVECON3, RN7302_LEN_DMA_WAVECON3, tmp);
    tmp = 0x00000300;
    MeterChipWriteReg(RN7302_REG_DMA_WAVECON2, RN7302_LEN_DMA_WAVECON2, tmp);
    
    // 写保护
    tmp = RN7302_CMD_WRDIS2;
    MeterChipWriteReg(RN7302_REG_WREN2, RN7302_LEN_WREN2, tmp);


    // 写保护
    tmp = RN7302_CMD_WR_DIS;
    MeterChipWriteReg(RN7302_REG_WREN, RN7302_LEN_WREN, tmp);
}



/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:           使用FIFO模式，读取波形数据                        **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
void rn7302_waveform_start(void)
{
    int32_t tmp;
    
    // 写使能位
    tmp = RN7302_CMD_WR_EN; 
    MeterChipWriteReg(RN7302_REG_WREN, RN7302_LEN_WREN, tmp); 
    
    tmp = 0; 
    MeterChipWriteReg(RN7302_REG_IN_WaveEN, RN7302_LEN_IN_WaveEN, tmp); 
    
//    tmp = 1; 
//    MeterChipWriteReg(RN7302_REG_EMMIF, RN7302_LEN_EMMIF, tmp); 
    // 启动波形采样
    tmp = 0xa0; 
    MeterChipWriteReg(RN7302_REG_WSAVECON, RN7302_LEN_WSAVECON, tmp); 

    // 查询方式，不启用中断
//    tmp = 1; 
//    MeterChipWriteReg(RN7302_REG_EMMIE, RN7302_LEN_EMMIE, tmp); 

    // 写保护
    tmp = RN7302_CMD_WR_DIS;
    MeterChipWriteReg(RN7302_REG_WREN, RN7302_LEN_WREN, tmp);
}




/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
void Rn7302Init(struct db_database_measure_base_type *pMeasureBase)
{

    int32_t tmp;
    int32_t s32Tmp = 0;
    struct db_cfg_user_para            *pCfgUser = &(pMeasureBase->db_cfg.user);
    struct db_cfg_sys_para              *pCfgSys = &(pMeasureBase->db_cfg.sys);

    set_rn7302_init_state(0);

    // 写使能位
    tmp = RN7302_CMD_WR_EN; 
    MeterChipWriteReg(RN7302_REG_WREN, RN7302_LEN_WREN, tmp); 

    // 切换到EMM模式
    tmp = RN7302_REG_WMSW_GOWMM;
    MeterChipWriteReg(RN7302_REG_WMSW, RN7302_LEN_WMSW, tmp); 
    rn7302_wait_wakeup_complete();

    // 软件复位
    tmp = RN7302_CMD_SOFTRST; 
    MeterChipWriteReg(RN7302_REG_SOFTRST, RN7302_LEN_SOFTRST, tmp);
    MSR_PORT_DELAY(100);

    // 写使能位
    tmp = RN7302_CMD_WR_EN; 
    MeterChipWriteReg(RN7302_REG_WREN, RN7302_LEN_WREN, tmp);
    // 计量控制位 ABC相有功、无功、视在、基波有功、基波无功、基波视在计量使能位
    tmp = RN7302_REG_EMUCON_PRUN | RN7302_REG_EMUCON_QRUN | RN7302_REG_EMUCON_SRUN | RN7302_REG_EMUCON_FPRUN | RN7302_REG_EMUCON_FQRUN | RN7302_REG_EMUCON_FSRUN;// ��������λ
    MeterChipWriteReg(RN7302_REG_EMUCON, RN7302_LEN_EMUCON, tmp);

    //模式选择 三相四线
    if((pCfgUser->base.WiringMode.value & 0x03) == P3W3)
    {
        tmp = RN7302_REG_MODSEL_3D;
    }
    else
    {
        tmp = RN7302_REG_MODSEL_4Y;
    }
    MeterChipWriteReg(RN7302_REG_MODSEL, RN7302_LEN_MODSEL, tmp);
    // 脉冲常数
    MeterChipWriteReg(RN7302_REG_HFConst1, RN7302_LEN_HFConst1, pCfgSys->adjust.HfConst1.value);

    // 电压增益初始化
    MeterChipWriteReg(RN7302_REG_GSUA, RN7302_LEN_GSUA, pCfgSys->adjust.GsUa.value);
    MeterChipWriteReg(RN7302_REG_GSUB, RN7302_LEN_GSUB, pCfgSys->adjust.GsUb.value);
    MeterChipWriteReg(RN7302_REG_GSUC, RN7302_LEN_GSUC, pCfgSys->adjust.GsUc.value);

    // 电流增益初始化
    MeterChipWriteReg(RN7302_REG_GSIA, RN7302_LEN_GSIA, pCfgSys->adjust.GsIa.value);
    MeterChipWriteReg(RN7302_REG_GSIB, RN7302_LEN_GSIB, pCfgSys->adjust.GsIb.value);
    MeterChipWriteReg(RN7302_REG_GSIC, RN7302_LEN_GSIC, pCfgSys->adjust.GsIc.value);
    MeterChipWriteReg(RN7302_REG_GSIN, RN7302_LEN_GSIN, pCfgSys->adjust.GsIn.value);


    if(pCfgSys->adjust.Pa_PhsL.value == 0)
    {
        // 通道相位校正
        // 扩展位
        tmp = (RN7302_CMD_WR_EN << 8 ) | 0xaa;
        MeterChipWriteReg(RN7302_REG_WREN2, RN7302_LEN_WREN2, tmp);

        MeterChipWriteReg(RN7302_REG_PHSUA, RN7302_LEN_EX_PHSUA, 0xa7 << 24);
        MeterChipWriteReg(RN7302_REG_PHSUB, RN7302_LEN_EX_PHSUB, 0xa7 << 24);
        MeterChipWriteReg(RN7302_REG_PHSUC, RN7302_LEN_EX_PHSUC, 0xa7 << 24);

        MeterChipWriteReg(RN7302_REG_PHSUA, RN7302_LEN_EX_PHSUA, (0xa7 << 24) | 0x200);
        MeterChipWriteReg(RN7302_REG_PHSUB, RN7302_LEN_EX_PHSUB, (0xa7 << 24) | 0x200);
        MeterChipWriteReg(RN7302_REG_PHSUC, RN7302_LEN_EX_PHSUC, (0xa7 << 24) | 0x200);

        //相位补偿，分3段,PhsxR1，PhsxR2，PhsxR3
        //在某相电流 Ix 增加方向，以 PRTH1H 为 Region1 和 Region2 的分段点，以 PRTH2H 为Region2 和 Region3 的分段点；
        //在某相电流 Ix 减小方向，以 PRTH1L 为 Region1 和 Region2的分段点，以 PRTH2L 为 Region2 和 Region3 的分段点。
        s32Tmp = ((pCfgSys->adjust.Phs_Ia.value & 0x3FF) + (int16_t)pCfgSys->prthvalue.PhsxR1.value) | (((pCfgSys->adjust.Phs_Ia.value >> 10 & 0x3FF) + (int16_t)pCfgSys->prthvalue.PhsxR2.value)<< 10) | ((((pCfgSys->adjust.Phs_Ia.value >> 20) & 0x3FF) + (int16_t)pCfgSys->prthvalue.PhsxR3.value) << 20);
        MeterChipWriteReg(RN7302_REG_PHSIA, RN7302_LEN_EX_PHSIA, s32Tmp);
        s32Tmp = ((pCfgSys->adjust.Phs_Ib.value & 0x3FF) + (int16_t)pCfgSys->prthvalue.PhsxR1.value) | (((pCfgSys->adjust.Phs_Ib.value >> 10 & 0x3FF) + (int16_t)pCfgSys->prthvalue.PhsxR2.value)<< 10) | ((((pCfgSys->adjust.Phs_Ib.value >> 20) & 0x3FF) + (int16_t)pCfgSys->prthvalue.PhsxR3.value) << 20);
        MeterChipWriteReg(RN7302_REG_PHSIB, RN7302_LEN_EX_PHSIB, s32Tmp);
        s32Tmp = ((pCfgSys->adjust.Phs_Ic.value & 0x3FF) + (int16_t)pCfgSys->prthvalue.PhsxR1.value) | (((pCfgSys->adjust.Phs_Ic.value >> 10 & 0x3FF) + (int16_t)pCfgSys->prthvalue.PhsxR2.value)<< 10) | ((((pCfgSys->adjust.Phs_Ic.value >> 20) & 0x3FF) + (int16_t)pCfgSys->prthvalue.PhsxR3.value) << 20);
        MeterChipWriteReg(RN7302_REG_PHSIC, RN7302_LEN_EX_PHSIC, s32Tmp);

        //相位补偿区域设置，PRTHx 为 2 字节无符号数，做比较时，将其与各相全波电流有效值 Ix（x=A，B，C下同）bit27-bit12 进行比较，以作判断
        // 检查除零错误
        if (pCfgSys->adjustAdvalue.Adjust_I.value == 0) {
            printf("Warning: Adjust_I.value is 0, using default value 1 to avoid division by zero\n");
            pCfgSys->adjustAdvalue.Adjust_I.value = 1;
        }
        
        //4000000  2.5A 976
        s32Tmp = (pCfgSys->prthvalue.Prth1L.value * pCfgSys->adjustAdvalue.Adjust_Ia.value / pCfgSys->adjustAdvalue.Adjust_I.value / 10) >> 12;
        MeterChipWriteReg(RN7302_REG_PRTH1L, RN7302_LEN_PRTH1L, s32Tmp);
        //6400000  4A 1562
        s32Tmp = (pCfgSys->prthvalue.Prth1H.value * pCfgSys->adjustAdvalue.Adjust_Ia.value / pCfgSys->adjustAdvalue.Adjust_I.value / 10) >> 12;
        MeterChipWriteReg(RN7302_REG_PRTH1H, RN7302_LEN_PRTH1H, s32Tmp);
        //12800000  8A 3125
        s32Tmp = (pCfgSys->prthvalue.Prth2L.value * pCfgSys->adjustAdvalue.Adjust_Ia.value / pCfgSys->adjustAdvalue.Adjust_I.value / 10) >> 12;
        MeterChipWriteReg(RN7302_REG_PRTH2L, RN7302_LEN_PRTH2L, s32Tmp);
        //16000000  10A 3906
        s32Tmp = (pCfgSys->prthvalue.Prth2H.value * pCfgSys->adjustAdvalue.Adjust_Ia.value / pCfgSys->adjustAdvalue.Adjust_I.value / 10) >> 12;
        MeterChipWriteReg(RN7302_REG_PRTH2H, RN7302_LEN_PRTH2H, s32Tmp);

        MeterChipWriteReg(RN7302_REG_PA_PHSL, RN7302_LEN_PA_PHSL, 0);
        MeterChipWriteReg(RN7302_REG_PB_PHSL, RN7302_LEN_PB_PHSL, 0);
        MeterChipWriteReg(RN7302_REG_PC_PHSL, RN7302_LEN_PC_PHSL, 0);
        MeterChipWriteReg(RN7302_REG_QA_PHSL, RN7302_LEN_QA_PHSL, 0);
        MeterChipWriteReg(RN7302_REG_QB_PHSL, RN7302_LEN_QB_PHSL, 0);
        MeterChipWriteReg(RN7302_REG_QC_PHSL, RN7302_LEN_QC_PHSL, 0);
        //基波
        MeterChipWriteReg(RN7302_REG_FPA_PHS, RN7302_LEN_FPA_PHS, 0);
        MeterChipWriteReg(RN7302_REG_FPB_PHS, RN7302_LEN_FPB_PHS, 0);
        MeterChipWriteReg(RN7302_REG_FPC_PHS, RN7302_LEN_FPC_PHS, 0);
        MeterChipWriteReg(RN7302_REG_FQA_PHS, RN7302_LEN_FQA_PHS, 0);
        MeterChipWriteReg(RN7302_REG_FQB_PHS, RN7302_LEN_FQB_PHS, 0);
        MeterChipWriteReg(RN7302_REG_FQC_PHS, RN7302_LEN_FQC_PHS, 0);
    }
    else
    {
        // 功率相位校正
        //全波
        MeterChipWriteReg(RN7302_REG_PA_PHSL, RN7302_LEN_PA_PHSL, pCfgSys->adjust.Pa_PhsL.value);
        MeterChipWriteReg(RN7302_REG_PB_PHSL, RN7302_LEN_PB_PHSL, pCfgSys->adjust.Pb_PhsL.value);
        MeterChipWriteReg(RN7302_REG_PC_PHSL, RN7302_LEN_PC_PHSL, pCfgSys->adjust.Pc_PhsL.value);
        MeterChipWriteReg(RN7302_REG_QA_PHSL, RN7302_LEN_QA_PHSL, pCfgSys->adjust.Qa_PhsL.value);
        MeterChipWriteReg(RN7302_REG_QB_PHSL, RN7302_LEN_QB_PHSL, pCfgSys->adjust.Qb_PhsL.value);
        MeterChipWriteReg(RN7302_REG_QC_PHSL, RN7302_LEN_QC_PHSL, pCfgSys->adjust.Qc_PhsL.value);
        //基波
        MeterChipWriteReg(RN7302_REG_FPA_PHS, RN7302_LEN_FPA_PHS, pCfgSys->adjust.Pa_PhsL.value);
        MeterChipWriteReg(RN7302_REG_FPB_PHS, RN7302_LEN_FPB_PHS, pCfgSys->adjust.Pb_PhsL.value);
        MeterChipWriteReg(RN7302_REG_FPC_PHS, RN7302_LEN_FPC_PHS, pCfgSys->adjust.Pc_PhsL.value);
        MeterChipWriteReg(RN7302_REG_FQA_PHS, RN7302_LEN_FQA_PHS, pCfgSys->adjust.Qa_PhsL.value);
        MeterChipWriteReg(RN7302_REG_FQB_PHS, RN7302_LEN_FQB_PHS, pCfgSys->adjust.Qb_PhsL.value);
        MeterChipWriteReg(RN7302_REG_FQC_PHS, RN7302_LEN_FQC_PHS, pCfgSys->adjust.Qc_PhsL.value);
    }
    

    
    // 电流等级
    switch(pCfgSys->interface.RatedCurrent.value)
    {
        case CURRENT_RATED_C5  :
            MeterChipWriteReg(RN7302_REG_ISTART_PS, RN7302_LEN_ISTART_PS,  C5_ISTART_VAL_5A);
            MeterChipWriteReg(RN7302_REG_ISTART_Q, RN7302_LEN_ISTART_Q,    C5_ISTART_VAL_5A);
        break;
        case CURRENT_RATED_K100:
            MeterChipWriteReg(RN7302_REG_ISTART_PS, RN7302_LEN_ISTART_PS,  K100_ISTART_VAL_100A);
            MeterChipWriteReg(RN7302_REG_ISTART_Q, RN7302_LEN_ISTART_Q,    K100_ISTART_VAL_100A);
        break;
        default :
            MeterChipWriteReg(RN7302_REG_ISTART_PS, RN7302_LEN_ISTART_PS,  C5_ISTART_VAL_5A);
            MeterChipWriteReg(RN7302_REG_ISTART_Q, RN7302_LEN_ISTART_Q,    C5_ISTART_VAL_5A);
        break;
    }

    // 偏移校正
    MeterChipWriteReg(RN7302_REG_IA_OS, RN7302_LEN_IA_OS,  pCfgSys->adjust.Offset_Ia.value);
    MeterChipWriteReg(RN7302_REG_IB_OS, RN7302_LEN_IB_OS,  pCfgSys->adjust.Offset_Ib.value);
    MeterChipWriteReg(RN7302_REG_IC_OS, RN7302_LEN_IC_OS,  pCfgSys->adjust.Offset_Ic.value);

    //过零阀值
    tmp = 2;
    MeterChipWriteReg(RN7302_REG_ZXOT, RN7302_LEN_ZXOT, tmp);


    Rn7302_CHSEL_Init(pCfgUser);

    Rn7302_CFCFG_Init(pCfgUser);

    // 切换到EMM模式
    tmp = RN7302_REG_WMSW_GOWMM;
    MeterChipWriteReg(RN7302_REG_WMSW, RN7302_LEN_WMSW, tmp); 

    // 清空采样数据缓存区
    tmp = RN7302_REG_WSAVECON_WSCOMM; 
    MeterChipWriteReg(RN7302_REG_WSAVECON, RN7302_LEN_WSAVECON, tmp);

    

    //初始化完成后需要等待2秒。否则，此时读到的电参量为0，会使告警误动
	MSR_PORT_DELAY(1000);
	
    //等待2秒后，读取所有电能寄存器作清零操作
//    RemainEnergyFileNameAdjust(pMeasureBase);

    set_rn7302_init_state(1);

    // 切换到EMM模式
    tmp = (RN7302_CMD_WR_EN << 8 ) | 0xaa;
    MeterChipWriteReg(RN7302_REG_WREN2, RN7302_LEN_WREN2, tmp);
    
    tmp = RN7302_REG_EMMCFG2_HW ;
    MeterChipWriteReg(RN7302_REG_EMMCFG2, RN7302_LEN_EMMCFG2, tmp); 

    tmp = RN7302_REG_EMUCFG3_FREQ ;
    MeterChipWriteReg(RN7302_REG_EMUCFG3, RN7302_LEN_EMUCFG3, tmp); 
    
    tmp = (RN7302_CMD_WR_DIS << 8 ) | 0xaa;
    MeterChipWriteReg(RN7302_REG_WREN2, RN7302_LEN_WREN2, tmp);
    // 写保护
    tmp = RN7302_CMD_WR_DIS;
    MeterChipWriteReg(RN7302_REG_WREN, RN7302_LEN_WREN, tmp);

    /* RN8306 HSDC 连续波形推送：暂关（SPI1 spiwave 连续跟拍未可靠）。
     * 需要时再打开 rn7302_waveform_init()。 */
    // rn7302_waveform_init();
 
    return;
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
void Rn7302HFConstSet(void *pargs)
{
    uint16_t Hfconst1;
    int32_t tmp;
       
    struct db_database_measure_base_type    *pMeasureBase = (struct db_database_measure_base_type *)pargs;
    struct db_cfg_user_para            *pCfgUser = &(pMeasureBase->db_cfg.user);
    struct db_cfg_sys_para              *pCfgSys = &(pMeasureBase->db_cfg.sys);
    if(pCfgUser->pulseout.pulse[0].constant.value < 1)
    {
        pCfgUser->pulseout.pulse[0].constant.value = 3600;
    }
    Pluse_Const = pCfgUser->pulseout.pulse[0].constant.value;
    Pluse_Checked = pCfgUser->pulseout.checked.value;

    /*
        HFConst1 = P 标准*3.6*(10^6)*4096000/(16*电表脉冲常数*额定电压 *标定电流*2^31)    3.6*(10^6)*4096000/16/2^31=429.15344
    */

    Hfconst1 = (uint64_t)pCfgSys->adjustAdvalue.Adjust_Pa.value * 429.15344 / Pluse_Const / pCfgSys->adjustAdvalue.Adjust_V.value / pCfgSys->adjustAdvalue.Adjust_I.value;

    tmp = RN7302_CMD_WR_EN; // 写使能
    MeterChipWriteReg(RN7302_REG_WREN, RN7302_LEN_WREN, tmp); 

    MeterChipWriteReg(RN7302_REG_HFConst1, RN7302_LEN_HFConst1, Hfconst1);
    // 写保护
    tmp = RN7302_CMD_WR_DIS;
    MeterChipWriteReg(RN7302_REG_WREN, RN7302_LEN_WREN, tmp);
    pCfgSys->adjust.HfConst1.value = Hfconst1;

    return;
}
/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int32_t Rn7302_Config_Pluse_check(void *pargs)
{
    struct db_database_measure_base_type    *pstVmem = (struct db_database_measure_base_type *)pargs;
    struct db_cfg_user_para *pCfgUser = &(pstVmem->db_cfg.user);
    struct db_cfg_sys_para *pCfgSys = &(pstVmem->db_cfg.sys);
    int32_t s32Tmp;
    int ret=0;

    ret = MeterChipReadReg(RN7302_REG_HFConst1, RN7302_LEN_HFConst1, &s32Tmp);
    if(ret == -1)
    {
        return -1;
    }
    //常数改变先将系统参数里常数写入校表参数
    if((s32Tmp != pCfgSys->adjust.HfConst1.value) || 
    (Pluse_Const != pCfgUser->pulseout.pulse[0].constant.value) || 
    (Pluse_Checked != pCfgUser->pulseout.checked.value)|| 
    (Pluse_Mode != pCfgUser->pulseout.pulse[0].mode.value))
    {
        Pluse_Mode = pCfgUser->pulseout.pulse[0].mode.value;
        Rn7302HFConstSet(pargs);
        return 1;
    }  

    
    return 0;
}

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
//接线模式,脉冲输出模式,脉冲常数更改时重新初始化
static int32_t Rn7302_Config_Pluse(void *pargs)
{
    struct db_database_measure_base_type    *pMeasureBase = (struct db_database_measure_base_type *)pargs;
    int32_t ret;
    ret = Rn7302_Config_Pluse_check(pMeasureBase);
    if(ret > 0)
    {
        Rn7302Init(pargs);
    }    
    return 0;
}

/*************************************************************************/
/** Function Name:     Rn7302CheckProcess                              **/    
/** Purpose:           检查 RN7302 芯片寄存器是否与配置一致            **/    
/** Params:            pargs - 指向测量基础数据结构的指针               **/    
/** Return:            0: 正常, -1: 寄存器异常（需要重新初始化）        **/    
/** Notice:            连续3次检测到寄存器值不匹配才判定为异常          **/    
/*************************************************************************/
int32_t Rn7302CheckProcess(void *pargs)
{
    struct db_database_measure_base_type *pMeasureBase = (struct db_database_measure_base_type *)pargs;
    struct db_cfg_sys_para *pCfgSys = &(pMeasureBase->db_cfg.sys);
    static int mismatch_count = 0;
    int32_t reg_hfconst1 = 0;
    int32_t reg_gsua = 0;
    int ret;

    /* 读取脉冲常数寄存器 HFConst1 */
    ret = MeterChipReadReg(RN7302_REG_HFConst1, RN7302_LEN_HFConst1, &reg_hfconst1);
    if(ret == -1)
    {
        printf("Rn7302CheckProcess: Failed to read HFConst1 register\n");
        return -1;
    }

    /* 读取 A 相电压增益寄存器 GsUa */
    ret = MeterChipReadReg(RN7302_REG_GSUA, RN7302_LEN_GSUA, &reg_gsua);
    if(ret == -1)
    {
        printf("Rn7302CheckProcess: Failed to read GsUa register\n");
        return -1;
    }

    /* 检查寄存器值是否与配置一致 */
    if((reg_hfconst1 != pCfgSys->adjust.HfConst1.value) || 
       (reg_gsua != pCfgSys->adjust.GsUa.value))
    {
        mismatch_count++;
        
        /* 连续3次检测到不匹配，判定为芯片异常 */
        if(mismatch_count > 3)
        {
            printf("Rn7302CheckProcess: Register mismatch detected %d times, chip needs re-init\n", 
                   mismatch_count);
            printf("  Expected HFConst1=0x%08X, Got=0x%08X\n", 
                   pCfgSys->adjust.HfConst1.value, reg_hfconst1);
            printf("  Expected GsUa=0x%08X, Got=0x%08X\n", 
                   pCfgSys->adjust.GsUa.value, reg_gsua);
            
            mismatch_count = 0;  /* 重置计数器 */
            return -1;
        }
    }
    else
    {
        /* 寄存器值正常，重置计数器 */
        if(mismatch_count > 0)
        {
            mismatch_count = 0;
        }
    }
    
    return 0;
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
void Rn7302ADCCFGSet(void *pargs)
{
    int32_t tmp;
    int32_t s32Tmp = 0;
       
    struct db_database_measure_base_type    *pMeasureBase = (struct db_database_measure_base_type *)pargs;
    struct db_cfg_sys_para              *pCfgSys = &(pMeasureBase->db_cfg.sys);

    // 写使能位
    tmp = RN7302_CMD_WR_EN; 
    MeterChipWriteReg(RN7302_REG_WREN, RN7302_LEN_WREN, tmp); 

    //  In channel  固定 2x GAIN。   5A版本 电流通道增益2倍 100A版本 电流通道增益1倍
    // 其他通道可配
    MeterChipReadReg(RN7302_REG_ADCCFG, RN7302_LEN_ADCCFG, &s32Tmp);
    //电流增益
    if(pCfgSys->adjustAd.Adjust_Gi.value == 2)
    {
        s32Tmp = (s32Tmp & 0xF03F) | 0x0540;
    }
    else if(pCfgSys->adjustAd.Adjust_Gi.value == 4)
    {
        s32Tmp = (s32Tmp & 0xF03F) | 0x0A80;
    }
    else if(pCfgSys->adjustAd.Adjust_Gi.value == 8)
    {
        s32Tmp = (s32Tmp & 0xF03F) | 0x0FC0;
    }
    else
    {
        s32Tmp = (s32Tmp & 0xF03F) | 0x0000;
    }

    //电压增益
    if(pCfgSys->adjustAd.Adjust_Gv.value == 2)
    {
        s32Tmp = (s32Tmp & 0xFFC0) | 0x0015;
    }
    else if(pCfgSys->adjustAd.Adjust_Gv.value == 4)
    {
        s32Tmp = (s32Tmp & 0xFFC0) | 0x002A;
    }
    else if(pCfgSys->adjustAd.Adjust_Gv.value == 8)
    {
        s32Tmp = (s32Tmp & 0xFFC0) | 0x003F;
    }
    else
    {
        s32Tmp = (s32Tmp & 0xFFC0) | 0x0000;
    }

    //漏电电流增益
    if(pCfgSys->adjustI0value.Adjust_Gi0.value == 2)
    {
        s32Tmp = (s32Tmp & 0xCFFF) | 0x1000;
    }
    else if(pCfgSys->adjustI0value.Adjust_Gi0.value == 4)
    {
        s32Tmp = (s32Tmp & 0xCFFF) | 0x2000;
    }
    else if(pCfgSys->adjustI0value.Adjust_Gi0.value == 8)
    {
        s32Tmp = (s32Tmp & 0xCFFF) | 0x3000;
    }
    else
    {
        s32Tmp = (s32Tmp & 0xCFFF) | 0x0000;
    }
    MeterChipWriteReg(RN7302_REG_ADCCFG, RN7302_LEN_ADCCFG,  s32Tmp);

    // 写保护
    tmp = RN7302_CMD_WR_DIS;
    MeterChipWriteReg(RN7302_REG_WREN, RN7302_LEN_WREN, tmp);

    return;
}

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
void Rn7302PHSIxSet(void *pargs)
{
    int32_t tmp;
    int32_t s32Tmp = 0;
       
    struct db_database_measure_base_type    *pMeasureBase = (struct db_database_measure_base_type *)pargs;
    struct db_cfg_sys_para              *pCfgSys = &(pMeasureBase->db_cfg.sys);

    // 写使能位
    tmp = RN7302_CMD_WR_EN; 
    MeterChipWriteReg(RN7302_REG_WREN, RN7302_LEN_WREN, tmp); 

    
    tmp = (RN7302_CMD_WR_DIS << 8 ) | 0xaa;
    MeterChipWriteReg(RN7302_REG_WREN2, RN7302_LEN_WREN2, tmp);
    //相位补偿，分3段,PhsxR1，PhsxR2，PhsxR3
    //在某相电流 Ix 增加方向，以 PRTH1H 为 Region1 和 Region2 的分段点，以 PRTH2H 为Region2 和 Region3 的分段点；
    //在某相电流 Ix 减小方向，以 PRTH1L 为 Region1 和 Region2的分段点，以 PRTH2L 为 Region2 和 Region3 的分段点。
    s32Tmp = ((pCfgSys->adjust.Phs_Ia.value & 0x3FF) + (int16_t)pCfgSys->prthvalue.PhsxR1.value) | (((pCfgSys->adjust.Phs_Ia.value >> 10 & 0x3FF) + (int16_t)pCfgSys->prthvalue.PhsxR2.value)<< 10) | ((((pCfgSys->adjust.Phs_Ia.value >> 20) & 0x3FF) + (int16_t)pCfgSys->prthvalue.PhsxR3.value) << 20);
    MeterChipWriteReg(RN7302_REG_PHSIA, RN7302_LEN_EX_PHSIA, s32Tmp);

    s32Tmp = ((pCfgSys->adjust.Phs_Ib.value & 0x3FF) + (int16_t)pCfgSys->prthvalue.PhsxR1.value) | (((pCfgSys->adjust.Phs_Ib.value >> 10 & 0x3FF) + (int16_t)pCfgSys->prthvalue.PhsxR2.value)<< 10) | ((((pCfgSys->adjust.Phs_Ib.value >> 20) & 0x3FF) + (int16_t)pCfgSys->prthvalue.PhsxR3.value) << 20);
    MeterChipWriteReg(RN7302_REG_PHSIB, RN7302_LEN_EX_PHSIB, s32Tmp);

    s32Tmp = ((pCfgSys->adjust.Phs_Ic.value & 0x3FF) + (int16_t)pCfgSys->prthvalue.PhsxR1.value) | (((pCfgSys->adjust.Phs_Ic.value >> 10 & 0x3FF) + (int16_t)pCfgSys->prthvalue.PhsxR2.value)<< 10) | ((((pCfgSys->adjust.Phs_Ic.value >> 20) & 0x3FF) + (int16_t)pCfgSys->prthvalue.PhsxR3.value) << 20);
    MeterChipWriteReg(RN7302_REG_PHSIC, RN7302_LEN_EX_PHSIC, s32Tmp);

    tmp = (RN7302_CMD_WR_DIS << 8 ) | 0xaa;
    MeterChipWriteReg(RN7302_REG_WREN2, RN7302_LEN_WREN2, tmp);

    MeterChipReadReg(RN7302_REG_IA, RN7302_LEN_IA, &tmp);
    MeterChipReadReg(RN7302_REG_IA, RN7302_LEN_IB, &tmp);
    MeterChipReadReg(RN7302_REG_IA, RN7302_LEN_IC, &tmp);

    //相位补偿区域设置，PRTHx 为 2 字节无符号数，做比较时，将其与各相全波电流有效值 Ix（x=A，B，C下同）bit27-bit12 进行比较，以作判断
    // 检查除零错误
    if (pCfgSys->adjustAdvalue.Adjust_I.value == 0) {
        printf("Warning: Adjust_I.value is 0, using default value 1 to avoid division by zero\n");
        pCfgSys->adjustAdvalue.Adjust_I.value = 1;
    }
    
    //4000000  2.5A 976
    s32Tmp = (pCfgSys->prthvalue.Prth1L.value * pCfgSys->adjustAdvalue.Adjust_Ia.value / pCfgSys->adjustAdvalue.Adjust_I.value / 10) >> 12;
    MeterChipWriteReg(RN7302_REG_PRTH1L, RN7302_LEN_PRTH1L, s32Tmp);
    //6400000  4A 1562
    s32Tmp = (pCfgSys->prthvalue.Prth1H.value * pCfgSys->adjustAdvalue.Adjust_Ia.value / pCfgSys->adjustAdvalue.Adjust_I.value / 10) >> 12;
    MeterChipWriteReg(RN7302_REG_PRTH1H, RN7302_LEN_PRTH1H, s32Tmp);
    //12800000  8A 3125
    s32Tmp = (pCfgSys->prthvalue.Prth2L.value * pCfgSys->adjustAdvalue.Adjust_Ia.value / pCfgSys->adjustAdvalue.Adjust_I.value / 10) >> 12;
    MeterChipWriteReg(RN7302_REG_PRTH2L, RN7302_LEN_PRTH2L, s32Tmp);
    //16000000  10A 3906
    s32Tmp = (pCfgSys->prthvalue.Prth2H.value * pCfgSys->adjustAdvalue.Adjust_Ia.value / pCfgSys->adjustAdvalue.Adjust_I.value / 10) >> 12;
    MeterChipWriteReg(RN7302_REG_PRTH2H, RN7302_LEN_PRTH2H, s32Tmp);

    tmp = (RN7302_CMD_WR_DIS << 8 ) | 0xaa;
    MeterChipWriteReg(RN7302_REG_WREN2, RN7302_LEN_WREN2, tmp);
    
    // 写保护
    tmp = RN7302_CMD_WR_DIS;
    MeterChipWriteReg(RN7302_REG_WREN, RN7302_LEN_WREN, tmp);


    return;
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static void Rn7302_Config_WringMode(void *pargs)
{
    struct db_database_measure_base_type *pMeasureBase = (struct db_database_measure_base_type *)pargs;
    struct db_cfg_user_para         *pCfgUser = &(pMeasureBase->db_cfg.user);
    uint16_t value;    
    int32_t WiringMode = 0;

    if(-1 != MeterChipReadReg(RN7302_REG_MODSEL, RN7302_LEN_MODSEL, &WiringMode))
    {
        if(WiringMode != pCfgUser->base.WiringMode.value)
        {
            value = RN7302_CMD_WR_EN; //写使能位
            MeterChipWriteReg(RN7302_REG_WREN, RN7302_LEN_WREN, value);
            //模式选择 三相四线
            if((pCfgUser->base.WiringMode.value & 0x03) == P3W3)
            {
                value = RN7302_REG_MODSEL_3D;
            }
            else
            {
                value = RN7302_REG_MODSEL_4Y;
            }
            MeterChipWriteReg(RN7302_REG_MODSEL, RN7302_LEN_MODSEL, value);
        }
    }
}




/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static void Rn7302_SOFTWARE_RST(void *pargs)
{
    struct db_database_measure_base_type    *pMeasureBase = (struct db_database_measure_base_type *)pargs;
    struct db_cfg_user_para            *pCfgUser = &(pMeasureBase->db_cfg.user);
    struct db_cfg_sys_para *pCfgSys = &(pMeasureBase->db_cfg.sys);
    int32_t tmp;
    int32_t hfconst1;
    int32_t factorU[3] = {0,0,0},factorI[4] = {0,0,0,0},factorPHS[3] = {0,0,0},factorQHS[3] = {0,0,0};

    if(pCfgUser->pulseout.pulse[0].constant.value < 1)
    {
        pCfgUser->pulseout.pulse[0].constant.value = 3600;
    }
    Pluse_Const = pCfgUser->pulseout.pulse[0].constant.value;

    /*
        HFConst1 = P 标准*3.6*(10^6)*4096000/(16*电表脉冲常数*额定电压 *标定电流*2^31)    3.6*(10^6)*4096000/16/2^31=429.15344
    */
    hfconst1 = (uint64_t)pCfgSys->adjustAdvalue.Adjust_Pa.value * 429.15344 / Pluse_Const / pCfgSys->adjustAdvalue.Adjust_V.value / pCfgSys->adjustAdvalue.Adjust_I.value;
        
    // 写使能位
    tmp = RN7302_CMD_WR_EN; 
    MeterChipWriteReg(RN7302_REG_WREN, RN7302_LEN_WREN, tmp); 

    // 切换到EMM模式
    tmp = RN7302_REG_WMSW_GOWMM;
    MeterChipWriteReg(RN7302_REG_WMSW, RN7302_LEN_WMSW, tmp); 
    MSR_PORT_DELAY(10);
    // 软件复位
    tmp = RN7302_CMD_SOFTRST; 
    MeterChipWriteReg(RN7302_REG_SOFTRST, RN7302_LEN_SOFTRST, tmp);
	MSR_PORT_DELAY(10);
            
    //将默认系数写入CE   
    MeterChipReadReg(RN7302_REG_HFConst1, RN7302_LEN_HFConst1, &hfconst1); 
    MeterChipReadReg(RN7302_REG_GSUA, RN7302_LEN_GSUA, &factorU[0]);
    MeterChipReadReg(RN7302_REG_GSUB, RN7302_LEN_GSUB, &factorU[1]);
    MeterChipReadReg(RN7302_REG_GSUC, RN7302_LEN_GSUC, &factorU[2]);  
    MeterChipReadReg(RN7302_REG_GSIA, RN7302_LEN_GSIA, &factorI[0]);
    MeterChipReadReg(RN7302_REG_GSIB, RN7302_LEN_GSIB, &factorI[1]);
    MeterChipReadReg(RN7302_REG_GSIC, RN7302_LEN_GSIC, &factorI[2]);
    MeterChipReadReg(RN7302_REG_GSIN, RN7302_LEN_GSIN, &factorI[3]);
    MeterChipReadReg(RN7302_REG_PA_PHSL, RN7302_LEN_PA_PHSL, &factorPHS[0]);
    MeterChipReadReg(RN7302_REG_PB_PHSL, RN7302_LEN_PB_PHSL, &factorPHS[1]);
    MeterChipReadReg(RN7302_REG_PC_PHSL, RN7302_LEN_PC_PHSL, &factorPHS[2]);
    MeterChipReadReg(RN7302_REG_QA_PHSL, RN7302_LEN_QA_PHSL, &factorQHS[0]);
    MeterChipReadReg(RN7302_REG_QB_PHSL, RN7302_LEN_QB_PHSL, &factorQHS[1]);
    MeterChipReadReg(RN7302_REG_QC_PHSL, RN7302_LEN_QC_PHSL, &factorQHS[2]);
  
    pCfgSys->adjust.HfConst1.value = hfconst1;
    pCfgSys->adjust.GsUa.value = factorU[0];
    pCfgSys->adjust.GsUb.value = factorU[1];
    pCfgSys->adjust.GsUc.value = factorU[2];
    pCfgSys->adjust.GsIa.value = factorI[0];
    pCfgSys->adjust.GsIb.value = factorI[1];
    pCfgSys->adjust.GsIc.value = factorI[2];
    pCfgSys->adjust.GsIn.value = factorI[3];
    pCfgSys->adjust.I0Adj.value = factorI[3];
    pCfgSys->adjust.Pa_PhsL.value = factorPHS[0];
    pCfgSys->adjust.Pb_PhsL.value = factorPHS[1];
    pCfgSys->adjust.Pc_PhsL.value = factorPHS[2];
    pCfgSys->adjust.Qa_PhsL.value = factorQHS[0];
    pCfgSys->adjust.Qb_PhsL.value = factorQHS[1];
    pCfgSys->adjust.Qc_PhsL.value = factorQHS[2];

}




/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
uint32_t Rn7302_Config(struct db_database_measure_base_type *pMeasureBase, int32_t flg, int32_t num)
{
    switch(flg)
    {
        case MSR_TYPE_RN7302_REINIT:
            Rn7302Init(pMeasureBase);
            break;
            
        case MSR_TYPE_RN7302_GET_INIT_STATE:
            return Rn7302_Init_State;
        case MSR_TYPE_RN7302_ADJUST_RST:
            Rn7302_RST_AdjustFactor(pMeasureBase);
            Rn7302Init(pMeasureBase);
            break;          
    }
    return 0;
}




