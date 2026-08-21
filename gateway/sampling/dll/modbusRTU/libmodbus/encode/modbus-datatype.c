#include <stdio.h>
#include <math.h>
#include "modbus-datatype.h"
#include "modbus-data.h"


/************************************************************************
函数功能：BCDDatatoValue   根据计算参数的值
输入参数：pBCDBuf   数据帧(BCD格式)首地址
          length    该参数的数据帧长度
          bfactor   参数的比率值(根据数据类型有所不同)
输出参数：
返回值  ：参数的实际值
说明    ：
************************************************************************/
double  BCDDatatoValue(const uint8_t *pBCDBuf, uint8_t length,double bfactor)
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


/*********************************************************************************************************
** Function name:       Modbus_swap64bit
** Descriptions:        
** input parameters:    None
** output parameters:   None
** Returned value:      0: success  other:fail
*********************************************************************************************************/
static void Modbus_swap64bit(unsigned char *p)
{
    uint8_t tmp[8];

    /*  */
    memcpy(tmp, p, 8);

    p[0] = tmp[7];
    p[1] = tmp[6];
    p[2] = tmp[5];
    p[3] = tmp[4];
    p[4] = tmp[3];
    p[5] = tmp[2];
    p[6] = tmp[1];
    p[7] = tmp[0];
}

/*********************************************************************************************************
** Function name:       Modbus_swap64bit
** Descriptions:        
** input parameters:    None
** output parameters:   None
** Returned value:      0: success  other:fail
*********************************************************************************************************/
static void Modbus_swap32bitIn64bit(unsigned char *p)
{
    uint8_t tmp[8];

    /*  */
    memcpy(tmp, p, 8);

    p[0] = tmp[4];
    p[1] = tmp[5];
    p[2] = tmp[6];
    p[3] = tmp[7];
    p[4] = tmp[0];
    p[5] = tmp[1];
    p[6] = tmp[2];
    p[7] = tmp[3];
}

/************************************************************************
函数功能：  ModbusRTURegDatatoDouble()
            寄存器数据转换为数据
输入参数：  DataType  数据类型
                            详细请查看mydatatype.h 
            pRegAddr  当前需转换的寄存器的地址
输出参数：  
返回值  ：  -1          失败
            非-1        当前使用的寄存器数量（由于数据类型多样，占用1、16、32、64）
            
            0       1       2       3     4       5       6       7  
            A       B       C       D     E       F       G       H
说明    ：
************************************************************************/
int32_t Modbus_DoubleToReg16(uint16_t wDataType, uint16_t xth, uint16_t *pRegValue, double *pFVal)
{
    int16_t             s16Tmp;
    uint16_t            u16Tmp;
    int32_t             s32Tmp;
    uint32_t            u32Tmp;
    uint64_t            u64Tmp;
    int64_t             s64Tmp;
    double              f64Tmp;
    double              f64TmpRound;
    float               f32Tmp;
    unsigned char      *p;
    union DataType_u32Tof32 unU32ToF32;

    if(xth > 3)
        return -1;

    f64Tmp = *pFVal;
    f64TmpRound = round(f64Tmp);

    switch(wDataType)
    {
    case DATATYPE_DATA_BITS:                // 0
        *pRegValue = (uint16_t)f64TmpRound;
        return 1;
    case DATATYPE_DATA_U16_AB:            // 
        u16Tmp = f64TmpRound;
        *pRegValue  = (u16Tmp);        
        return 1;
    case DATATYPE_DATA_S16_AB:            // 
        s16Tmp = f64TmpRound;
        *pRegValue  = (s16Tmp);    
        return 1;
    case DATATYPE_DATA_U16_BA:            // 
        u16Tmp = f64TmpRound;
        *pRegValue  = SWAP_16BIT(u16Tmp);   
        return 1;
    case DATATYPE_DATA_S16_BA:            // 
        s16Tmp =f64TmpRound;
        *pRegValue  = SWAP_16BIT(s16Tmp);   
        return 1;
    case DATATYPE_DATA_U32_ABCD:      // 
        if(xth > 2) 
            return -1;
        u32Tmp = f64TmpRound;
        u32Tmp = SWAP_32BIT(u32Tmp);
        p = (uint8_t*)&u32Tmp;
        *pRegValue =  (uint16_t)p[xth*2] << 8 | (uint16_t)p[xth*2+1];
        return 2;
    case DATATYPE_DATA_S32_ABCD:      // 
        if(xth > 2) 
            return -1;
        s32Tmp = f64TmpRound;
        s32Tmp = SWAP_32BIT(s32Tmp);
        p = (uint8_t*)&s32Tmp;
        *pRegValue =  (uint16_t)p[xth*2] << 8 | (uint16_t)p[xth*2+1];
        return 2;
    case DATATYPE_DATA_U32_BADC:      // 
        if(xth > 2) 
            return -1;
        u32Tmp = f64TmpRound;
        u32Tmp = SWAP_32BIT(u32Tmp);
        p = (uint8_t*)&u32Tmp;
        *pRegValue =  (uint16_t)p[xth*2] | (uint16_t)p[xth*2+1] << 8;        
        return 2;
    case DATATYPE_DATA_S32_BADC:      // 
        if(xth > 2) 
            return -1;
        s32Tmp = f64TmpRound;
        s32Tmp = SWAP_32BIT(s32Tmp);
        p = (uint8_t*)&s32Tmp;
        *pRegValue =  (uint16_t)p[xth*2] | (uint16_t)p[xth*2+1] << 8;       
        return 2;
   
    case DATATYPE_DATA_U32_CDAB:      // 
        if(xth > 2) 
            return -1;
        u32Tmp = f64TmpRound;
        p = (uint8_t*)&u32Tmp;
        *pRegValue = p[xth*2] | (uint16_t)p[xth*2+1] << 8;

        return 2;
    case DATATYPE_DATA_S32_CDAB:      
       if(xth > 2) 
            return -1;
        s32Tmp = f64TmpRound;
        p = (uint8_t*)&s32Tmp;
        *pRegValue = p[xth*2] | (uint16_t)p[xth*2+1] << 8;
        return 2;
    case DATATYPE_DATA_U32_DCBA:      
        if(xth > 2) 
            return -1;
        u32Tmp = f64TmpRound;
        p = (uint8_t*)&u32Tmp;
        *pRegValue = (uint16_t)p[xth*2] << 8 | (uint16_t)p[xth*2+1];
        return 2;
    case DATATYPE_DATA_S32_DCBA:     
        if(xth > 2) 
            return -1;
        s32Tmp = f64TmpRound;
        p = (uint8_t*)&s32Tmp;
        *pRegValue = (uint16_t)p[xth*2] << 8 | (uint16_t)p[xth*2+1];
        return 2;  
    case DATATYPE_DATA_F32_ABCD:   
        if(xth > 2) 
            return -1;
        unU32ToF32.f32Val = f64Tmp;
        unU32ToF32.u32Val = SWAP_32BIT(unU32ToF32.u32Val);
        p = (uint8_t*)&unU32ToF32.u32Val;
        *pRegValue =  (uint16_t)p[xth*2] << 8 | (uint16_t)p[xth*2+1];
        return 2;
    case DATATYPE_DATA_F32_BADC:      
        if(xth > 2) 
            return -1;
        unU32ToF32.f32Val = f64Tmp;
        unU32ToF32.u32Val = SWAP_32BIT(unU32ToF32.u32Val);
        p = (uint8_t*)&unU32ToF32.u32Val;
        *pRegValue =  (uint16_t)p[xth*2] | (uint16_t)p[xth*2+1] << 8;
        return 2;
    case DATATYPE_DATA_F32_CDAB:      // 
         if(xth > 2) 
            return -1;
        f32Tmp = f64Tmp;
        p = (uint8_t*)&f32Tmp;
        *pRegValue =  (uint16_t)p[xth*2] | (uint16_t)p[xth*2+1] << 8;
        return 2;
    case DATATYPE_DATA_F32_DCBA:      // 
        if(xth > 2) 
            return -1;
        f32Tmp = f64Tmp;
        p = (uint8_t*)&f32Tmp;
        *pRegValue =  (uint16_t)p[xth*2] << 8 | (uint16_t)p[xth*2+1];
        return 2;
    case DATATYPE_DATA_D64_ABCDEFGH:
        p =  (uint8_t *)&f64Tmp;
        Modbus_swap64bit(p);
        *pRegValue =  (uint16_t)p[xth*2] << 8 | (uint16_t)p[xth*2+1];
        return 4;
    case DATATYPE_DATA_D64_GHEFCDAB:
         p =  (uint8_t *)&f64Tmp;
        *pRegValue =  (uint16_t)p[xth*2]  | (uint16_t)p[xth*2+1] << 8;
        return 4;
    case DATATYPE_DATA_D64_BADCFEHG:
        p =  (uint8_t *)&f64Tmp;
        Modbus_swap64bit(p);
        *pRegValue =  (uint16_t)p[xth*2] | (uint16_t)p[xth*2+1] << 8;
        return 4;
    case DATATYPE_DATA_D64_HGFEDCBA:
        p =  (uint8_t *)&f64Tmp;
        *pRegValue =  (uint16_t)p[xth*2] << 8 | (uint16_t)p[xth*2+1];
        return 4;
    case DATATYPE_DATA_D64_EFGHABCD:
        p =  (uint8_t *)&f64Tmp;
        Modbus_swap64bit(p);
        Modbus_swap32bitIn64bit(p);
        *pRegValue =  (uint16_t)p[xth*2] << 8 | (uint16_t)p[xth*2+1];
        return 4;
    case DATATYPE_DATA_D64_CDABGHEF:
        p =  (uint8_t *)&f64Tmp;
        Modbus_swap32bitIn64bit(p);
        *pRegValue =  (uint16_t)p[xth*2]  | (uint16_t)p[xth*2+1] << 8;
        return 4;
    case DATATYPE_DATA_D64_FEHGBADC:
         p =  (uint8_t *)&f64Tmp;
        Modbus_swap64bit(p);
        Modbus_swap32bitIn64bit(p);
        *pRegValue =  (uint16_t)p[xth*2] | (uint16_t)p[xth*2+1]  << 8;
        return 4;
    case DATATYPE_DATA_D64_DCBAHGFE:
        p =  (uint8_t *)&f64Tmp;
        Modbus_swap32bitIn64bit(p);
        *pRegValue =  (uint16_t)p[xth*2] << 8 | (uint16_t)p[xth*2+1] ;
        return 4;
    case DATATYPE_DATA_S64_ABCDEFGH:
        s64Tmp = f64TmpRound;
        p =  (uint8_t *)&s64Tmp;
        Modbus_swap64bit(p);
        *pRegValue =  (uint16_t)p[xth*2] << 8 | (uint16_t)p[xth*2+1];
        return 4;
    case DATATYPE_DATA_S64_GHEFCDAB:
        s64Tmp = f64TmpRound;
        p =  (uint8_t *)&s64Tmp;
        *pRegValue =  (uint16_t)p[xth*2]  | (uint16_t)p[xth*2+1] << 8;
        return 4;
    case DATATYPE_DATA_S64_BADCFEHG:
        s64Tmp = f64TmpRound;
        p =  (uint8_t *)&s64Tmp;
        Modbus_swap64bit(p);
        *pRegValue =  (uint16_t)p[xth*2] | (uint16_t)p[xth*2+1] << 8;
        return 4;
    case DATATYPE_DATA_S64_HGFEDCBA:
        s64Tmp = f64TmpRound;
        p =  (uint8_t *)&s64Tmp;
        *pRegValue =  (uint16_t)p[xth*2] << 8 | (uint16_t)p[xth*2+1];
        return 4;
    case DATATYPE_DATA_U64_ABCDEFGH:
        u64Tmp = f64TmpRound;
        p =  (uint8_t *)&u64Tmp;
        Modbus_swap64bit(p);
        *pRegValue =  (uint16_t)p[xth*2] << 8 | (uint16_t)p[xth*2+1];
        return 4;
    case DATATYPE_DATA_U64_GHEFCDAB:
        u64Tmp = f64TmpRound;
        p =  (uint8_t *)&u64Tmp;
        *pRegValue =  (uint16_t)p[xth*2]  | (uint16_t)p[xth*2+1] << 8;
        return 4;
    case DATATYPE_DATA_U64_BADCFEHG:
        u64Tmp = f64TmpRound;
        p =  (uint8_t *)&u64Tmp;
        Modbus_swap64bit(p);
        *pRegValue =  (uint16_t)p[xth*2] | (uint16_t)p[xth*2+1] << 8;
        return 4;
    case DATATYPE_DATA_U64_HGFEDCBA:
        u64Tmp = f64TmpRound;
        p =  (uint8_t *)&u64Tmp;
        *pRegValue =  (uint16_t)p[xth*2] << 8 | (uint16_t)p[xth*2+1];
        return 4;
    default:
        return -1;

    }    
}

/************************************************************************
函数功能：  ModbusRTURegDatatoDouble()
            寄存器数据转换为数据
输入参数：  DataType  数据类型
                            详细请查看mydatatype.h 
            pRegAddr  当前需转换的寄存器的地址
输出参数：  
返回值  ：  -1          失败
            非-1        当前使用的寄存器数量（由于数据类型多样，占用1、16、32、64）
            
            0       1       2       3     4       5       6       7  
            A       B       C       D     E       F       G       H
说明    ：
************************************************************************/
int32_t Modbus_DoubleToReg16_CtrlCmd(uint16_t wDataType, uint16_t xth, uint16_t *pRegValue, double *pFVal)
{
    int16_t             s16Tmp;
    uint16_t            u16Tmp;
    int32_t             s32Tmp;
    uint32_t            u32Tmp;
    uint64_t            u64Tmp;
    int64_t             s64Tmp;
    double              f64Tmp;
    double              f64TmpRound;
    float               f32Tmp;
    unsigned char      *p;
    union DataType_u32Tof32 unU32ToF32;

    if(xth > 3)
        return -1;

    f64Tmp = *pFVal;
    f64TmpRound = round(f64Tmp);

    switch(wDataType)
    {
    case DATATYPE_DATA_BITS:                // 0
        *pRegValue = (uint16_t)f64TmpRound;
        return 1;
    case DATATYPE_DATA_U16_AB:            // 
        u16Tmp = f64TmpRound;
        *pRegValue  = (u16Tmp);        
        return 1;
    case DATATYPE_DATA_S16_AB:            // 
        s16Tmp = f64TmpRound;
        *pRegValue  = (s16Tmp);    
        return 1;
    case DATATYPE_DATA_U16_BA:            // 
        u16Tmp = f64TmpRound;
        *pRegValue  = SWAP_16BIT(u16Tmp);   
        return 1;
    case DATATYPE_DATA_S16_BA:            // 
        s16Tmp =f64TmpRound;
        *pRegValue  = SWAP_16BIT(s16Tmp);   
        return 1;
    case DATATYPE_DATA_U32_ABCD:      // 
        if(xth > 2) 
            return -1;
        u32Tmp = f64TmpRound;
        u32Tmp = SWAP_32BIT(u32Tmp);
        p = (uint8_t*)&u32Tmp;
        *pRegValue =  (uint16_t)p[xth*2] << 8 | (uint16_t)p[xth*2+1];
        return 2;
    case DATATYPE_DATA_S32_ABCD:      // 
        if(xth > 2) 
            return -1;
        s32Tmp = f64TmpRound;
        s32Tmp = SWAP_32BIT(s32Tmp);
        p = (uint8_t*)&s32Tmp;
        *pRegValue =  (uint16_t)p[xth*2] << 8 | (uint16_t)p[xth*2+1];
        return 2;
    case DATATYPE_DATA_U32_BADC:      // 
        if(xth > 2) 
            return -1;
        u32Tmp = f64TmpRound;
        u32Tmp = SWAP_32BIT(u32Tmp);
        p = (uint8_t*)&u32Tmp;
        *pRegValue =  (uint16_t)p[xth*2] | (uint16_t)p[xth*2+1] << 8;        
        return 2;
    case DATATYPE_DATA_S32_BADC:      // 
        if(xth > 2) 
            return -1;
        s32Tmp = f64TmpRound;
        s32Tmp = SWAP_32BIT(s32Tmp);
        p = (uint8_t*)&s32Tmp;
        *pRegValue =  (uint16_t)p[xth*2] | (uint16_t)p[xth*2+1] << 8;       
        return 2;
   
    case DATATYPE_DATA_U32_DCBA:      // 
        if(xth > 2) 
            return -1;
        u32Tmp = f64TmpRound;
        p = (uint8_t*)&u32Tmp;
        *pRegValue = p[xth*2] | (uint16_t)p[xth*2+1] << 8;
        return 2;
    case DATATYPE_DATA_S32_DCBA:
       if(xth > 2) 
            return -1;
        s32Tmp = f64TmpRound;
        p = (uint8_t*)&s32Tmp;
        *pRegValue = p[xth*2] | (uint16_t)p[xth*2+1] << 8;
        return 2;
    case DATATYPE_DATA_U32_CDAB:
        if(xth > 2) 
            return -1;
        u32Tmp = f64TmpRound;
        p = (uint8_t*)&u32Tmp;
        *pRegValue = (uint16_t)p[xth*2] << 8 | (uint16_t)p[xth*2+1];
        return 2;
    case DATATYPE_DATA_S32_CDAB:
        if(xth > 2) 
            return -1;
        s32Tmp = f64TmpRound;
        p = (uint8_t*)&s32Tmp;
        *pRegValue = (uint16_t)p[xth*2] << 8 | (uint16_t)p[xth*2+1];
        return 2;  
    case DATATYPE_DATA_F32_ABCD:   
        if(xth > 2) 
            return -1;
        unU32ToF32.f32Val = f64Tmp;
        unU32ToF32.u32Val = SWAP_32BIT(unU32ToF32.u32Val);
        p = (uint8_t*)&unU32ToF32.u32Val;
        *pRegValue =  (uint16_t)p[xth*2] << 8 | (uint16_t)p[xth*2+1];
        return 2;
    case DATATYPE_DATA_F32_BADC:      
        if(xth > 2) 
            return -1;
        unU32ToF32.f32Val = f64Tmp;
        unU32ToF32.u32Val = SWAP_32BIT(unU32ToF32.u32Val);
        p = (uint8_t*)&unU32ToF32.u32Val;
        *pRegValue =  (uint16_t)p[xth*2] | (uint16_t)p[xth*2+1] << 8;
        return 2;
    case DATATYPE_DATA_F32_CDAB:      // 
         if(xth > 2) 
            return -1;
        f32Tmp = f64Tmp;
        p = (uint8_t*)&f32Tmp;
        *pRegValue =  (uint16_t)p[xth*2] | (uint16_t)p[xth*2+1] << 8;
        return 2;
    case DATATYPE_DATA_F32_DCBA:      // 
        if(xth > 2) 
            return -1;
        f32Tmp = f64Tmp;
        p = (uint8_t*)&f32Tmp;
        *pRegValue =  (uint16_t)p[xth*2] << 8 | (uint16_t)p[xth*2+1];
        return 2;
    case DATATYPE_DATA_D64_ABCDEFGH:
        p =  (uint8_t *)&f64Tmp;
        Modbus_swap64bit(p);
        *pRegValue =  (uint16_t)p[xth*2] << 8 | (uint16_t)p[xth*2+1];
        return 4;
    case DATATYPE_DATA_D64_GHEFCDAB:
         p =  (uint8_t *)&f64Tmp;
        *pRegValue =  (uint16_t)p[xth*2]  | (uint16_t)p[xth*2+1] << 8;
        return 4;
    case DATATYPE_DATA_D64_BADCFEHG:
        p =  (uint8_t *)&f64Tmp;
        Modbus_swap64bit(p);
        *pRegValue =  (uint16_t)p[xth*2] | (uint16_t)p[xth*2+1] << 8;
        return 4;
    case DATATYPE_DATA_D64_HGFEDCBA:
        p =  (uint8_t *)&f64Tmp;
        *pRegValue =  (uint16_t)p[xth*2] << 8 | (uint16_t)p[xth*2+1];
        return 4;
    case DATATYPE_DATA_D64_EFGHABCD:
        p =  (uint8_t *)&f64Tmp;
        Modbus_swap64bit(p);
        Modbus_swap32bitIn64bit(p);
        *pRegValue =  (uint16_t)p[xth*2] << 8 | (uint16_t)p[xth*2+1];
        return 4;
    case DATATYPE_DATA_D64_CDABGHEF:
        p =  (uint8_t *)&f64Tmp;
        Modbus_swap32bitIn64bit(p);
        *pRegValue =  (uint16_t)p[xth*2]  | (uint16_t)p[xth*2+1] << 8;
        return 4;
    case DATATYPE_DATA_D64_FEHGBADC:
         p =  (uint8_t *)&f64Tmp;
        Modbus_swap64bit(p);
        Modbus_swap32bitIn64bit(p);
        *pRegValue =  (uint16_t)p[xth*2] | (uint16_t)p[xth*2+1]  << 8;
        return 4;
    case DATATYPE_DATA_D64_DCBAHGFE:
        p =  (uint8_t *)&f64Tmp;
        Modbus_swap32bitIn64bit(p);
        *pRegValue =  (uint16_t)p[xth*2] << 8 | (uint16_t)p[xth*2+1] ;
        return 4;
    case DATATYPE_DATA_S64_ABCDEFGH:
        s64Tmp = f64TmpRound;
        p =  (uint8_t *)&s64Tmp;
        Modbus_swap64bit(p);
        *pRegValue =  (uint16_t)p[xth*2] << 8 | (uint16_t)p[xth*2+1];
        return 4;
    case DATATYPE_DATA_S64_GHEFCDAB:
        s64Tmp = f64TmpRound;
        p =  (uint8_t *)&s64Tmp;
        *pRegValue =  (uint16_t)p[xth*2]  | (uint16_t)p[xth*2+1] << 8;
        return 4;
    case DATATYPE_DATA_S64_BADCFEHG:
        s64Tmp = f64TmpRound;
        p =  (uint8_t *)&s64Tmp;
        Modbus_swap64bit(p);
        *pRegValue =  (uint16_t)p[xth*2] | (uint16_t)p[xth*2+1] << 8;
        return 4;
    case DATATYPE_DATA_S64_HGFEDCBA:
        s64Tmp = f64TmpRound;
        p =  (uint8_t *)&s64Tmp;
        *pRegValue =  (uint16_t)p[xth*2] << 8 | (uint16_t)p[xth*2+1];
        return 4;
    case DATATYPE_DATA_U64_ABCDEFGH:
        u64Tmp = f64TmpRound;
        p =  (uint8_t *)&u64Tmp;
        Modbus_swap64bit(p);
        *pRegValue =  (uint16_t)p[xth*2] << 8 | (uint16_t)p[xth*2+1];
        return 4;
    case DATATYPE_DATA_U64_GHEFCDAB:
        u64Tmp = f64TmpRound;
        p =  (uint8_t *)&u64Tmp;
        *pRegValue =  (uint16_t)p[xth*2]  | (uint16_t)p[xth*2+1] << 8;
        return 4;
    case DATATYPE_DATA_U64_BADCFEHG:
        u64Tmp = f64TmpRound;
        p =  (uint8_t *)&u64Tmp;
        Modbus_swap64bit(p);
        *pRegValue =  (uint16_t)p[xth*2] | (uint16_t)p[xth*2+1] << 8;
        return 4;
    case DATATYPE_DATA_U64_HGFEDCBA:
        u64Tmp = f64TmpRound;
        p =  (uint8_t *)&u64Tmp;
        *pRegValue =  (uint16_t)p[xth*2] << 8 | (uint16_t)p[xth*2+1];
        return 4;
    default:
        return -1;
    }
}

/************************************************************************
函数功能：  ModbusRTURegDatatoDouble()
            寄存器数据转换为数据
输入参数：  DataType  数据类型
                            详细请查看mydatatype.h 
            pRegAddr  当前需转换的寄存器的地址
输出参数：  
返回值  ：  -1          失败
            非-1        当前使用的寄存器数量（由于数据类型多样，占用1、16、32、64）
            
            0       1       2       3     4       5       6       7  
            A       B       C       D     E       F       G       H
说明    ：
************************************************************************/
int32_t Modbus_RegDatatoDouble(uint16_t wDataType, uint16_t wBitField, uint16_t *pRegAddr, double *pFVal)
{
	uint8_t             u8Tmp;
    int16_t             s16Tmp;
    uint16_t            u16Tmp;
    int32_t             s32Tmp;
    uint32_t            u32Tmp;
    uint64_t            u64Tmp;
    int64_t             s64Tmp;
    double              f64Tmp;
    char                *p;
    float               signValue = 1.0;
    union DataType_u32Tof32 unU32ToF32;
    switch(wDataType)
    {
    case DATATYPE_DATA_BITS:                // 0
        if(wBitField > 15)
            return -1;
        u16Tmp = ((*pRegAddr) >> wBitField) & 0x01;
        *pFVal = (double)u16Tmp;
        return 1;
    case DATATYPE_DATA_U16_AB:            // 
        u16Tmp = (*pRegAddr);
        *pFVal  = (double)u16Tmp;        
        return 1;
    case DATATYPE_DATA_S16_AB:            // 
        s16Tmp = (*pRegAddr);
        *pFVal  = (double)s16Tmp;
        return 1;
    case DATATYPE_DATA_ST16_AB:
        u16Tmp = (*pRegAddr)&0x07FFF;
        if((*pRegAddr) &0x8000)
        {
            signValue = -1.0;
        }
        *pFVal  = (double)u16Tmp * signValue;
        return 1;
    case DATATYPE_DATA_U16_BA:            // 
        u16Tmp = SWAP_16BIT(*pRegAddr);
        *pFVal  = (double)u16Tmp;
        return 1;
    case DATATYPE_DATA_S16_BA:            // 
        s16Tmp = SWAP_16BIT(*pRegAddr);
        *pFVal  = (double)s16Tmp;
        return 1;
    case DATATYPE_DATA_ST16_BA:
        u16Tmp = (SWAP_16BIT(*pRegAddr))&0x07FFF;
        if((SWAP_16BIT(*pRegAddr)) &0x8000)
        {
            signValue = -1.0;
        }
        *pFVal  = (double)u16Tmp * signValue;
        return 1;
    case DATATYPE_DATA_U32_ABCD:      // 
        u16Tmp = (pRegAddr[0]);
        u32Tmp = ((pRegAddr[1])) | (u16Tmp << 16);
        *pFVal  = (double)u32Tmp;
        return 2;
    case DATATYPE_DATA_S32_ABCD:      // 
        u16Tmp = (pRegAddr[0]);     
        s32Tmp = ((pRegAddr[1])) | (u16Tmp << 16);         
        *pFVal  = (double)s32Tmp;      
        return 2;
    case DATATYPE_DATA_ST32_ABCD:      // 
        u16Tmp = (pRegAddr[0]);     
        u32Tmp = ((pRegAddr[1])) | (u16Tmp << 16);
        if(u32Tmp & 0x80000000)
        {
            signValue = -1.0;
        }
        *pFVal  = (double)(u32Tmp & 0x7FFFFFFF) * signValue;      
        return 2;
	case DATATYPE_DATA_ABx10000_CD:
		u16Tmp = (pRegAddr[0]);
        u32Tmp = ((pRegAddr[1])) + (u16Tmp * 10000);
        *pFVal  = (double)u32Tmp;
		return 2;
	case DATATYPE_DATA_CDx10000_AB:
		u16Tmp = (pRegAddr[0]);
        u32Tmp = ((pRegAddr[1] * 10000)) + (u16Tmp);
        *pFVal  = (double)u32Tmp;
		return 2;
    case DATATYPE_DATA_U32_BADC:      // 
        u16Tmp = SWAP_16BIT(pRegAddr[0]);
        u32Tmp = (SWAP_16BIT(pRegAddr[1])) | (u16Tmp << 16);        
        *pFVal  = (double)u32Tmp;           
        return 2;
    case DATATYPE_DATA_S32_BADC:      // 
        u16Tmp = SWAP_16BIT(pRegAddr[0]);
        s32Tmp = (SWAP_16BIT(pRegAddr[1])) | (u16Tmp << 16);
        *pFVal  = (double)s32Tmp;
        return 2;
    case DATATYPE_DATA_ST32_BADC:      // 
        u16Tmp = SWAP_16BIT(pRegAddr[0]);
        u32Tmp = (SWAP_16BIT(pRegAddr[1])) | (u16Tmp << 16);
        if(u32Tmp & 0x80000000)
        {
            signValue = -1.0;
        }
        *pFVal  = (double)(u32Tmp & 0x7FFFFFFF) * signValue;      
        return 2;
    case DATATYPE_DATA_U32_CDAB:      // 
        u16Tmp = (pRegAddr[0]);
        u32Tmp = (pRegAddr[1] << 16) | (u16Tmp);
        *pFVal  = (double)u32Tmp;
        return 2;
    case DATATYPE_DATA_S32_CDAB:      
        u16Tmp = (pRegAddr[0]);
        s32Tmp = (pRegAddr[1] << 16) | (u16Tmp);
        *pFVal  = (double)s32Tmp;
        return 2;
    case DATATYPE_DATA_ST32_CDAB:      
        u16Tmp = (pRegAddr[0]);
        u32Tmp = (pRegAddr[1] << 16) | (u16Tmp);
        if(u32Tmp & 0x80000000)
        {
            signValue = -1.0;
        }
        *pFVal  = (double)(u32Tmp & 0x7FFFFFFF) * signValue;    
        return 2;
    case DATATYPE_DATA_U32_DCBA:      
        u16Tmp = SWAP_16BIT(pRegAddr[0]);
        u32Tmp = (SWAP_16BIT(pRegAddr[1]) << 16) | (u16Tmp);
        *pFVal  = (double)u32Tmp;
        return 2;
    case DATATYPE_DATA_S32_DCBA:     
        u16Tmp = SWAP_16BIT(pRegAddr[0]);
        s32Tmp = (SWAP_16BIT(pRegAddr[1]) << 16) | (u16Tmp);
        *pFVal  = (double)s32Tmp;
        return 2;
    case DATATYPE_DATA_ST32_DCBA:     
        u16Tmp = SWAP_16BIT(pRegAddr[0]);
        u32Tmp = (SWAP_16BIT(pRegAddr[1]) << 16) | (u16Tmp);
        if(u32Tmp & 0x80000000)
        {
            signValue = -1.0;
        }
        *pFVal  = (double)(u32Tmp & 0x7FFFFFFF) * signValue;  
        return 2;
    case DATATYPE_DATA_F32_ABCD:      
        u16Tmp = (pRegAddr[0]);
        u32Tmp = ((pRegAddr[1])) | (u16Tmp << 16);
        unU32ToF32.u32Val = u32Tmp;
        *pFVal  = (double)unU32ToF32.f32Val;
        return 2;
    case DATATYPE_DATA_F32_BADC:      
        u16Tmp = SWAP_16BIT(pRegAddr[0]);
        u32Tmp = (SWAP_16BIT(pRegAddr[1])) | (u16Tmp << 16);
        unU32ToF32.u32Val = u32Tmp;
        *pFVal  = (double)unU32ToF32.f32Val;
        return 2;
    case DATATYPE_DATA_F32_CDAB:      // 
        u16Tmp = (pRegAddr[0]);
        u32Tmp = ((pRegAddr[1]) << 16) | (u16Tmp);
        unU32ToF32.u32Val = u32Tmp;
        *pFVal  = (double)unU32ToF32.f32Val;
        return 2;
    case DATATYPE_DATA_F32_DCBA:      // 
        u16Tmp = SWAP_16BIT(pRegAddr[0]);
        u32Tmp = (SWAP_16BIT(pRegAddr[1]) << 16) | u16Tmp;
        unU32ToF32.u32Val = u32Tmp;
        *pFVal  = (double)unU32ToF32.f32Val;
        return 2;
    case DATATYPE_DATA_D64_ABCDEFGH:
        p =  (char *)&f64Tmp;
        p[7] =  (pRegAddr[0] >> 8) & 0xff;
        p[6] =  pRegAddr[0] & 0xff;
        p[5] =  (pRegAddr[1] >> 8) & 0xff;
        p[4] =  pRegAddr[1] & 0xff;
        p[3] =  (pRegAddr[2] >> 8) & 0xff;
        p[2] =  pRegAddr[2] & 0xff;
        p[1] =  (pRegAddr[3] >> 8) & 0xff;
        p[0] =  pRegAddr[3] & 0xff;
        *pFVal  = f64Tmp;
        return 4;
    case DATATYPE_DATA_D64_GHEFCDAB:
        p =  (char *)&f64Tmp;
        p[7] =  (pRegAddr[3] >> 8) & 0xff;
        p[6] =  pRegAddr[3] & 0xff;
        p[5] =  (pRegAddr[2] >> 8) & 0xff;
        p[4] =  pRegAddr[2] & 0xff;
        p[3] =  (pRegAddr[1] >> 8) & 0xff;
        p[2] =  pRegAddr[1] & 0xff;
        p[1] =  (pRegAddr[0] >> 8) & 0xff;
        p[0] =  pRegAddr[0] & 0xff;
        *pFVal  = f64Tmp;
        return 4;
    case DATATYPE_DATA_D64_BADCFEHG:
        p =  (char *)&f64Tmp;
        p[7] =  pRegAddr[0] & 0xff;
        p[6] =  (pRegAddr[0] >> 8) & 0xff;
        p[5] =  pRegAddr[1] & 0xff;
        p[4] =  (pRegAddr[1] >> 8) & 0xff;
        p[3] =  pRegAddr[2] & 0xff;
        p[2] =  (pRegAddr[2] >> 8) & 0xff;        
        p[1] =  pRegAddr[3] & 0xff;
        p[0] =  (pRegAddr[3] >> 8) & 0xff;
        *pFVal  = f64Tmp;
        return 4;
    case DATATYPE_DATA_D64_HGFEDCBA:
        p =  (char *)&f64Tmp;
        p[7] =  pRegAddr[3] & 0xff;
        p[6] =  (pRegAddr[3] >> 8) & 0xff;
        p[5] =  pRegAddr[2] & 0xff;
        p[4] =  (pRegAddr[2] >> 8) & 0xff;
        p[3] =  pRegAddr[1] & 0xff;
        p[2] =  (pRegAddr[1] >> 8) & 0xff;
        p[1] =  pRegAddr[0] & 0xff;
        p[0] =  (pRegAddr[0] >> 8) & 0xff;
        *pFVal  = f64Tmp;
        return 4;
    case DATATYPE_DATA_D64_EFGHABCD:
        p =  (char *)&f64Tmp;
        p[7] =  (pRegAddr[2] >> 8) & 0xff;
        p[6] =  pRegAddr[2] & 0xff;
        p[5] =  (pRegAddr[3] >> 8) & 0xff;
        p[4] =  pRegAddr[3] & 0xff;
        p[3] =  (pRegAddr[0] >> 8) & 0xff;
        p[2] =  pRegAddr[0] & 0xff;
        p[1] =  (pRegAddr[1] >> 8) & 0xff;
        p[0] =  pRegAddr[1] & 0xff;
        *pFVal  = f64Tmp;
        return 4;
    case DATATYPE_DATA_D64_CDABGHEF:
        p =  (char *)&f64Tmp;
        p[7] =  (pRegAddr[1] >> 8) & 0xff;
        p[6] =  pRegAddr[1] & 0xff;
        p[5] =  (pRegAddr[0] >> 8) & 0xff;
        p[4] =  pRegAddr[0] & 0xff;
        p[3] =  (pRegAddr[3] >> 8) & 0xff;
        p[2] =  pRegAddr[3] & 0xff;
        p[1] =  (pRegAddr[2] >> 8) & 0xff;
        p[0] =  pRegAddr[2] & 0xff;
        *pFVal  = f64Tmp;
        return 4;
    case DATATYPE_DATA_D64_FEHGBADC:
        p =  (char *)&f64Tmp;
        p[7] =  pRegAddr[2] & 0xff;
        p[6] =  (pRegAddr[2] >> 8) & 0xff;
        p[5] =  pRegAddr[3] & 0xff;
        p[4] =  (pRegAddr[3] >> 8) & 0xff;
        p[3] =  pRegAddr[0] & 0xff;
        p[2] =  (pRegAddr[0] >> 8) & 0xff;
        p[1] =  pRegAddr[1] & 0xff;
        p[0] =  (pRegAddr[1] >> 8) & 0xff;
        *pFVal  = f64Tmp;
        return 4;
    case DATATYPE_DATA_D64_DCBAHGFE:
        p =  (char *)&f64Tmp;
        p[7] =  pRegAddr[1] & 0xff;
        p[6] =  (pRegAddr[1] >> 8) & 0xff;
        p[5] =  pRegAddr[0] & 0xff;
        p[4] =  (pRegAddr[0] >> 8) & 0xff;
        p[3] =  pRegAddr[3] & 0xff;
        p[2] =  (pRegAddr[3] >> 8) & 0xff;
        p[1] =  pRegAddr[2] & 0xff;
        p[0] =  (pRegAddr[2] >> 8) & 0xff;
        *pFVal  = f64Tmp;
        return 4;
    case DATATYPE_DATA_S64_ABCDEFGH:
        p =  (char *)&s64Tmp;
        p[7] =  (pRegAddr[0] >> 8) & 0xff;
        p[6] =  pRegAddr[0] & 0xff;
        p[5] =  (pRegAddr[1] >> 8) & 0xff;
        p[4] =  pRegAddr[1] & 0xff;
        p[3] =  (pRegAddr[2] >> 8) & 0xff;
        p[2] =  pRegAddr[2] & 0xff;
        p[1] =  (pRegAddr[3] >> 8) & 0xff;
        p[0] =  pRegAddr[3] & 0xff;
        *pFVal  = s64Tmp;
        return 4;
    case DATATYPE_DATA_S64_GHEFCDAB:
        p =  (char *)&s64Tmp;
        p[7] =  (pRegAddr[3] >> 8) & 0xff;
        p[6] =  pRegAddr[3] & 0xff;
        p[5] =  (pRegAddr[2] >> 8) & 0xff;
        p[4] =  pRegAddr[2] & 0xff;
        p[3] =  (pRegAddr[1] >> 8) & 0xff;
        p[2] =  pRegAddr[1] & 0xff;
        p[1] =  (pRegAddr[0] >> 8) & 0xff;
        p[0] =  pRegAddr[0] & 0xff;
        *pFVal  = s64Tmp;
        return 4;
    case DATATYPE_DATA_S64_BADCFEHG:
        p =  (char *)&s64Tmp;
        p[7] =  pRegAddr[0] & 0xff;
        p[6] =  (pRegAddr[0] >> 8) & 0xff;
        p[5] =  pRegAddr[1] & 0xff;
        p[4] =  (pRegAddr[1] >> 8) & 0xff;
        p[3] =  pRegAddr[2] & 0xff;
        p[2] =  (pRegAddr[2] >> 8) & 0xff;        
        p[1] =  pRegAddr[3] & 0xff;
        p[0] =  (pRegAddr[3] >> 8) & 0xff;
        *pFVal  = s64Tmp;
        return 4;
    case DATATYPE_DATA_S64_HGFEDCBA:
        p =  (char *)&s64Tmp;
        p[7] =  pRegAddr[3] & 0xff;
        p[6] =  (pRegAddr[3] >> 8) & 0xff;
        p[5] =  pRegAddr[2] & 0xff;
        p[4] =  (pRegAddr[2] >> 8) & 0xff;
        p[3] =  pRegAddr[1] & 0xff;
        p[2] =  (pRegAddr[1] >> 8) & 0xff;
        p[1] =  pRegAddr[0] & 0xff;
        p[0] =  (pRegAddr[0] >> 8) & 0xff;
        *pFVal  = s64Tmp;
        return 4;
    case DATATYPE_DATA_U64_ABCDEFGH:
        p =  (char *)&u64Tmp;
        p[7] =  (pRegAddr[0] >> 8) & 0xff;
        p[6] =  pRegAddr[0] & 0xff;
        p[5] =  (pRegAddr[1] >> 8) & 0xff;
        p[4] =  pRegAddr[1] & 0xff;
        p[3] =  (pRegAddr[2] >> 8) & 0xff;
        p[2] =  pRegAddr[2] & 0xff;
        p[1] =  (pRegAddr[3] >> 8) & 0xff;
        p[0] =  pRegAddr[3] & 0xff;
        *pFVal  = u64Tmp;
        return 4;
    case DATATYPE_DATA_U64_GHEFCDAB:
        p =  (char *)&u64Tmp;
        p[7] =  (pRegAddr[3] >> 8) & 0xff;
        p[6] =  pRegAddr[3] & 0xff;
        p[5] =  (pRegAddr[2] >> 8) & 0xff;
        p[4] =  pRegAddr[2] & 0xff;
        p[3] =  (pRegAddr[1] >> 8) & 0xff;
        p[2] =  pRegAddr[1] & 0xff;
        p[1] =  (pRegAddr[0] >> 8) & 0xff;
        p[0] =  pRegAddr[0] & 0xff;
        *pFVal  = u64Tmp;
        return 4;
    case DATATYPE_DATA_U64_BADCFEHG:
        p =  (char *)&u64Tmp;
        p[7] =  pRegAddr[0] & 0xff;
        p[6] =  (pRegAddr[0] >> 8) & 0xff;
        p[5] =  pRegAddr[1] & 0xff;
        p[4] =  (pRegAddr[1] >> 8) & 0xff;
        p[3] =  pRegAddr[2] & 0xff;
        p[2] =  (pRegAddr[2] >> 8) & 0xff;        
        p[1] =  pRegAddr[3] & 0xff;
        p[0] =  (pRegAddr[3] >> 8) & 0xff;
        *pFVal  = u64Tmp;
        return 4;
    case DATATYPE_DATA_U64_HGFEDCBA:
        p =  (char *)&u64Tmp;
        p[7] =  pRegAddr[3] & 0xff;
        p[6] =  (pRegAddr[3] >> 8) & 0xff;
        p[5] =  pRegAddr[2] & 0xff;
        p[4] =  (pRegAddr[2] >> 8) & 0xff;
        p[3] =  pRegAddr[1] & 0xff;
        p[2] =  (pRegAddr[1] >> 8) & 0xff;
        p[1] =  pRegAddr[0] & 0xff;
        p[0] =  (pRegAddr[0] >> 8) & 0xff;
        *pFVal  = u64Tmp;
        return 4;
	case DATATYPE_DATA_U8_HIGH:      // 
        u8Tmp = ((*pRegAddr) >> 8) & 0xFF;
        *pFVal  = (double)u8Tmp;
        return 1;
	case DATATYPE_DATA_U8_LOW:      // 
        u8Tmp = (*pRegAddr) & 0xFF;
        *pFVal  = (double)u8Tmp;
        return 1;

    case DATATYPE_DATA_BCD_AB:
        u16Tmp = (*pRegAddr);
        *pFVal  = BCDDatatoValue((uint8_t *)&u16Tmp, 2, 1);
        return 1;
    case DATATYPE_DATA_BCD_BA:
        u16Tmp = SWAP_16BIT(*pRegAddr);
        *pFVal  = BCDDatatoValue((uint8_t *)&u16Tmp, 2, 1);
        return 1;
    case DATATYPE_DATA_BCD_ABCD:
        u16Tmp = (pRegAddr[0]);
        u32Tmp = ((pRegAddr[1])) | (u16Tmp << 16);
        *pFVal  = BCDDatatoValue((uint8_t *)&u32Tmp, 4, 1);
        return 2;
    case DATATYPE_DATA_BCD_CDAB:
        u16Tmp = (pRegAddr[0]);
        u32Tmp = (pRegAddr[1] << 16) | (u16Tmp);
        *pFVal  = BCDDatatoValue((uint8_t *)&u32Tmp, 4, 1);
        return 2;
    case DATATYPE_DATA_BCD_ABCDEFGH:
        p =  (char *)&f64Tmp;
        p[7] =  (pRegAddr[0] >> 8) & 0xff;
        p[6] =  pRegAddr[0] & 0xff;
        p[5] =  (pRegAddr[1] >> 8) & 0xff;
        p[4] =  pRegAddr[1] & 0xff;
        p[3] =  (pRegAddr[2] >> 8) & 0xff;
        p[2] =  pRegAddr[2] & 0xff;
        p[1] =  (pRegAddr[3] >> 8) & 0xff;
        p[0] =  pRegAddr[3] & 0xff;
        *pFVal  = BCDDatatoValue((uint8_t *)&f64Tmp, 8, 1);
        return 4;

    case DATATYPE_DATA_BCD_GHEFCDAB:
        p =  (char *)&f64Tmp;
        p[7] =  (pRegAddr[3] >> 8) & 0xff;
        p[6] =  pRegAddr[3] & 0xff;
        p[5] =  (pRegAddr[2] >> 8) & 0xff;
        p[4] =  pRegAddr[2] & 0xff;
        p[3] =  (pRegAddr[1] >> 8) & 0xff;
        p[2] =  pRegAddr[1] & 0xff;
        p[1] =  (pRegAddr[0] >> 8) & 0xff;
        p[0] =  pRegAddr[0] & 0xff;
        *pFVal  = BCDDatatoValue((uint8_t *)&f64Tmp, 8, 1);
        return 4;

    case DATATYPE_DATA_BCD_EFGHABCD:
        p =  (char *)&f64Tmp;
        p[7] =  (pRegAddr[2] >> 8) & 0xff;
        p[6] =  pRegAddr[2] & 0xff;
        p[5] =  (pRegAddr[3] >> 8) & 0xff;
        p[4] =  pRegAddr[3] & 0xff;
        p[3] =  (pRegAddr[0] >> 8) & 0xff;
        p[2] =  pRegAddr[0] & 0xff;
        p[1] =  (pRegAddr[1] >> 8) & 0xff;
        p[0] =  pRegAddr[1] & 0xff;
        *pFVal  = BCDDatatoValue((uint8_t *)&f64Tmp, 8, 1);
        return 4;
    case DATATYPE_DATA_BCD_CDABGHEF:
        p =  (char *)&f64Tmp;
        p[7] =  (pRegAddr[1] >> 8) & 0xff;
        p[6] =  pRegAddr[1] & 0xff;
        p[5] =  (pRegAddr[0] >> 8) & 0xff;
        p[4] =  pRegAddr[0] & 0xff;
        p[3] =  (pRegAddr[3] >> 8) & 0xff;
        p[2] =  pRegAddr[3] & 0xff;
        p[1] =  (pRegAddr[2] >> 8) & 0xff;
        p[0] =  pRegAddr[2] & 0xff;
        *pFVal  = BCDDatatoValue((uint8_t *)&f64Tmp, 8, 1);
        return 4;
    case DATATYPE_DATA_U32ABCD_DOT_F32ABCD:
        /* 
            U32.F32 
            先算整数部分，
         */
        u16Tmp = (pRegAddr[0]);
        u32Tmp = ((pRegAddr[1])) | (u16Tmp << 16);
        f64Tmp  = (double)u32Tmp;
        /*
            再算小数部分
        */
        u16Tmp = (pRegAddr[2]);
        u32Tmp = ((pRegAddr[3])) | (u16Tmp << 16);
        unU32ToF32.u32Val = u32Tmp;
        *pFVal  = f64Tmp + (double)unU32ToF32.f32Val;

        return 4;
    case DATATYPE_DATA_U32CDAB_DOT_F32CDAB:
        /* 
            U32.F32 
            先算整数部分，
         */
        u16Tmp = (pRegAddr[0]);
        u32Tmp = (pRegAddr[1] << 16) | (u16Tmp);
        f64Tmp  = (double)u32Tmp;
        /*
            再算小数部分
        */
        u16Tmp = (pRegAddr[2]);
        u32Tmp = ((pRegAddr[3]) << 16) | (u16Tmp);
        unU32ToF32.u32Val = u32Tmp;
        *pFVal  = f64Tmp + (double)unU32ToF32.f32Val;

        return 4;
    default:
        return -1;

    }    
}
/************************************************************************
函数功能：  ModbusRTUDataTypeToRegNum()
            数据类型转换成寄存器数量
输入参数：  DataType  数据类型
                            详细请查看mydatatype.h 
            pRegAddr  当前需转换的寄存器的地址
输出参数：  
返回值  ：  -1          失败
            非-1        当前使用的寄存器数量（由于数据类型多样，占用1、16、32、64）
说明    ：
************************************************************************/
int32_t Modbus_DataTypeToRegNum(uint16_t DataType)
{
    switch(DataType)
    {
    case DATATYPE_DATA_BITS:             
        return 1;
    case DATATYPE_DATA_U16_BA:          
        return 1;
    case DATATYPE_DATA_S16_BA:          
        return 1;
    case DATATYPE_DATA_U16_AB:          
        return 1;
    case DATATYPE_DATA_S16_AB:          
        return 1;
    case DATATYPE_DATA_U32_DCBA:      
        return 2;
    case DATATYPE_DATA_S32_DCBA:      
        return 2;
    case DATATYPE_DATA_U32_CDAB:      
        return 2;
    case DATATYPE_DATA_S32_CDAB:      
        return 2;
    case DATATYPE_DATA_U32_BADC:      
        return 2;
    case DATATYPE_DATA_S32_BADC:      
        return 2;
    case DATATYPE_DATA_U32_ABCD:      
        return 2;
    case DATATYPE_DATA_S32_ABCD:      
        return 2;
    case DATATYPE_DATA_F32_DCBA:      
        return 2;
    case DATATYPE_DATA_F32_CDAB:      
        return 2;
    case DATATYPE_DATA_F32_BADC:      
        return 2;
    case DATATYPE_DATA_F32_ABCD:      
        return 2;
    case DATATYPE_DATA_D64_ABCDEFGH:
        return 4;
    case DATATYPE_DATA_D64_GHEFCDAB:
        return 4;
    case DATATYPE_DATA_D64_BADCFEHG:
        return 4;
    case DATATYPE_DATA_D64_HGFEDCBA:
        return 4;
    case DATATYPE_DATA_D64_EFGHABCD:
        return 4;
    case DATATYPE_DATA_D64_CDABGHEF:
        return 4;
    case DATATYPE_DATA_D64_FEHGBADC:
        return 4;
    case DATATYPE_DATA_D64_DCBAHGFE:
        return 4;
    case DATATYPE_DATA_ST16_AB:
        return 1;
    case DATATYPE_DATA_ST16_BA:
        return 1;
    case DATATYPE_DATA_ST32_ABCD:
        return 2;
    case DATATYPE_DATA_ST32_BADC:
        return 2;
    case DATATYPE_DATA_ST32_CDAB:
        return 2;
    case DATATYPE_DATA_ST32_DCBA:
        return 2;
    case DATATYPE_DATA_S64_ABCDEFGH:
        return 4;
    case DATATYPE_DATA_S64_GHEFCDAB:
        return 4;
    case DATATYPE_DATA_S64_BADCFEHG:
        return 4;
    case DATATYPE_DATA_S64_HGFEDCBA:
        return 4;
    case DATATYPE_DATA_U64_ABCDEFGH:
        return 4;
    case DATATYPE_DATA_U64_GHEFCDAB:
        return 4;
    case DATATYPE_DATA_U64_BADCFEHG:
        return 4;
    case DATATYPE_DATA_U64_HGFEDCBA:
        return 4;
    case DATATYPE_DATA_ABx10000_CD:
        return 2;
    case DATATYPE_DATA_CDx10000_AB:
        return 2; 
    case DATATYPE_DATA_BCD_AB:
        return 1;
    case DATATYPE_DATA_BCD_BA:
        return 1;
    case DATATYPE_DATA_BCD_ABCD:
        return 2;
    case DATATYPE_DATA_BCD_CDAB:
        return 2;
    case DATATYPE_DATA_BCD_ABCDEFGH:
        return 4;
    case DATATYPE_DATA_BCD_GHEFCDAB:
        return 4;
    case DATATYPE_DATA_BCD_EFGHABCD:
        return 4;
    case DATATYPE_DATA_BCD_CDABGHEF:
        return 4;
    case DATATYPE_DATA_U32ABCD_DOT_F32ABCD:
        return 4;
    case DATATYPE_DATA_U32CDAB_DOT_F32CDAB:
        return 4;

        
    default:
        return -1;

    }    
}

