#ifndef _DATA_TYPE_H
#define _DATA_TYPE_H

#include <stdint.h>
#include "cjt188_datatype.h"



/* 数据类型定义 */
enum CJT188DataType
{
	DATATYPE_CJT188_XXXXXX_XX_U,        //0,带单位的XXXXXX.XX
    DATATYPE_CJT188_XXXX_XXXX_U,        //1,带单位的XXXX.XXXX
    DATATYPE_CJT188_XXXXXX_XX,          //2,不带单位的XXXXXX.XX
	DATATYPE_CJT188_XXXXXX,             //3,不带单位的XXXXXX
	DATATYPE_CJT188_XXXX_XX,            //4,不带单位的XXXX.XX
    DATATYPE_CJT188_DD,                 //6,BCD码,（结算、抄表）日期
    DATATYPE_CJT188_XXXX,               //5,不带单位的XXXX
    DATATYPE_CJT188_YYYYMMDDHHMMSSS,    //7,时间格式
    DATATYPE_CJT188_HH                  //8,HEX
};


int32_t  CJT188BCDDatatoValue(uint8_t *pBCDBuf, uint8_t DataType, uint8_t metertype, uint8_t bitfield, double *pFVal);
#endif

