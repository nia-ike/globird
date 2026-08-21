/*************************************************
  Copyright (C), 2003-2017, pilot Co., Ltd.
  File name:      Rn7302.h
  Author:       longzh    Version: 1.0       Date: 2017-6-9
  Description:  Rn7302 ����
  Others:         
  History:              
*************************************************/
#ifndef __RN7302_REGISTER_H__
#define __RN7302_REGISTER_H__
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

struct rn7302_emu
{
    /*" "  全波计量参数寄存器""*/
    int32_t    UA;                        /*"  offset  0x07        "*/
    int32_t    UB;                        /*"  offset  0x08        "*/
    int32_t    UC;                        /*"  offset  0x09       "*/
    int32_t    USUM;                      /*"  offset  0x0A        "*/
    int32_t    IA;                        /*"  offset  0x0B        "*/
    int32_t    IB;                        /*"  offset  0x0C        "*/
    int32_t    IC;                        /*"  offset  0x0D       "*/
    int32_t    IN;                        /*"  offset  0x0E        "*/
    int32_t    ISUM;                      /*"  offset  0x10        "*/
    int32_t    PA;                        /*"  offset  0x14        "*/
    int32_t    PB;                        /*"  offset  0x15        "*/
    int32_t    PC;                        /*"  offset  0x16        "*/
    int32_t    PT;                        /*"  offset  0x17        "*/
    int32_t    QA;                        /*"  offset  0x18        "*/
    int32_t    QB;                        /*"  offset  0x19        "*/
    int32_t    QC;                        /*"  offset  0x1A        "*/
    int32_t    QT;                        /*"  offset  0x1B        "*/
    int32_t    SA;                        /*"  offset  0x1C        "*/
    int32_t    SB;                        /*"  offset  0x1D        "*/
    int32_t    SC;                        /*"  offset  0x1E        "*/
    int32_t    STA;                       /*"  offset  0x1F        "*/
    int32_t    PfA;                       /*"  offset  0x20        "*/
    int32_t    PfB;                       /*"  offset  0x21        "*/
    int32_t    PfC;                       /*"  offset  0x22        "*/
    int32_t    PfTA;                      /*"  offset  0x23        "*/
    int32_t    EPA;                       /*"  offset  0x30        "*/
    int32_t    EPB;                       /*"  offset  0x31        "*/
    int32_t    EPC;                       /*"  offset  0x32        "*/
    int32_t    EPT;                       /*"  offset  0x33        "*/
    int32_t    EQA;                       /*"  offset  0x3C        "*/
    int32_t    EQB;                       /*"  offset  0x3D        "*/
    int32_t    EQC;                       /*"  offset  0x3E        "*/
    int32_t    EQT;                       /*"  offset  0x3F        "*/
    int32_t    ESA;                       /*"  offset  0x48        "*/
    int32_t    ESB;                       /*"  offset  0x49        "*/
    int32_t    ESC;                       /*"  offset  0x4A        "*/
    int32_t    ESTA;                      /*"  offset  0x4B        "*/
    int32_t    STV;                       /*"  offset  0x4C        "*/
    int32_t    PfTV;                      /*"  offset  0x4D        "*/
    int32_t    ESTV;                      /*"  offset  0x4F        "*/
    int32_t    UFreq;                     /*"  offset  0x57        "*/
    int32_t    YUA;                       /*"  offset  0x50        "*/
    int32_t    YUB;                       /*"  offset  0x51        "*/
    int32_t    YUC;                       /*"  offset  0x52        "*/
    int32_t    YIA;                       /*"  offset  0x53        "*/
    int32_t    YIB;                       /*"  offset  0x54        "*/
    int32_t    YIC;                       /*"  offset  0x55        "*/
    int32_t    YIN;                       /*"  offset  0x56        "*/

};


struct rn7302_emu_fullwave
{
    /*" "  全波计量参数寄存器""*/
    int32_t    UA;                        /*"  offset  0x07        "*/
    int32_t    UB;                        /*"  offset  0x08        "*/
    int32_t    UC;                        /*"  offset  0x09       "*/
    int32_t    USUM;                      /*"  offset  0x0A        "*/
    int32_t    IA;                        /*"  offset  0x0B        "*/
    int32_t    IB;                        /*"  offset  0x0C        "*/
    int32_t    IC;                        /*"  offset  0x0D       "*/
    int32_t    IN;                        /*"  offset  0x0E        "*/
    int32_t    Reserved0F;                /*"  offset  0x0F        "*/    
    int32_t    ISUM;                      /*"  offset  0x10        "*/
    int32_t    IA_NVM1;                   /*"  offset  0x11        "*/
    int32_t    IB_NVM1;                   /*"  offset  0x12        "*/
    int32_t    IC_NVM1;                   /*"  offset  0x13        "*/
    int32_t    PA;                        /*"  offset  0x14        "*/
    int32_t    PB;                        /*"  offset  0x15        "*/
    int32_t    PC;                        /*"  offset  0x16        "*/
    int32_t    PT;                        /*"  offset  0x17        "*/
    int32_t    QA;                        /*"  offset  0x18        "*/
    int32_t    QB;                        /*"  offset  0x19        "*/
    int32_t    QC;                        /*"  offset  0x1A        "*/
    int32_t    QT;                        /*"  offset  0x1B        "*/
    int32_t    SA;                        /*"  offset  0x1C        "*/
    int32_t    SB;                        /*"  offset  0x1D        "*/
    int32_t    SC;                        /*"  offset  0x1E        "*/
    int32_t    STA;                       /*"  offset  0x1F        "*/
    int32_t    PfA;                       /*"  offset  0x20        "*/
    int32_t    PfB;                       /*"  offset  0x21        "*/
    int32_t    PfC;                       /*"  offset  0x22        "*/
    int32_t    PfTA;                      /*"  offset  0x23        "*/
    int32_t    PAFCnt;                    /*"  offset  0x24        "*/
    int32_t    PBFCnt;                    /*"  offset  0x25        "*/
    int32_t    PCFCnt;                    /*"  offset  0x26        "*/
    int32_t    PTFCnt;                    /*"  offset  0x27        "*/
    int32_t    QAFCnt;                    /*"  offset  0x28        "*/
    int32_t    QBFCnt;                    /*"  offset  0x29        "*/
    int32_t    QCFCnt;                    /*"  offset  0x2A        "*/
    int32_t    QTFCnt;                    /*"  offset  0x2B        "*/
    int32_t    SAFCnt;                    /*"  offset  0x2C        "*/
    int32_t    SBFCnt;                    /*"  offset  0x2D        "*/
    int32_t    SCFCnt;                    /*"  offset  0x2E        "*/
    int32_t    STFACnt;                   /*"  offset  0x2F        "*/
    int32_t    EPA;                       /*"  offset  0x30        "*/
    int32_t    EPB;                       /*"  offset  0x31        "*/
    int32_t    EPC;                       /*"  offset  0x32        "*/
    int32_t    EPT;                       /*"  offset  0x33        "*/
    int32_t    PosEPA;                    /*"  offset  0x34        "*/
    int32_t    PosEPB;                    /*"  offset  0x35        "*/
    int32_t    PosEPC;                    /*"  offset  0x36        "*/
    int32_t    PosEPT;                    /*"  offset  0x37        "*/
    int32_t    NegEPA;                    /*"  offset  0x38        "*/
    int32_t    NegEPB;                    /*"  offset  0x39        "*/
    int32_t    NegEPC;                    /*"  offset  0x3A        "*/
    int32_t    NegEPT;                    /*"  offset  0x3B        "*/
    int32_t    EQA;                       /*"  offset  0x3C        "*/
    int32_t    EQB;                       /*"  offset  0x3D        "*/
    int32_t    EQC;                       /*"  offset  0x3E        "*/
    int32_t    EQT;                       /*"  offset  0x3F        "*/
    int32_t    PosEQA;                    /*"  offset  0x40        "*/
    int32_t    PosEQB;                    /*"  offset  0x41        "*/
    int32_t    PosEQC;                    /*"  offset  0x42        "*/
    int32_t    PosEQT;                    /*"  offset  0x43        "*/
    int32_t    NegEQA;                    /*"  offset  0x44        "*/
    int32_t    NegEQB;                    /*"  offset  0x45        "*/
    int32_t    NegEQC;                    /*"  offset  0x46        "*/
    int32_t    NegEQT;                    /*"  offset  0x47        "*/
    int32_t    ESA;                       /*"  offset  0x48        "*/
    int32_t    ESB;                       /*"  offset  0x49        "*/
    int32_t    ESC;                       /*"  offset  0x4A        "*/
    int32_t    ESTA;                      /*"  offset  0x4B        "*/
    int32_t    STV;                       /*"  offset  0x4C        "*/
    int32_t    PfTV;                      /*"  offset  0x4D        "*/
    int32_t    STFVCnt;                   /*"  offset  0x4E        "*/
    int32_t    ESTV;                      /*"  offset  0x4F        "*/
    int32_t    UFreq;                  /*"  offset  0x57        "*/

};

struct rn7302_emu_fundwave
{
    /*" "  基波谐波计量参数寄存器""*/
    int32_t    YUA;                    /*"  offset  0x50        "*/
    int32_t    YUB;                    /*"  offset  0x51        "*/
    int32_t    YUC;                    /*"  offset  0x52        "*/
    int32_t    YIA;                    /*"  offset  0x53        "*/
    int32_t    YIB;                    /*"  offset  0x54        "*/
    int32_t    YIC;                    /*"  offset  0x55        "*/
    int32_t    YIN;                    /*"  offset  0x56        "*/
    int32_t    UFreq;                  /*"  offset  0x57        "*/
    int32_t    FUA;                    /*"  offset  0x58        "*/
    int32_t    FUB;                    /*"  offset  0x59        "*/
    int32_t    FUC;                    /*"  offset  0x5A        "*/
    int32_t    FIA;                    /*"  offset  0x5B        "*/
    int32_t    FIB;                    /*"  offset  0x5C        "*/
    int32_t    FIC;                    /*"  offset  0x5D        "*/
    int32_t    FPA;                    /*"  offset  0x5E        "*/
    int32_t    FPB;                    /*"  offset  0x5F        "*/
    int32_t    FPC;                    /*"  offset  0x60        "*/
    int32_t    FPT;                    /*"  offset  0x61        "*/
    int32_t    FQA;                    /*"  offset  0x62        "*/
    int32_t    FQB;                    /*"  offset  0x63        "*/
    int32_t    FQC;                    /*"  offset  0x64        "*/
    int32_t    FQT;                    /*"  offset  0x65        "*/
    int32_t    FSA;                    /*"  offset  0x66        "*/
    int32_t    FSB;                    /*"  offset  0x67        "*/
    int32_t    FSC;                    /*"  offset  0x68        "*/
    int32_t    FSTA;                   /*"  offset  0x69        "*/
    int32_t    FPfA;                   /*"  offset  0x6A        "*/
    int32_t    FPfB;                   /*"  offset  0x6B        "*/
    int32_t    FPfC;                   /*"  offset  0x6C        "*/
    int32_t    FPfTA;                  /*"  offset  0x6D        "*/
    int32_t    FPAFCnt;                /*"  offset  0x6E        "*/
    int32_t    FPBFCnt;                /*"  offset  0x6F        "*/
    int32_t    FPCFCnt;                /*"  offset  0x70        "*/
    int32_t    FPTFCnt;                /*"  offset  0x71        "*/
    int32_t    FQAFCnt;                /*"  offset  0x72        "*/
    int32_t    FQBFCnt;                /*"  offset  0x73        "*/
    int32_t    FQCFCnt;                /*"  offset  0x74        "*/
    int32_t    FQTFCnt;                /*"  offset  0x75        "*/
    int32_t    FSAFCnt;                /*"  offset  0x76        "*/
    int32_t    FSBFCnt;                /*"  offset  0x77        "*/
    int32_t    FSCFCnt;                /*"  offset  0x78        "*/
    int32_t    FSTAFCnt;               /*"  offset  0x79        "*/
    int32_t    FEPA;                   /*"  offset  0x7A        "*/
    int32_t    FEPB;                   /*"  offset  0x7B        "*/
    int32_t    FEPC;                   /*"  offset  0x7C        "*/
    int32_t    FEPT;                   /*"  offset  0x7D        "*/
    int32_t    PosFEPA;                /*"  offset  0x7E        "*/
    int32_t    PosFEPB;                /*"  offset  0x7F        "*/
    int32_t    PosFEPC;                /*"  offset  0x80        "*/
    int32_t    PosFEPT;                /*"  offset  0x81        "*/
    int32_t    NegFEPA;                /*"  offset  0x82        "*/
    int32_t    NegFEPB;                /*"  offset  0x83        "*/
    int32_t    NegFEPC;                /*"  offset  0x84        "*/
    int32_t    NegFEPT;                /*"  offset  0x85        "*/
    int32_t    FEQA;                   /*"  offset  0x86        "*/
    int32_t    FEQB;                   /*"  offset  0x87        "*/
    int32_t    FEQC;                   /*"  offset  0x88        "*/
    int32_t    FEQT;                   /*"  offset  0x89        "*/
    int32_t    PosFEQA;                /*"  offset  0x8A        "*/
    int32_t    PosFEQB;                /*"  offset  0x8B        "*/
    int32_t    PosFEQC;                /*"  offset  0x8C        "*/
    int32_t    PosFEQT;                /*"  offset  0x8D        "*/
    int32_t    NegFEQA;                /*"  offset  0x8E        "*/
    int32_t    NegFEQB;                /*"  offset  0x8F        "*/
    int32_t    NegFEQC;                /*"  offset  0x90        "*/
    int32_t    NegFEQT;                /*"  offset  0x91        "*/
    int32_t    FESA;                   /*"  offset  0x92        "*/
    int32_t    FESB;                   /*"  offset  0x93        "*/
    int32_t    FESC;                   /*"  offset  0x94        "*/
    int32_t    FESTA;                  /*"  offset  0x95        "*/
    int32_t    HUA;                    /*"  offset  0x96        "*/
    int32_t    HUB;                    /*"  offset  0x97        "*/
    int32_t    HUC;                    /*"  offset  0x98        "*/
    int32_t    HIA;                    /*"  offset  0x99        "*/
    int32_t    HIB;                    /*"  offset  0x9A        "*/
    int32_t    HIC;                    /*"  offset  0x9B        "*/
    int32_t    FSTV;                   /*"  offset  0x9C        "*/
    int32_t    FPfTV;                  /*"  offset  0x9D        "*/
    int32_t    FSTVFCnt;               /*"  offset  0x9E        "*/
    int32_t    FESTV;                  /*"  offset  0x9F        "*/
};


struct rn7302_emu_halfwave
{
    /*" "  半波计量参数寄存器""*/
    int32_t    UA_HW;                    /*"  offset  0xA0        "*/
    int32_t    UB_HW;                    /*"  offset  0xA1        "*/
    int32_t    UC_HW;                    /*"  offset  0xA2        "*/
    int32_t    IA_HW;                    /*"  offset  0xA3        "*/
    int32_t    IB_HW;                    /*"  offset  0xA4        "*/
    int32_t    IC_HW;                    /*"  offset  0xA5        "*/
    int32_t    UAPEAK_HW;                /*"  offset  0xA6        "*/
    int32_t    UBPEAK_HW;                /*"  offset  0xA7        "*/
    int32_t    UCPEAK_HW;                /*"  offset  0xA8        "*/
    int32_t    IN_HW;                    /*"  offset  0xA9        "*/
    int32_t    FUA_WAVE;                 /*"  offset  0xAA        "*/
    int32_t    FUB_WAVE;                 /*"  offset  0xAB        "*/
    int32_t    FUC_WAVE;                 /*"  offset  0xAC        "*/
    int32_t    FIA_WAVE;                 /*"  offset  0xAD        "*/
    int32_t    FIB_WAVE;                 /*"  offset  0xAE        "*/
    int32_t    FIC_WAVE;                 /*"  offset  0xAF        "*/
    int32_t    FIN_WAVE;                 /*"  offset  0xB0        "*/
    int32_t    FPT;                    /*"  offset  0x61        "*/
    int32_t    FQA;                    /*"  offset  0x62        "*/
    int32_t    FQB;                    /*"  offset  0x63        "*/
    int32_t    FQC;                    /*"  offset  0x64        "*/
    int32_t    FQT;                    /*"  offset  0x65        "*/
    int32_t    FSA;                    /*"  offset  0x66        "*/
    int32_t    FSB;                    /*"  offset  0x67        "*/
    int32_t    FSC;                    /*"  offset  0x68        "*/
    int32_t    FSTA;                   /*"  offset  0x69        "*/
    int32_t    FPfA;                   /*"  offset  0x6A        "*/
    int32_t    FPfB;                   /*"  offset  0x6B        "*/
    int32_t    FPfC;                   /*"  offset  0x6C        "*/
    int32_t    FPfTA;                  /*"  offset  0x6D        "*/
    int32_t    FPAFCnt;                /*"  offset  0x6E        "*/
    int32_t    FPBFCnt;                /*"  offset  0x6F        "*/
    int32_t    FPCFCnt;                /*"  offset  0x70        "*/
    int32_t    FPTFCnt;                /*"  offset  0x71        "*/
    int32_t    FQAFCnt;                /*"  offset  0x72        "*/
    int32_t    FQBFCnt;                /*"  offset  0x73        "*/
    int32_t    FQCFCnt;                /*"  offset  0x74        "*/
    int32_t    FQTFCnt;                /*"  offset  0x75        "*/
    int32_t    FSAFCnt;                /*"  offset  0x76        "*/
    int32_t    FSBFCnt;                /*"  offset  0x77        "*/
    int32_t    FSCFCnt;                /*"  offset  0x78        "*/
    int32_t    FSTAFCnt;               /*"  offset  0x79        "*/
    int32_t    FEPA;                   /*"  offset  0x7A        "*/
    int32_t    FEPB;                   /*"  offset  0x7B        "*/
    int32_t    FEPC;                   /*"  offset  0x7C        "*/
    int32_t    FEPT;                   /*"  offset  0x7D        "*/
    int32_t    PosFEPA;                /*"  offset  0x7E        "*/
    int32_t    PosFEPB;                /*"  offset  0x7F        "*/
    int32_t    PosFEPC;                /*"  offset  0x80        "*/
    int32_t    PosFEPT;                /*"  offset  0x81        "*/
    int32_t    NegFEPA;                /*"  offset  0x82        "*/
    int32_t    NegFEPB;                /*"  offset  0x83        "*/
    int32_t    NegFEPC;                /*"  offset  0x84        "*/
    int32_t    NegFEPT;                /*"  offset  0x85        "*/
    int32_t    FEQA;                   /*"  offset  0x86        "*/
    int32_t    FEQB;                   /*"  offset  0x87        "*/
    int32_t    FEQC;                   /*"  offset  0x88        "*/
    int32_t    FEQT;                   /*"  offset  0x89        "*/
    int32_t    PosFEQA;                /*"  offset  0x8A        "*/
    int32_t    PosFEQB;                /*"  offset  0x8B        "*/
    int32_t    PosFEQC;                /*"  offset  0x8C        "*/
    int32_t    PosFEQT;                /*"  offset  0x8D        "*/
    int32_t    NegFEQA;                /*"  offset  0x8E        "*/
    int32_t    NegFEQB;                /*"  offset  0x8F        "*/
    int32_t    NegFEQC;                /*"  offset  0x90        "*/
    int32_t    NegFEQT;                /*"  offset  0x91        "*/
    int32_t    FESA;                   /*"  offset  0x92        "*/
    int32_t    FESB;                   /*"  offset  0x93        "*/
    int32_t    FESC;                   /*"  offset  0x94        "*/
    int32_t    FESTA;                  /*"  offset  0x95        "*/
    int32_t    HUA;                    /*"  offset  0x96        "*/
    int32_t    HUB;                    /*"  offset  0x97        "*/
    int32_t    HUC;                    /*"  offset  0x98        "*/
    int32_t    HIA;                    /*"  offset  0x99        "*/
    int32_t    HIB;                    /*"  offset  0x9A        "*/
    int32_t    HIC;                    /*"  offset  0x9B        "*/
    int32_t    FSTV;                   /*"  offset  0x9C        "*/
    int32_t    FPfTV;                  /*"  offset  0x9D        "*/
    int32_t    FSTVFCnt;               /*"  offset  0x9E        "*/
    int32_t    FESTV;                  /*"  offset  0x9F        "*/
};



struct rn7302_emu_energy
{
    /*" "  全波计量参数寄存器""*/
    int32_t    PosEPa;                   
    int32_t    PosEPb;                   
    int32_t    PosEPc;                   
    int32_t    PosEPt;                   
    int32_t    NegEPa;                   
    int32_t    NegEPb;                   
    int32_t    NegEPc;                   
    int32_t    NegEPt;                   
    int32_t    EPt;                      
    int32_t    PosEQa;                   
    int32_t    PosEQb;                   
    int32_t    PosEQc;                   
    int32_t    PosEQt;                   
    int32_t    NegEQa;                   
    int32_t    NegEQb;                   
    int32_t    NegEQc;                   
    int32_t    NegEQt;                   
    int32_t    EQt;                      
    int32_t    ESa;                      
    int32_t    ESb;                      
    int32_t    ESc;                      
    int32_t    ESt;                      
    int32_t    Estv;                     
};



#ifdef  __cplusplus
}
#endif

#endif

