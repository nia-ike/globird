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
#include <errno.h>
#include "math.h"
#include "Rn7302.h"
#include "Rn7302_read.h"
#include "Rn7302File.h"
#include <sys/time.h>
#include "debug.h"
#include "db_struct.h"
#include "Rn7302_register.h"
#include "Rn7302_phasecheck.h"

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

static struct rn7302_emu         EmuFull;
static struct rn7302_emu         EmuFund;
static struct rn7302_emu         EmuHalf;

static struct rn7302_emu_energy         EmuFullEnergy;
static struct rn7302_emu_energy         EmuFundEnergy;

/**
 * @brief  单寄存器读取；失败打印并透传 errno
 * @return 0 成功；-1 失败
 */
static int emu_chip_read_reg(uint32_t reg, uint32_t nbyte, int32_t *pval, const char *tag)
{
    int ret;
    int saved_errno;

    ret = MeterChipReadReg(reg, nbyte, pval);
    saved_errno = errno;
    if (ret < 0) {
        printf("[EMU-ERR] %s reg=0x%X ret=%d errno=%d(%s)\n",
               tag, reg, ret, saved_errno, strerror(saved_errno));
        errno = saved_errno;
        return -1;
    }
    return 0;
}

#define EMU_RD(_tag, _dst, _reg, _len) \
    do { \
        if (emu_chip_read_reg((_reg), (_len), (int32_t *)&(_dst), (_tag)) != 0) \
            return -1; \
    } while (0)

/**
 * @brief  读取全波计量寄存器；驱动失败时不发布缓存
 */
int get_EmuFull(struct rn7302_emu **ppEmu)
{
    struct rn7302_emu tmp;

    if (ppEmu != NULL)
        *ppEmu = NULL;

    memset(&tmp, 0, sizeof(tmp));
    EMU_RD("Full", tmp.UA,    RN7302_REG_UA,    RN7302_LEN_UA);
    EMU_RD("Full", tmp.UB,    RN7302_REG_UB,    RN7302_LEN_UB);
    EMU_RD("Full", tmp.UC,    RN7302_REG_UC,    RN7302_LEN_UC);
    EMU_RD("Full", tmp.IA,    RN7302_REG_IA,    RN7302_LEN_IA);
    EMU_RD("Full", tmp.IB,    RN7302_REG_IB,    RN7302_LEN_IB);
    EMU_RD("Full", tmp.IC,    RN7302_REG_IC,    RN7302_LEN_IC);
    EMU_RD("Full", tmp.IN,    RN7302_REG_IN,    RN7302_LEN_IN);
    EMU_RD("Full", tmp.PA,    RN7302_REG_PA,    RN7302_LEN_PA);
    EMU_RD("Full", tmp.PB,    RN7302_REG_PB,    RN7302_LEN_PB);
    EMU_RD("Full", tmp.PC,    RN7302_REG_PC,    RN7302_LEN_PC);
    EMU_RD("Full", tmp.PT,    RN7302_REG_PT,    RN7302_LEN_PT);
    EMU_RD("Full", tmp.QA,    RN7302_REG_QA,    RN7302_LEN_QA);
    EMU_RD("Full", tmp.QB,    RN7302_REG_QB,    RN7302_LEN_QB);
    EMU_RD("Full", tmp.QC,    RN7302_REG_QC,    RN7302_LEN_QC);
    EMU_RD("Full", tmp.QT,    RN7302_REG_QT,    RN7302_LEN_QT);
    EMU_RD("Full", tmp.SA,    RN7302_REG_SA,    RN7302_LEN_SA);
    EMU_RD("Full", tmp.SB,    RN7302_REG_SB,    RN7302_LEN_SB);
    EMU_RD("Full", tmp.SC,    RN7302_REG_SC,    RN7302_LEN_SC);
    EMU_RD("Full", tmp.STA,   RN7302_REG_STA,   RN7302_LEN_STA);
    EMU_RD("Full", tmp.PfA,   RN7302_REG_PFA,   RN7302_LEN_PFA);
    EMU_RD("Full", tmp.PfB,   RN7302_REG_PFB,   RN7302_LEN_PFB);
    EMU_RD("Full", tmp.PfC,   RN7302_REG_PFC,   RN7302_LEN_PFC);
    EMU_RD("Full", tmp.PfTV,  RN7302_REG_PFTV,  RN7302_LEN_PFTV);
    EMU_RD("Full", tmp.UFreq, RN7302_REG_UFREQ, RN7302_LEN_UFREQ);
    EMU_RD("Full", tmp.YUA,   RN7302_REG_YUA,   RN7302_LEN_YUA);
    EMU_RD("Full", tmp.YUB,   RN7302_REG_YUB,   RN7302_LEN_YUB);
    EMU_RD("Full", tmp.YUC,   RN7302_REG_YUC,   RN7302_LEN_YUC);
    EMU_RD("Full", tmp.YIA,   RN7302_REG_YIA,   RN7302_LEN_YIA);
    EMU_RD("Full", tmp.YIB,   RN7302_REG_YIB,   RN7302_LEN_YIB);
    EMU_RD("Full", tmp.YIC,   RN7302_REG_YIC,   RN7302_LEN_YIC);

    EmuFull = tmp;
    if (ppEmu != NULL)
        *ppEmu = &EmuFull;
    return 0;
}

/**
 * @brief  读取基波计量寄存器；驱动失败时不发布缓存
 */
int get_EmuFund(struct rn7302_emu **ppEmu)
{
    struct rn7302_emu tmp;

    if (ppEmu != NULL)
        *ppEmu = NULL;

    memset(&tmp, 0, sizeof(tmp));
    EMU_RD("Fund", tmp.UA,    RN7302_REG_FUA,   RN7302_LEN_FUA);
    EMU_RD("Fund", tmp.UB,    RN7302_REG_FUB,   RN7302_LEN_FUB);
    EMU_RD("Fund", tmp.UC,    RN7302_REG_FUC,   RN7302_LEN_FUC);
    EMU_RD("Fund", tmp.IA,    RN7302_REG_FIA,   RN7302_LEN_FIA);
    EMU_RD("Fund", tmp.IB,    RN7302_REG_FIB,   RN7302_LEN_FIB);
    EMU_RD("Fund", tmp.IC,    RN7302_REG_FIC,   RN7302_LEN_FIC);
    EMU_RD("Fund", tmp.PA,    RN7302_REG_FPA,   RN7302_LEN_FPA);
    EMU_RD("Fund", tmp.PB,    RN7302_REG_FPB,   RN7302_LEN_FPB);
    EMU_RD("Fund", tmp.PC,    RN7302_REG_FPC,   RN7302_LEN_FPC);
    EMU_RD("Fund", tmp.PT,    RN7302_REG_FPT,   RN7302_LEN_FPT);
    EMU_RD("Fund", tmp.QA,    RN7302_REG_FQA,   RN7302_LEN_FQA);
    EMU_RD("Fund", tmp.QB,    RN7302_REG_FQB,   RN7302_LEN_FQB);
    EMU_RD("Fund", tmp.QC,    RN7302_REG_FQC,   RN7302_LEN_FQC);
    EMU_RD("Fund", tmp.QT,    RN7302_REG_FQT,   RN7302_LEN_FQT);
    EMU_RD("Fund", tmp.SA,    RN7302_REG_FSA,   RN7302_LEN_FSA);
    EMU_RD("Fund", tmp.SB,    RN7302_REG_FSB,   RN7302_LEN_FSB);
    EMU_RD("Fund", tmp.SC,    RN7302_REG_FSC,   RN7302_LEN_FSC);
    EMU_RD("Fund", tmp.STA,   RN7302_REG_FSTA,  RN7302_LEN_FSTA);
    EMU_RD("Fund", tmp.PfA,   RN7302_REG_FPFA,  RN7302_LEN_FPFA);
    EMU_RD("Fund", tmp.PfB,   RN7302_REG_FPFB,  RN7302_LEN_FPFB);
    EMU_RD("Fund", tmp.PfC,   RN7302_REG_FPFC,  RN7302_LEN_FPFC);
    EMU_RD("Fund", tmp.PfTV,  RN7302_REG_FPFTV, RN7302_LEN_FPFTV);
    EMU_RD("Fund", tmp.UFreq, RN7302_REG_UFREQ, RN7302_LEN_UFREQ);
    EMU_RD("Fund", tmp.YUA,   RN7302_REG_YUA,   RN7302_LEN_YUA);
    EMU_RD("Fund", tmp.YUB,   RN7302_REG_YUB,   RN7302_LEN_YUB);
    EMU_RD("Fund", tmp.YUC,   RN7302_REG_YUC,   RN7302_LEN_YUC);
    EMU_RD("Fund", tmp.YIA,   RN7302_REG_YIA,   RN7302_LEN_YIA);
    EMU_RD("Fund", tmp.YIB,   RN7302_REG_YIB,   RN7302_LEN_YIB);
    EMU_RD("Fund", tmp.YIC,   RN7302_REG_YIC,   RN7302_LEN_YIC);

    EmuFund = tmp;
    if (ppEmu != NULL)
        *ppEmu = &EmuFund;
    return 0;
}

/**
 * @brief  读取半波计量寄存器；驱动失败时不更新缓存并返回错误（无软件幅值跳变判据）
 */
int get_EmuHalf(struct rn7302_emu **ppEmu)
{
    int ret;
    int saved_errno;
    uint32_t value[8];

    if (ppEmu != NULL)
        *ppEmu = NULL;

    memset(value, 0, sizeof(value));
    ret = MeterChipReadArea(RN7302_REG_UA_HW, sizeof(value), value);
    saved_errno = errno;

    if (ret < 0) {
        printf("[HW-ERR] spi/crc fail ret=%d errno=%d(%s)\n",
               ret, saved_errno, strerror(saved_errno));
        errno = saved_errno;
        return -1;
    }
    if (ret != 32) {
        printf("[HW-ERR] unexpected ret=%d errno=%d(%s)\n",
               ret, saved_errno, strerror(saved_errno));
        errno = (saved_errno != 0) ? saved_errno : EIO;
        return -1;
    }

    EmuHalf.UA = value[0];
    EmuHalf.UB = value[1];
    EmuHalf.UC = value[2];
    EmuHalf.IA = value[3];
    EmuHalf.IB = value[4];
    EmuHalf.IC = value[5];

    ret = MeterChipReadArea(RN7302_REG_SPL_PA, sizeof(value), value);
    saved_errno = errno;
    if (ret < 0) {
        printf("[HW-ERR] SPL_PA spi/crc fail ret=%d errno=%d(%s)\n",
               ret, saved_errno, strerror(saved_errno));
        errno = saved_errno;
        return -1;
    }
    if (ret != 32) {
        errno = (saved_errno != 0) ? saved_errno : EIO;
        return -1;
    }
    EmuHalf.PA = value[0];
    EmuHalf.PB = value[1];
    EmuHalf.PC = value[2];
    EmuHalf.QA = value[3];
    EmuHalf.QB = value[4];
    EmuHalf.QC = value[5];

    EMU_RD("Half", EmuHalf.YUA,   RN7302_REG_YUA,   RN7302_LEN_YUA);
    EMU_RD("Half", EmuHalf.YUB,   RN7302_REG_YUB,   RN7302_LEN_YUB);
    EMU_RD("Half", EmuHalf.YUC,   RN7302_REG_YUC,   RN7302_LEN_YUC);
    EMU_RD("Half", EmuHalf.YIA,   RN7302_REG_YIA,   RN7302_LEN_YIA);
    EMU_RD("Half", EmuHalf.YIB,   RN7302_REG_YIB,   RN7302_LEN_YIB);
    EMU_RD("Half", EmuHalf.YIC,   RN7302_REG_YIC,   RN7302_LEN_YIC);
    EMU_RD("Half", EmuHalf.UFreq, RN7302_REG_UFREQ, RN7302_LEN_UFREQ);

    if (ppEmu != NULL)
        *ppEmu = &EmuHalf;
    return 0;
}


/*************************************************************************/
/** Function Name:     get_EmuFullEnergy                                     **/    
/** Purpose:           全波电能计量值                                       **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
struct rn7302_emu_energy* get_EmuFullEnergy(void)
{   
    MeterChipReadReg(RN7302_REG_POSEPA, RN7302_LEN_POSEPA,   (int32_t*)&EmuFullEnergy.PosEPa   );   
    MeterChipReadReg(RN7302_REG_POSEPB, RN7302_LEN_POSEPB,   (int32_t*)&EmuFullEnergy.PosEPb   );   
    MeterChipReadReg(RN7302_REG_POSEPC, RN7302_LEN_POSEPC,   (int32_t*)&EmuFullEnergy.PosEPc   );   
    MeterChipReadReg(RN7302_REG_POSEPT, RN7302_LEN_POSEPT,   (int32_t*)&EmuFullEnergy.PosEPt   );   
    MeterChipReadReg(RN7302_REG_NEGEPA, RN7302_LEN_NEGEPA,   (int32_t*)&EmuFullEnergy.NegEPa   );   
    MeterChipReadReg(RN7302_REG_NEGEPB, RN7302_LEN_NEGEPB,   (int32_t*)&EmuFullEnergy.NegEPb   );   
    MeterChipReadReg(RN7302_REG_NEGEPC, RN7302_LEN_NEGEPC,   (int32_t*)&EmuFullEnergy.NegEPc   );   
    MeterChipReadReg(RN7302_REG_NEGEPT, RN7302_LEN_NEGEPT,   (int32_t*)&EmuFullEnergy.NegEPt   );   
    MeterChipReadReg(RN7302_REG_EPT   , RN7302_LEN_EPT   ,   (int32_t*)&EmuFullEnergy.EPt      );      
    MeterChipReadReg(RN7302_REG_POSEQA, RN7302_LEN_POSEQA,   (int32_t*)&EmuFullEnergy.PosEQa   );    
    MeterChipReadReg(RN7302_REG_POSEQB, RN7302_LEN_POSEQB,   (int32_t*)&EmuFullEnergy.PosEQb   );   
    MeterChipReadReg(RN7302_REG_POSEQC, RN7302_LEN_POSEQC,   (int32_t*)&EmuFullEnergy.PosEQc   );   
    MeterChipReadReg(RN7302_REG_POSEQT, RN7302_LEN_POSEQT,   (int32_t*)&EmuFullEnergy.PosEQt   );   
    MeterChipReadReg(RN7302_REG_NEGEQA, RN7302_LEN_NEGEQA,   (int32_t*)&EmuFullEnergy.NegEQa   );   
    MeterChipReadReg(RN7302_REG_NEGEQB, RN7302_LEN_NEGEQB,   (int32_t*)&EmuFullEnergy.NegEQb   );   
    MeterChipReadReg(RN7302_REG_NEGEQC, RN7302_LEN_NEGEQC,   (int32_t*)&EmuFullEnergy.NegEQc   );   
    MeterChipReadReg(RN7302_REG_NEGEQT, RN7302_LEN_NEGEQT,   (int32_t*)&EmuFullEnergy.NegEQt   );   
    MeterChipReadReg(RN7302_REG_EQT   , RN7302_LEN_EQT   ,   (int32_t*)&EmuFullEnergy.EQt      );      
    MeterChipReadReg(RN7302_REG_ESA   , RN7302_LEN_ESA   ,   (int32_t*)&EmuFullEnergy.ESa      );       
    MeterChipReadReg(RN7302_REG_ESB   , RN7302_LEN_ESB   ,   (int32_t*)&EmuFullEnergy.ESb      );       
    MeterChipReadReg(RN7302_REG_ESC   , RN7302_LEN_ESC   ,   (int32_t*)&EmuFullEnergy.ESc      );       
    MeterChipReadReg(RN7302_REG_EST   , RN7302_LEN_EST   ,   (int32_t*)&EmuFullEnergy.ESt      );       
    MeterChipReadReg(RN7302_REG_ESTV  , RN7302_LEN_ESTV  ,   (int32_t*)&EmuFullEnergy.Estv     );     
    
    return &EmuFullEnergy;
}

/*************************************************************************/
/** Function Name:     get_EmuFundEnergy                                     **/    
/** Purpose:           基波电能计量值                                       **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
struct rn7302_emu_energy* get_EmuFundEnergy(void)
{   
    MeterChipReadReg(RN7302_REG_POSFEPA, RN7302_LEN_POSFEPA,   (int32_t*)&EmuFundEnergy.PosEPa   );   
    MeterChipReadReg(RN7302_REG_POSFEPB, RN7302_LEN_POSFEPB,   (int32_t*)&EmuFundEnergy.PosEPb   );   
    MeterChipReadReg(RN7302_REG_POSFEPC, RN7302_LEN_POSFEPC,   (int32_t*)&EmuFundEnergy.PosEPc   );   
    MeterChipReadReg(RN7302_REG_POSFEPT, RN7302_LEN_POSFEPT,   (int32_t*)&EmuFundEnergy.PosEPt   );   
    MeterChipReadReg(RN7302_REG_NEGFEPA, RN7302_LEN_NEGFEPA,   (int32_t*)&EmuFundEnergy.NegEPa   );   
    MeterChipReadReg(RN7302_REG_NEGFEPB, RN7302_LEN_NEGFEPB,   (int32_t*)&EmuFundEnergy.NegEPb   );   
    MeterChipReadReg(RN7302_REG_NEGFEPC, RN7302_LEN_NEGFEPC,   (int32_t*)&EmuFundEnergy.NegEPc   );   
    MeterChipReadReg(RN7302_REG_NEGFEPT, RN7302_LEN_NEGFEPT,   (int32_t*)&EmuFundEnergy.NegEPt   );   
    MeterChipReadReg(RN7302_REG_FEPT   , RN7302_LEN_FEPT   ,   (int32_t*)&EmuFundEnergy.EPt      );      
    MeterChipReadReg(RN7302_REG_POSFEQA, RN7302_LEN_POSFEQA,   (int32_t*)&EmuFundEnergy.PosEQa   );    
    MeterChipReadReg(RN7302_REG_POSFEQB, RN7302_LEN_POSFEQB,   (int32_t*)&EmuFundEnergy.PosEQb   );   
    MeterChipReadReg(RN7302_REG_POSFEQC, RN7302_LEN_POSFEQC,   (int32_t*)&EmuFundEnergy.PosEQc   );   
    MeterChipReadReg(RN7302_REG_POSFEQT, RN7302_LEN_POSFEQT,   (int32_t*)&EmuFundEnergy.PosEQt   );   
    MeterChipReadReg(RN7302_REG_NEGFEQA, RN7302_LEN_NEGFEQA,   (int32_t*)&EmuFundEnergy.NegEQa   );   
    MeterChipReadReg(RN7302_REG_NEGFEQB, RN7302_LEN_NEGFEQB,   (int32_t*)&EmuFundEnergy.NegEQb   );   
    MeterChipReadReg(RN7302_REG_NEGFEQC, RN7302_LEN_NEGFEQC,   (int32_t*)&EmuFundEnergy.NegEQc   );   
    MeterChipReadReg(RN7302_REG_NEGFEQT, RN7302_LEN_NEGFEQT,   (int32_t*)&EmuFundEnergy.NegEQt   );   
    MeterChipReadReg(RN7302_REG_FEQT   , RN7302_LEN_FEQT   ,   (int32_t*)&EmuFundEnergy.EQt      );      
    MeterChipReadReg(RN7302_REG_FESA   , RN7302_LEN_FESA   ,   (int32_t*)&EmuFundEnergy.ESa      );       
    MeterChipReadReg(RN7302_REG_FESB   , RN7302_LEN_FESB   ,   (int32_t*)&EmuFundEnergy.ESb      );       
    MeterChipReadReg(RN7302_REG_FESC   , RN7302_LEN_FESC   ,   (int32_t*)&EmuFundEnergy.ESc      );       
    MeterChipReadReg(RN7302_REG_FESTA  , RN7302_LEN_FESTA  ,   (int32_t*)&EmuFundEnergy.ESt      );       
    MeterChipReadReg(RN7302_REG_FESTV  , RN7302_LEN_FESTV  ,   (int32_t*)&EmuFundEnergy.Estv     );       
    
    return &EmuFundEnergy;
}
