#include <stdio.h>
#include "cjt188_datatype.h"
#include "cjt188_data.h"
#include "math.h"

/************************************************************************
函数功能：DataTypeToByteNum   根据CJT188的格式计算参数的数据长度
输入参数：DataType   数据类型
输出参数：
返回值  ：参数的数据长度
说明    ：
************************************************************************/
static int32_t DataTypeToByteNum(uint16_t DataType)
{
    switch(DataType)
    {
        case DATATYPE_CJT188_XXXXXX_XX_U:        //0,带单位的XXXXXX.XX
            return 4;
        case DATATYPE_CJT188_XXXX_XXXX_U:        //1,带单位的XXXX.XXXX
            return 4;
        case DATATYPE_CJT188_XXXXXX_XX:          //2,不带单位的XXXXXX.XX
            return 4;
        case DATATYPE_CJT188_XXXXXX:             //3,不带单位的XXXXXX
            return 3;
        case DATATYPE_CJT188_XXXX_XX:            //4,不带单位的XXXX.XX
            return 3;
        case DATATYPE_CJT188_XXXX:               //5,不带单位的XXXX
            return 2;
        case DATATYPE_CJT188_DD:                 //6,BCD码,（结算、抄表）日期
            return 1;
        case DATATYPE_CJT188_YYYYMMDDHHMMSSS:    //7,时间格式
            return 7;
        case DATATYPE_CJT188_HH:                 //8,HEX
            return 1;
        default:
            return 0;
    }
}

/************************************************************************
函数功能：DataTypeToBfactor   根据CJT188的格式计算参数的比率值
输入参数：DataType   数据类型
输出参数：
返回值  ：参数的比率值
说明    ：
************************************************************************/
double DataTypeToBfactor(uint16_t DataType)
{
    switch(DataType)
    {
        case DATATYPE_CJT188_XXXXXX_XX_U:        //0,带单位的XXXXXX.XX
            return 1e-2;
        case DATATYPE_CJT188_XXXX_XXXX_U:        //1,带单位的XXXX.XXXX
            return 1e-4;
        case DATATYPE_CJT188_XXXXXX_XX:          //2,带单位的XXXXXX.XX
            return 1e-2;
        case DATATYPE_CJT188_XXXXXX:             //3,不带单位的XXXXXX
            return 1;
        case DATATYPE_CJT188_XXXX_XX:            //4,不带单位的XXXX.XX
            return 1e-2;
        case DATATYPE_CJT188_XXXX:               //5,不带单位的XXXX
            return 1;
        case DATATYPE_CJT188_DD:                 //6,BCD码,（结算、抄表）日期
            return 1;
        case DATATYPE_CJT188_HH:                 //7,HEX
            return 1;
        default:
            return 0;
    }
}


/************************************************************************
函数功能：DataTypeToByteNum   根据CJT188的格式计算参数的数据长度
输入参数：DataType   数据类型
输出参数：
返回值  ：参数的数据长度
说明    ：1.长度16H的偏移量0当前累积流量，1结束日累积流量，2实时时间，3表状态ST
          2..长度16H的偏移量0结算日热量，1当前热量，2热功率，3流量，4累积流量，5供水热量，
                                        6回水温度，7累积工作时间，8实时时间，9状态ST
************************************************************************/
static int32_t DataTypeToByteNumBefor(uint8_t metertype, uint8_t bitfield)
{

    if((metertype >= 0x20) && (metertype <= 0x29) )
    {
        switch(bitfield)
        {
            case 0:             //0, 结算日热量，len=5
                return 0;
            case 1:             //1，当前热量，len=5
                return 5;
            case 2:             //2，热功率，len=5
                return 10;
            case 3:             //3，流量，len=5
                return 15;
            case 4:             //4, 累积流量，len=5
                return 20;
            case 5:             //5，供水热量，len=3
                return 25;
            case 6:             //6，回水温度，len=3
                return 28;
            case 7:             //7，累积工作时间，len=3
                return 31;
            case 8:             //8, 实时时间，len=7
                return 34;
            case 9:             //9，状态ST，len=2
                return 41;
            default:
                return 0;
        }
    }
    else
    {
        switch(bitfield)
        {
            case 0:             //0, 当前累计流量，len=5
            return 0;
            case 1:             //1，结算日累计流量，len=5
                return 5;
            case 2:             //2，实时时间，len=7
                return 10;
            case 3:             //3，状态ST，len=2
                return 17;
            default:
                return 0;
        }
    }
}
/************************************************************************
函数功能：CJT188BCDDatatoValue   根据CJT188计算参数的值
输入参数：pBCDBuf   数据帧(BCD格式)首地址
          length    该参数的数据帧长度
          bfactor   参数的比率值(根据数据类型有所不同)
          unitfactor 参数的单位转换成标准的参数单位值
输出参数：
返回值  ：参数的实际值
说明    ：
************************************************************************/
int32_t  CJT188BCDDatatoValue(uint8_t *pBCDBuf, uint8_t DataType, uint8_t metertype, uint8_t bitfield, double *pFVal)
{
    uint8_t i, tmp,HHalfByte,LHalfByte;
    double Value = 0;
    uint16_t length = 0,beforlen=0;
    length = DataTypeToByteNum(DataType);
    beforlen = DataTypeToByteNumBefor(metertype,bitfield);

    for(i=0; i<length; i++)
    {   
        LHalfByte = pBCDBuf[i+beforlen]&0x0F ;
        HHalfByte = (pBCDBuf[i+beforlen]>>4)&0x0F;
        if((HHalfByte > 0x0A) || (LHalfByte > 0x0A))
        {
            return -1;
        }
        tmp = HHalfByte*10 + LHalfByte;   
        Value += tmp * pow(100,i);    
    }

    *pFVal = Value * DataTypeToBfactor(DataType);

    return 1;
}

