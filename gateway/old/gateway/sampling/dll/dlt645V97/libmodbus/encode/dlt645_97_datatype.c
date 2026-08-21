#include <stdio.h>
#include <math.h>
#include "dlt645_97_datatype.h"
#include "dlt645_97_data.h"

/************************************************************************
函数功能：  DLT645BCDDatatoValue   根据DLT645计算参数的值
            寄存器数据转换为数据
输入参数：pBCDBuf   数据帧(BIN格式)首地址
          length    该参数的数据帧长度
          bfactor   参数的比率值(根据数据类型有所不同)
输出参数：
返回值  ：参数的实际值。 用于输出 > 9 的带0xa  0xb ...0xf
说明    ：
************************************************************************/
double DLT645_BIN_DatatoValue(uint8_t *pBCDBuf, uint8_t length)
{
    uint8_t i;
    int64_t tmp = 0;
    double value = 0;

    for(i=0; i<length; i++)
    {
        tmp = tmp  | (((int64_t)pBCDBuf[i]) << (8*i)); 
    }

    value = tmp;
    
    return value;  
}


/************************************************************************
函数功能：  DLT645BCDDatatoValue   根据DLT645计算参数的值
            寄存器数据转换为数据
输入参数：pBCDBuf   数据帧(BCD格式)首地址
          length    该参数的数据帧长度
          bfactor   参数的比率值(根据数据类型有所不同)
输出参数：
返回值  ：参数的实际值
说明    ：
************************************************************************/
double DLT645_BCD_DatatoValue(uint8_t *pBCDBuf, uint8_t length,double bfactor)
{
    uint8_t i, tmp,HHalfByte,LHalfByte;
    double Value = 0;

    for(i=0; i<length; i++)
    {
        LHalfByte = pBCDBuf[i]&0x0F ;
        HHalfByte = (pBCDBuf[i]>>4)&0x0F;
        if((HHalfByte > 0x0A) || (LHalfByte > 0x0A))
        {
            return -1;
        }
        tmp = HHalfByte*10 + LHalfByte;
        Value += tmp * pow(100,i);
    }
    Value *= bfactor;
    return Value;  
}

/************************************************************************
函数功能：DLT645BCDSignedDatatoValue   根据DLT645计算参数的值
输入参数：pBCDBuf   数据帧(BCD格式)首地址
          length    该参数的数据帧长度
          bfactor   参数的比率值(根据数据类型有所不同)
输出参数：
返回值  ：参数的实际值
说明    ：
************************************************************************/
double  DLT645_BCD_SignedDatatoValue(const uint8_t *pBCDBuf, uint8_t length,double bfactor)
{
    uint8_t i, tmp,HHalfByte,LHalfByte;
    double Value = 0;

    //最高位为 1表示负,  0 表示正
    double Signed_Factor = 1.00;

    for(i=0; i<length; i++)
    {
        LHalfByte = pBCDBuf[i]&0x0F ;
        HHalfByte = (pBCDBuf[i]>>4)&0x0F;

        if(i == length-1)   //最高位
        {
            if(HHalfByte >= 0x08)
            {
                Signed_Factor = -1.00;
                HHalfByte -= 0x08;
            }
            else
            {
                Signed_Factor = 1.00;
            }
        }

        tmp = HHalfByte*10 + LHalfByte;
        Value += tmp * pow(100,i);
    }
    Value *= bfactor;
    Value *= Signed_Factor;

    return Value;
}

/************************************************************************
函数功能：  dlt645V07_DataTypeToRegNum()
            根据DLT645的格式计算参数的数据长度
输入参数：  DataType  数据类型
                            详细请查看mydatatype.h 
            pRegAddr  当前需转换的寄存器的地址
输出参数：  
返回值  ：  -1          失败
            非-1        当前使用的寄存器数量（由于数据类型多样，占用1、16、32、64）
说明    ：
************************************************************************/
int32_t dlt645V07_DataTypeToByteNum(uint16_t DataType)
{
    switch(DataType)
    {
        case DATATYPE_DLT645_XXXXXX_XX :
            return 4;
        case DATATYPE_DLT645_XXX_X :
            return 2;
        case DATATYPE_DLT645_XXX_XXX :
            return 3;
        case DATATYPE_DLT645_XX_XXXX :
            return 3;
        case DATATYPE_DLT645_X_XXX :
            return 2;
        case DATATYPE_DLT645_XX_XX :
            return 2;
        case DATATYPE_DLT645_XXXXXXXX :
            return 4;
        case DATATYPE_DLT645_XX :
            return 1;
        case DATATYPE_DLT645_XXX :
            return 2;
        case DATATYPE_DLT645_XXXX :
            return 2;
        
        default:
            return -1;
    }    
}

/************************************************************************
函数功能：DataTypeToBfactor   根据DLT645的格式计算参数的比率值
输入参数：DataType   数据类型
输出参数：
返回值  ：参数的比率值
说明    ：
************************************************************************/
double dlt645V07_DataTypeToBfactor(uint16_t DataType)
{
    switch(DataType)
    {
        case DATATYPE_DLT645_XXXXXX_XX :
            return 1e-2;
        case DATATYPE_DLT645_XXX_X :
            return 1e-1;
        case DATATYPE_DLT645_XXX_XXX :
            return 1e-3;
        case DATATYPE_DLT645_XX_XXXX :
            return 1e-4;
        case DATATYPE_DLT645_X_XXX :
            return 1e-3;
        case DATATYPE_DLT645_XX_XX :
            return 1e-2;
        case DATATYPE_DLT645_XXXXXXXX :
            return 1;
        case DATATYPE_DLT645_XX :
            return 1;
        case DATATYPE_DLT645_XXX :
            return 1;
        case DATATYPE_DLT645_XXXX :
            return 1;
       
        default:
            return 0;
    }
}


/************************************************************************
函数功能：  DLT645_BCDDatatoValue   根据DLT645计算参数的值
            寄存器数据转换为数据
输入参数：pBCDBuf   数据帧(BCD格式)首地址
          length    该参数的数据帧长度
          bfactor   参数的比率值(根据数据类型有所不同)
输出参数：
返回值  ：参数的实际值
说明    ：
************************************************************************/
int32_t DLT645V07_DatatoValue(uint8_t *pBCDBuf, uint16_t DataType, double *pFVal)
{
    switch(DataType)
    {
        case DATATYPE_DLT645_XXXXXX_XX :
        case DATATYPE_DLT645_XXX_X :
        case DATATYPE_DLT645_XXX_XXX:
        case DATATYPE_DLT645_XX_XXXX :
        case DATATYPE_DLT645_X_XXX :
        case DATATYPE_DLT645_XX_XX :
        case DATATYPE_DLT645_XXXXXXXX :
        case DATATYPE_DLT645_XX :
        case DATATYPE_DLT645_XXX :
        case DATATYPE_DLT645_XXXX:
        
            *pFVal = DLT645_BCD_DatatoValue(pBCDBuf,dlt645V07_DataTypeToByteNum(DataType),dlt645V07_DataTypeToBfactor(DataType));
            return 1;
        
        default:
            return 0;
    }

}