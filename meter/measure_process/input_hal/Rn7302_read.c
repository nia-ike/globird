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

/*************************************************************************/
/** Function Name:     get_Emufull                                     **/    
/** Purpose:           全波计量值                                       **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
//struct rn7302_emu_fullwave* get_EmuFull(void)
struct rn7302_emu* get_EmuFull(void)
{   
    MeterChipReadReg(RN7302_REG_UA, RN7302_LEN_UA,   (int32_t*)&EmuFull.UA   );       //UA
    MeterChipReadReg(RN7302_REG_UB, RN7302_LEN_UB,   (int32_t*)&EmuFull.UB   );       //UB
    MeterChipReadReg(RN7302_REG_UC, RN7302_LEN_UC,   (int32_t*)&EmuFull.UC   );       //UC
    MeterChipReadReg(RN7302_REG_IA, RN7302_LEN_IA,   (int32_t*)&EmuFull.IA   );       //IA
    MeterChipReadReg(RN7302_REG_IB, RN7302_LEN_IB,   (int32_t*)&EmuFull.IB   );       //IB
    MeterChipReadReg(RN7302_REG_IC, RN7302_LEN_IC,   (int32_t*)&EmuFull.IC   );       //IC
    MeterChipReadReg(RN7302_REG_IN, RN7302_LEN_IN,   (int32_t*)&EmuFull.IN   );       //IN
    MeterChipReadReg(RN7302_REG_PA, RN7302_LEN_PA,   (int32_t*)&EmuFull.PA   );       //PA
    MeterChipReadReg(RN7302_REG_PB, RN7302_LEN_PB,   (int32_t*)&EmuFull.PB   );       //PB
    MeterChipReadReg(RN7302_REG_PC, RN7302_LEN_PC,   (int32_t*)&EmuFull.PC   );       //PC
    MeterChipReadReg(RN7302_REG_PT, RN7302_LEN_PT,   (int32_t*)&EmuFull.PT   );       //PT
    MeterChipReadReg(RN7302_REG_QA, RN7302_LEN_QA,   (int32_t*)&EmuFull.QA   );       //QA
    MeterChipReadReg(RN7302_REG_QB, RN7302_LEN_QB,   (int32_t*)&EmuFull.QB   );       //QB
    MeterChipReadReg(RN7302_REG_QC, RN7302_LEN_QC,   (int32_t*)&EmuFull.QC   );       //QC
    MeterChipReadReg(RN7302_REG_QT, RN7302_LEN_QT,   (int32_t*)&EmuFull.QT   );       //QT
    MeterChipReadReg(RN7302_REG_SA, RN7302_LEN_SA,   (int32_t*)&EmuFull.SA   );       //SA
    MeterChipReadReg(RN7302_REG_SB, RN7302_LEN_SB,   (int32_t*)&EmuFull.SB   );       //SB
    MeterChipReadReg(RN7302_REG_SC, RN7302_LEN_SC,   (int32_t*)&EmuFull.SC   );       //SC
    MeterChipReadReg(RN7302_REG_STA, RN7302_LEN_STA,   (int32_t*)&EmuFull.STA  );       //ST
    MeterChipReadReg(RN7302_REG_PFA, RN7302_LEN_PFA,  (int32_t*)&EmuFull.PfA  );       //PFA
    MeterChipReadReg(RN7302_REG_PFB, RN7302_LEN_PFB,  (int32_t*)&EmuFull.PfB  );       //PFB
    MeterChipReadReg(RN7302_REG_PFC, RN7302_LEN_PFC,  (int32_t*)&EmuFull.PfC  );       //PFC
    MeterChipReadReg(RN7302_REG_PFTV, RN7302_LEN_PFTV, (int32_t*)&EmuFull.PfTV );       //PFT
    MeterChipReadReg(RN7302_REG_UFREQ, RN7302_LEN_UFREQ,(int32_t*)&EmuFull.UFreq);       //HZ
    MeterChipReadReg(RN7302_REG_YUA, RN7302_LEN_YUA,  (int32_t*)&EmuFull.YUA  );       //YUA
    MeterChipReadReg(RN7302_REG_YUB, RN7302_LEN_YUB,  (int32_t*)&EmuFull.YUB  );       //YUB
    MeterChipReadReg(RN7302_REG_YUC, RN7302_LEN_YUC,  (int32_t*)&EmuFull.YUC  );       //YUC
    MeterChipReadReg(RN7302_REG_YIA, RN7302_LEN_YIA,  (int32_t*)&EmuFull.YIA  );       //YIA
    MeterChipReadReg(RN7302_REG_YIB, RN7302_LEN_YIB,  (int32_t*)&EmuFull.YIB  );       //YIB
    MeterChipReadReg(RN7302_REG_YIC, RN7302_LEN_YIC,  (int32_t*)&EmuFull.YIC  );       //YIC
    
    return &EmuFull;
}


/*************************************************************************/
/** Function Name:     get_EmuFundamental                              **/    
/** Purpose:           基波谐波计量值                                   **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
//struct rn7302_emu_fundwave * get_EmuFund(void)
struct rn7302_emu * get_EmuFund(void)
{   
    MeterChipReadReg(RN7302_REG_FUA, RN7302_LEN_FUA,   (int32_t*)&EmuFund.UA   );       
    MeterChipReadReg(RN7302_REG_FUB, RN7302_LEN_FUB,   (int32_t*)&EmuFund.UB   );       
    MeterChipReadReg(RN7302_REG_FUC, RN7302_LEN_FUC,   (int32_t*)&EmuFund.UC   );       
    MeterChipReadReg(RN7302_REG_FIA, RN7302_LEN_FIA,   (int32_t*)&EmuFund.IA   );       
    MeterChipReadReg(RN7302_REG_FIB, RN7302_LEN_FIB,   (int32_t*)&EmuFund.IB   );       
    MeterChipReadReg(RN7302_REG_FIC, RN7302_LEN_FIC,   (int32_t*)&EmuFund.IC   );       
    MeterChipReadReg(RN7302_REG_FPA, RN7302_LEN_FPA,   (int32_t*)&EmuFund.PA   );       
    MeterChipReadReg(RN7302_REG_FPB, RN7302_LEN_FPB,   (int32_t*)&EmuFund.PB   );       
    MeterChipReadReg(RN7302_REG_FPC, RN7302_LEN_FPC,   (int32_t*)&EmuFund.PC   );       
    MeterChipReadReg(RN7302_REG_FPT, RN7302_LEN_FPT,   (int32_t*)&EmuFund.PT   );       
    MeterChipReadReg(RN7302_REG_FQA, RN7302_LEN_FQA,   (int32_t*)&EmuFund.QA   );       
    MeterChipReadReg(RN7302_REG_FQB, RN7302_LEN_FQB,   (int32_t*)&EmuFund.QB   );       
    MeterChipReadReg(RN7302_REG_FQC, RN7302_LEN_FQC,   (int32_t*)&EmuFund.QC   );       
    MeterChipReadReg(RN7302_REG_FQT, RN7302_LEN_FQT,   (int32_t*)&EmuFund.QT   );       
    MeterChipReadReg(RN7302_REG_FSA, RN7302_LEN_FSA,   (int32_t*)&EmuFund.SA   );       
    MeterChipReadReg(RN7302_REG_FSB, RN7302_LEN_FSB,   (int32_t*)&EmuFund.SB   );       
    MeterChipReadReg(RN7302_REG_FSC, RN7302_LEN_FSC,   (int32_t*)&EmuFund.SC   );       
    MeterChipReadReg(RN7302_REG_FSTA, RN7302_LEN_FSTA,  (int32_t*)&EmuFund.STA  );       
    MeterChipReadReg(RN7302_REG_FPFA, RN7302_LEN_FPFA,  (int32_t*)&EmuFund.PfA  );       
    MeterChipReadReg(RN7302_REG_FPFB, RN7302_LEN_FPFB,  (int32_t*)&EmuFund.PfB  );       
    MeterChipReadReg(RN7302_REG_FPFC, RN7302_LEN_FPFC,  (int32_t*)&EmuFund.PfC  );       
    MeterChipReadReg(RN7302_REG_FPFTV, RN7302_LEN_FPFTV, (int32_t*)&EmuFund.PfTV );       
    MeterChipReadReg(RN7302_REG_UFREQ, RN7302_LEN_UFREQ, (int32_t*)&EmuFund.UFreq);       
    MeterChipReadReg(RN7302_REG_YUA, RN7302_LEN_YUA,   (int32_t*)&EmuFund.YUA  );       //YUA
    MeterChipReadReg(RN7302_REG_YUB, RN7302_LEN_YUB,   (int32_t*)&EmuFund.YUB  );       //YUB
    MeterChipReadReg(RN7302_REG_YUC, RN7302_LEN_YUC,   (int32_t*)&EmuFund.YUC  );       //YUC
    MeterChipReadReg(RN7302_REG_YIA, RN7302_LEN_YIA,   (int32_t*)&EmuFund.YIA  );       //YIA
    MeterChipReadReg(RN7302_REG_YIB, RN7302_LEN_YIB,   (int32_t*)&EmuFund.YIB  );       //YIB
    MeterChipReadReg(RN7302_REG_YIC, RN7302_LEN_YIC,   (int32_t*)&EmuFund.YIC  );       //YIC
    return &EmuFund;
}



/*************************************************************************/
/** Function Name:     get_EmuFundamental                              **/    
/** Purpose:           基波谐波计量值                                   **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
//struct rn7302_emu_halfwave * get_EmuHalf(void)
struct rn7302_emu * get_EmuHalf(void)
{   
    int ret;
    uint32_t value[8];

    ret = MeterChipReadArea(RN7302_REG_UA_HW, sizeof(value), value);
    EmuHalf.UA = value[0];
    EmuHalf.UB = value[1];
    EmuHalf.UC = value[2];
    EmuHalf.IA = value[3];
    EmuHalf.IB = value[4];
    EmuHalf.IC = value[5];

    if (ICHSEL_ON == get_ICHSEL_flag())
    {
        MeterChipReadReg(RN7302_REG_YUA, RN7302_LEN_YUA,    (int32_t*)&EmuHalf.YUA  );       //YUA
        MeterChipReadReg(RN7302_REG_YUB, RN7302_LEN_YUB,    (int32_t*)&EmuHalf.YUB  );       //YUB
        MeterChipReadReg(RN7302_REG_YUC, RN7302_LEN_YUC,    (int32_t*)&EmuHalf.YUC  );       //YUC
        MeterChipReadReg(RN7302_REG_YIA, RN7302_LEN_YIA,    (int32_t*)&EmuHalf.YIA  );       //YIA
        MeterChipReadReg(RN7302_REG_YIB, RN7302_LEN_YIB,    (int32_t*)&EmuHalf.YIB  );       //YIB
        MeterChipReadReg(RN7302_REG_YIC, RN7302_LEN_YIC,    (int32_t*)&EmuHalf.YIC  );       //YIC
    }
    else
    {
        MeterChipReadArea(RN7302_REG_SPL_PA, sizeof(value), value);
        EmuHalf.PA = value[0];
        EmuHalf.PB = value[1];
        EmuHalf.PC = value[2];
        EmuHalf.QA = value[3];
        EmuHalf.QB = value[4];
        EmuHalf.QC = value[5];
    }

    MeterChipReadReg(RN7302_REG_UFREQ, RN7302_LEN_UFREQ,   (int32_t*)&EmuHalf.UFreq );  
         
    return &EmuHalf;
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
