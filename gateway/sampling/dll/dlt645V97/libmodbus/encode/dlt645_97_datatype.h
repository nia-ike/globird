#ifndef _DATA_TYPE_H
#define _DATA_TYPE_H

#include <stdint.h>
#include "dlt645_97_datatype.h"


union DataType_u32Tof32
{
    uint32_t    u32Val;
    float       f32Val;
};

union DataType_u64Tof64
{
    uint64_t    u64Val;
    double       f64Val;
};


/* 数据类型定义 */
enum DataType_Data
{
    /*
        
          16bits   =>  | BYTE1 | BYTE0 |

          32bits   =>  | BYTE3 | BYTE2 | BYTE1 | BYTE0 |   


    */
    DATATYPE_DLT645_XXXXXX_XX,
    DATATYPE_DLT645_XXX_X,
    DATATYPE_DLT645_XXX_XXX,
    DATATYPE_DLT645_XX_XXXX,
    DATATYPE_DLT645_X_XXX,
    DATATYPE_DLT645_XX_XX,
    DATATYPE_DLT645_XXXXXXXX,
    DATATYPE_DLT645_XX,
    DATATYPE_DLT645_XXX,
    DATATYPE_DLT645_XXXX,
    
    

    DATATYPE_DATA_MAX,

};


int32_t DLT645V07_DatatoValue(uint8_t *pBCDBuf, uint16_t DataType, double *pFVal);


#endif

