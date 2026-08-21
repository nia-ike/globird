#ifndef _DATA_TYPE_H
#define _DATA_TYPE_H

#include <stdint.h>

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

    DATATYPE_DATA_BITS,             // 0

    DATATYPE_DATA_U16_AB,          // 1
    DATATYPE_DATA_S16_AB,          // 2
    DATATYPE_DATA_U16_BA,          // 3
    DATATYPE_DATA_S16_BA,          // 4
    
    DATATYPE_DATA_U32_ABCD,      //   5
    DATATYPE_DATA_S32_ABCD,      //   6
    DATATYPE_DATA_U32_BADC,      //   7
    DATATYPE_DATA_S32_BADC,      //   8
    DATATYPE_DATA_U32_CDAB,      //   9
    DATATYPE_DATA_S32_CDAB,      //   10
    DATATYPE_DATA_U32_DCBA,      //   11
    DATATYPE_DATA_S32_DCBA,      //   12
    
    DATATYPE_DATA_F32_ABCD,      //   13
    DATATYPE_DATA_F32_BADC,      //   14
    DATATYPE_DATA_F32_CDAB,      //   15
    DATATYPE_DATA_F32_DCBA,      //   16
    
    DATATYPE_DATA_D64_ABCDEFGH,  //   17
    DATATYPE_DATA_D64_GHEFCDAB,  //   18
    DATATYPE_DATA_D64_BADCFEHG,  //   19
    DATATYPE_DATA_D64_HGFEDCBA,  //   20
    DATATYPE_DATA_D64_EFGHABCD,  //   21
    DATATYPE_DATA_D64_CDABGHEF,  //   22
    DATATYPE_DATA_D64_FEHGBADC,  //   23
    DATATYPE_DATA_D64_DCBAHGFE,  //   24

    DATATYPE_DATA_ST16_AB,       // 25
    DATATYPE_DATA_ST16_BA,       // 26
    DATATYPE_DATA_ST32_ABCD,     // 27
    DATATYPE_DATA_ST32_BADC,     // 28
    DATATYPE_DATA_ST32_CDAB,     // 29
    DATATYPE_DATA_ST32_DCBA,     // 30

    DATATYPE_DATA_S64_ABCDEFGH,  // 31
    DATATYPE_DATA_S64_GHEFCDAB,  // 32
    DATATYPE_DATA_S64_BADCFEHG,  // 33
    DATATYPE_DATA_S64_HGFEDCBA,  // 34
    DATATYPE_DATA_U64_ABCDEFGH,  // 35
    DATATYPE_DATA_U64_GHEFCDAB,  // 36
    DATATYPE_DATA_U64_BADCFEHG,  // 37
    DATATYPE_DATA_U64_HGFEDCBA,  // 38
    
	DATATYPE_DATA_U8_HIGH,       //39
	DATATYPE_DATA_U8_LOW,        //40

	DATATYPE_DATA_ABx10000_CD = 128,	// USER DEFINE    32BIT AB*10000+CD  (溯高美表)
	DATATYPE_DATA_CDx10000_AB = 129,	// USER DEFINE
    
    DATATYPE_DATA_BCD_AB ,	//  BCD AB  130
    DATATYPE_DATA_BCD_BA ,	//  BCD BA  131
    DATATYPE_DATA_BCD_ABCD ,	//  BCD ABCD  132
    DATATYPE_DATA_BCD_BADC,     //
    DATATYPE_DATA_BCD_CDAB ,	// BCD CDAB   134
    DATATYPE_DATA_BCD_DCBA,     //
    DATATYPE_DATA_BCD_ABCDEFGH,  //   ABCDEFGH  136
    DATATYPE_DATA_BCD_GHEFCDAB,  //   GHEFCDAB  137
    DATATYPE_DATA_BCD_BADCFEHG,  //
    DATATYPE_DATA_BCD_HGFEDCBA,  //
    DATATYPE_DATA_BCD_EFGHABCD,  //   EFGHABCD  140
    DATATYPE_DATA_BCD_CDABGHEF,  //   CDABGHEF  141
    DATATYPE_DATA_BCD_FEHGBADC,  //
    DATATYPE_DATA_BCD_DCBAHGFE,  //

    DATATYPE_DATA_U32ABCD_DOT_F32ABCD, //  U32ABCD.F32 => 整数为ABCD整型.小数为float  
    DATATYPE_DATA_U32CDAB_DOT_F32CDAB, //  U32CDAB.F32 => 整数为CDAB整型.小数为float  
    DATATYPE_DATA_MAX,

};

int32_t Modbus_DoubleToReg16(uint16_t wDataType, uint16_t xth, uint16_t *pRegValue, double *pFVal);
int32_t Modbus_DoubleToReg16_CtrlCmd(uint16_t wDataType, uint16_t xth, uint16_t *pRegValue, double *pFVal);
int32_t Modbus_RegDatatoDouble(uint16_t wDataType, uint16_t wBitField, uint16_t *pRegAddr, double *pFVal);
int32_t Modbus_DataTypeToRegNum(uint16_t DataType);


#endif

