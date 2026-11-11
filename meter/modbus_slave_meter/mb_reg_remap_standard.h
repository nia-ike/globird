/*
* Copyright (c) 2013,珠海派诺电子有限公司
* All rights reserved.
* 
* 文件名称：
* 文件标识：
* 描    述：
* 
* 当前版本：V1.0
* 作    者：chenzy
* 更改说明：
* 完成日期：20131108
*
* 修改记录 
*  作者       时间         版本      修改描述
*  chenzy   2020.6.09   V1.00      创建
*/


#ifndef _MB_REG_REMAP_STANDARD_H_
#define _MB_REG_REMAP_STANDARD_H_

#include <stdint.h>
#include <stddef.h>


#include <stdio.h>
#include <string.h>
#include "db_struct.h"
#include "mb_cfg.h"


#define __DYNAMICS_DATABASE_STRUCT__

/*
    在模块中定义的静态实体结构体才能使用本宏
    
*/
#ifndef __DYNAMICS_DATABASE_STRUCT__

#define REG_ALIGN_U16(x)                    ((uint16_t*)&x) 
#define REG_ALIGN_U32_H(x)                  ((uint16_t*)&x + 1)
#define REG_ALIGN_U32_L(x)                  ((uint16_t*)&x)
#define REG_ALIGN_U64_GH(x)                 ((uint16_t*)&x + 3)
#define REG_ALIGN_U64_EF(x)                 ((uint16_t*)&x + 2)
#define REG_ALIGN_U64_CD(x)                 ((uint16_t*)&x + 1)
#define REG_ALIGN_U64_AB(x)                 ((uint16_t*)&x)




/*
    用作modbus 【写参数】时，找出对应的参数边界范围（db_cfgsysDefault 和 db_cfguserDefault  db_cfgsys 和 db_cfguser ），用作写参数时的边界限制
    注意：
    1.通常大多数仪表的参数配置区的参数顺序定义与modbus协议一致，是不需要这个偏移来对应检索。但少量仪表 参数配置区与协议定义顺序不一致，就需要映射
    这时候在modbus【写参数】时，须根据偏移查找出该寄存器对应的参数在参数存储区和边界表的对应关系.
    
    2. 实时数据不需要写，所以偏移填0
    
    3. IAR 不支持 typeof，无法省略 结构体类型填写，  MDK支持typeof（编译选项增加-gnu），所以  REG_CFG_OFFSET_IN_DISK 和 REG_CFG_REMAP，可省略结构体填写
       除了stm8必须使用IAR以外，其他全部使用MDK，所以仍然保留typeof用法，所以暂时保留2种接口方式
*/
#ifdef _IAR_
#define REG_CFG_OFFSET_IN_DISK(t, m)        (offsetof(t, m) / sizeof(uint16_t))     // t 是结构体类型
#else
#define REG_CFG_OFFSET_IN_DISK(t, m)        (offsetof(typeof(t), m) / sizeof(uint16_t)) // t 是结构体名称， 因为内部用typeof 转换成 结构体类型了
#endif

/*
    寄存器映射的简写
    t - type    结构体类型
    s - struct  结构体名称
    m - member  结构体成员
*/
#ifdef _IAR_
#define REG_CFG_REMAP(t, s, m)                  {REG_ALIGN_U16(s.m), REG_CFG_OFFSET_IN_DISK(t, m), REG_ORDER_LITTLE_EDIAN} 
#else
#define REG_CFG_REMAP(s, m)                     {REG_ALIGN_U16(s.m), REG_CFG_OFFSET_IN_DISK(s, m), REG_ORDER_LITTLE_EDIAN} 
#endif

#define REG_READ_ALIGN_U16(x,y)                 {REG_ALIGN_U16(x),    0, y                 }
#define REG_READ_ALIGN_U32_H(x,y)               {REG_ALIGN_U32_H(x),  0, y                 }
#define REG_READ_ALIGN_U32_L(x,y)               {REG_ALIGN_U32_L(x),  0, y                 }
#define REG_READ_ALIGN_U64_GH(x,y)              {REG_ALIGN_U64_GH(x), 0, y                 }
#define REG_READ_ALIGN_U64_EF(x,y)              {REG_ALIGN_U64_EF(x), 0, y                 }
#define REG_READ_ALIGN_U64_CD(x,y)              {REG_ALIGN_U64_CD(x), 0, y                 }
#define REG_READ_ALIGN_U64_AB(x,y)              {REG_ALIGN_U64_AB(x), 0, y                 }

#else 
/*
    动态结构体。（结构体是外部指针，比如共享内存等应用时，只有结构体类型，传递指针是动态的，所以无法定义时初始化, 因此需要用偏移量）

    ABCDEFGH 为字节序 标识代号。     A 为高   H为低字节。  
*/

#define REG_OFFSET_OF_VMEM(x)                   (offsetof(struct db_database_type, x) / sizeof(uint16_t)) // 获取 当前X成员变量在 struct db_database_type 中的偏移量（以modbus单个寄存器大小为基本单元，即2bytes）

#define REG_ALIGN_ORIGIN(x)                     (REG_OFFSET_OF_VMEM(x))
#define REG_ALIGN_U16(x)                        (REG_OFFSET_OF_VMEM(x))
#define REG_ALIGN_U32_H(x)                      (REG_OFFSET_OF_VMEM(x) + 1)
#define REG_ALIGN_U32_L(x)                      (REG_OFFSET_OF_VMEM(x))
#define REG_ALIGN_U64_GH(x)                     (REG_OFFSET_OF_VMEM(x) + 0)
#define REG_ALIGN_U64_EF(x)                     (REG_OFFSET_OF_VMEM(x) + 1)
#define REG_ALIGN_U64_CD(x)                     (REG_OFFSET_OF_VMEM(x) + 2)
#define REG_ALIGN_U64_AB(x)                     (REG_OFFSET_OF_VMEM(x) + 3)

#define REG_READ_ALIGN_U16(x,y)                 {0, REG_ALIGN_U16(x),   REG_ALIGN_ORIGIN(x), 1,  y}
#define REG_READ_ALIGN_U32_CD(x,y)              {0, REG_ALIGN_U32_L(x), REG_ALIGN_ORIGIN(x), 2,  y}
#define REG_READ_ALIGN_U32_AB(x,y)              {0, REG_ALIGN_U32_H(x), REG_ALIGN_ORIGIN(x), 2,  y}
#define REG_READ_ALIGN_U64_GH(x,y)              {0, REG_ALIGN_U64_GH(x),REG_ALIGN_ORIGIN(x), 4,  y}
#define REG_READ_ALIGN_U64_EF(x,y)              {0, REG_ALIGN_U64_EF(x),REG_ALIGN_ORIGIN(x), 4,  y}
#define REG_READ_ALIGN_U64_CD(x,y)              {0, REG_ALIGN_U64_CD(x),REG_ALIGN_ORIGIN(x), 4,  y}
#define REG_READ_ALIGN_U64_AB(x,y)              {0, REG_ALIGN_U64_AB(x),REG_ALIGN_ORIGIN(x), 4,  y}

/*
    有些数据为内部即可生成，无需共享内存，因此数据可直接去地址
*/
#define REG_WITH_ADDR_ALIGN_U16(x)                    ((uint16_t*)&x) 
#define REG_WITH_ADDR_ALIGN_U32_H(x)                  ((uint16_t*)&x + 1)
#define REG_WITH_ADDR_ALIGN_U32_L(x)                  ((uint16_t*)&x)
#define REG_WITH_ADDR_ALIGN_U64_GH(x)                 ((uint16_t*)&x + 3)
#define REG_WITH_ADDR_ALIGN_U64_EF(x)                 ((uint16_t*)&x + 2)
#define REG_WITH_ADDR_ALIGN_U64_CD(x)                 ((uint16_t*)&x + 1)
#define REG_WITH_ADDR_ALIGN_U64_AB(x)                 ((uint16_t*)&x)

#define REG_READ_WITH_ADDR_ALIGN_U16(x,y)             {REG_WITH_ADDR_ALIGN_U16(x),    0, 0, 1,  y}
#define REG_READ_WITH_ADDR_ALIGN_U32_CD(x,y)          {REG_WITH_ADDR_ALIGN_U32_L(x) , 0, 0, 2,  y}
#define REG_READ_WITH_ADDR_ALIGN_U32_AB(x,y)          {REG_WITH_ADDR_ALIGN_U32_H(x) , 0, 0, 2,  y}
#define REG_READ_WITH_ADDR_ALIGN_U64_GH(x,y)          {REG_WITH_ADDR_ALIGN_U64_GH(x), 0, 0, 4,  y}
#define REG_READ_WITH_ADDR_ALIGN_U64_EF(x,y)          {REG_WITH_ADDR_ALIGN_U64_EF(x), 0, 0, 4,  y}
#define REG_READ_WITH_ADDR_ALIGN_U64_CD(x,y)          {REG_WITH_ADDR_ALIGN_U64_CD(x), 0, 0, 4,  y}
#define REG_READ_WITH_ADDR_ALIGN_U64_AB(x,y)          {REG_WITH_ADDR_ALIGN_U64_AB(x), 0, 0, 4,  y}


#endif


/*
    有些数据为结构体中连续定义且与协议文档一致，无需重映射时， 可使用如下定义信息表即可，无需一个个映射
*/
#define REG_READ_ALIGN_BLOCK(x, t, y)              {0, REG_OFFSET_OF_VMEM(x), sizeof(t)/sizeof(uint16_t), y}

//自定义长度
#define REG_READ_ALIGN_BLOCK_WITH_LEN(x, t, l, y)     {0, REG_OFFSET_OF_VMEM(x), l, y}

/*
    寄存器大小端定义
    在modbus报文需要按大端模式传输。 
    但CPU按小端模式定义变量，所以在modbus打包函数中会做大小端交换。
    在字符串定义时（比如 “PMAC503C”）需按大端传输，因此打包时不必交换
*/
#define REG_ORDER_LITTLE_EDIAN                    0
#define REG_ORDER_BIG_EDIAN                       1




/***************0地址开始的自定义寄存器段************************************************************************/
static uint16_t metertype_grid = 3;
static uint16_t softwarever = 100, hardwarever = 100;

struct remap_Modbus_node remap_opwow_info_grid[] = {
    // METER TYPE
    REG_READ_WITH_ADDR_ALIGN_U16(metertype_grid,                    REG_ORDER_LITTLE_EDIAN), 

    // MODEL
    REG_READ_ALIGN_U16( db_cfg_info.device.device[0],                  REG_ORDER_BIG_EDIAN),
    REG_READ_ALIGN_U16( db_cfg_info.device.device[2],                  REG_ORDER_BIG_EDIAN),
    REG_READ_ALIGN_U16( db_cfg_info.device.device[4],                  REG_ORDER_BIG_EDIAN),
    REG_READ_ALIGN_U16( db_cfg_info.device.device[6],                  REG_ORDER_BIG_EDIAN),
    REG_READ_ALIGN_U16( db_cfg_info.device.device[8],                  REG_ORDER_BIG_EDIAN),
    REG_READ_ALIGN_U16( db_cfg_info.device.device[10],                 REG_ORDER_BIG_EDIAN),
    REG_READ_ALIGN_U16( db_cfg_info.device.device[12],                 REG_ORDER_BIG_EDIAN),
    REG_READ_ALIGN_U16( db_cfg_info.device.device[14],                 REG_ORDER_BIG_EDIAN),

    // SN
    REG_READ_ALIGN_U16( db_null.dummy,                          REG_ORDER_LITTLE_EDIAN),  
    REG_READ_ALIGN_U16( db_null.dummy,                          REG_ORDER_LITTLE_EDIAN),  
    REG_READ_ALIGN_U16( db_null.dummy,                          REG_ORDER_LITTLE_EDIAN),  
    REG_READ_ALIGN_U16( db_null.dummy,                          REG_ORDER_LITTLE_EDIAN),  
    REG_READ_ALIGN_U16( db_null.dummy,                          REG_ORDER_LITTLE_EDIAN),  
    REG_READ_ALIGN_U16( db_null.dummy,                          REG_ORDER_LITTLE_EDIAN),   
    REG_READ_ALIGN_U16( db_null.dummy,                          REG_ORDER_LITTLE_EDIAN),  
    REG_READ_ALIGN_U16( db_null.dummy,                          REG_ORDER_LITTLE_EDIAN),  

    REG_READ_WITH_ADDR_ALIGN_U16(softwarever,                    REG_ORDER_LITTLE_EDIAN), 
    REG_READ_WITH_ADDR_ALIGN_U16(softwarever,                    REG_ORDER_LITTLE_EDIAN), 
    REG_READ_WITH_ADDR_ALIGN_U16(softwarever,                    REG_ORDER_LITTLE_EDIAN), 
    REG_READ_WITH_ADDR_ALIGN_U16(softwarever,                    REG_ORDER_LITTLE_EDIAN), 

    REG_READ_WITH_ADDR_ALIGN_U16(hardwarever,                    REG_ORDER_LITTLE_EDIAN), 
    REG_READ_WITH_ADDR_ALIGN_U16(hardwarever,                    REG_ORDER_LITTLE_EDIAN), 
    REG_READ_WITH_ADDR_ALIGN_U16(hardwarever,                    REG_ORDER_LITTLE_EDIAN), 
    REG_READ_WITH_ADDR_ALIGN_U16(hardwarever,                    REG_ORDER_LITTLE_EDIAN), 
};


struct remap_Modbus_node remap_opwow_value_grid[] = {
    // ENERGY
    REG_READ_ALIGN_U64_EF(db_measurebase[0].db_measure.energy.realtime.kWhImp[0] , REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_GH(db_measurebase[0].db_measure.energy.realtime.kWhImp[0] , REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_EF(db_measurebase[0].db_measure.energy.realtime.kWhExp[0] , REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_GH(db_measurebase[0].db_measure.energy.realtime.kWhExp[0] , REG_ORDER_LITTLE_EDIAN),

    REG_READ_ALIGN_U64_EF(db_measurebase[0].db_measure.energy.realtime.kWhImp[1] , REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_GH(db_measurebase[0].db_measure.energy.realtime.kWhImp[1] , REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_EF(db_measurebase[0].db_measure.energy.realtime.kWhExp[1] , REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_GH(db_measurebase[0].db_measure.energy.realtime.kWhExp[1] , REG_ORDER_LITTLE_EDIAN),

    REG_READ_ALIGN_U64_EF(db_measurebase[0].db_measure.energy.realtime.kWhImp[2] , REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_GH(db_measurebase[0].db_measure.energy.realtime.kWhImp[2] , REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_EF(db_measurebase[0].db_measure.energy.realtime.kWhExp[2] , REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_GH(db_measurebase[0].db_measure.energy.realtime.kWhExp[2] , REG_ORDER_LITTLE_EDIAN),

    REG_READ_ALIGN_U64_EF(db_measurebase[0].db_measure.energy.realtime.kWhImp[3] , REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_GH(db_measurebase[0].db_measure.energy.realtime.kWhImp[3] , REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_EF(db_measurebase[0].db_measure.energy.realtime.kWhExp[3] , REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_GH(db_measurebase[0].db_measure.energy.realtime.kWhExp[3] , REG_ORDER_LITTLE_EDIAN),

    //  
    REG_READ_ALIGN_U32_CD( db_measurebase[0].db_measure.numeric.halfwave.primary.Va,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U32_CD( db_measurebase[0].db_measure.numeric.halfwave.primary.Vb,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U32_CD( db_measurebase[0].db_measure.numeric.halfwave.primary.Vc,      REG_ORDER_LITTLE_EDIAN),

    REG_READ_ALIGN_U32_CD( db_measurebase[0].db_measure.numeric.halfwave.primary.Ia,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U32_CD( db_measurebase[0].db_measure.numeric.halfwave.primary.Ib,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U32_CD( db_measurebase[0].db_measure.numeric.halfwave.primary.Ic,      REG_ORDER_LITTLE_EDIAN),

    REG_READ_ALIGN_U32_CD( db_measurebase[0].db_measure.numeric.halfwave.primary.Hz,      REG_ORDER_LITTLE_EDIAN), // 

    REG_READ_ALIGN_U64_EF( db_measurebase[0].db_measure.numeric.halfwave.primary.Pa,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_GH( db_measurebase[0].db_measure.numeric.halfwave.primary.Pa,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_EF( db_measurebase[0].db_measure.numeric.halfwave.primary.Pb,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_GH( db_measurebase[0].db_measure.numeric.halfwave.primary.Pb,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_EF( db_measurebase[0].db_measure.numeric.halfwave.primary.Pc,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_GH( db_measurebase[0].db_measure.numeric.halfwave.primary.Pc,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_EF( db_measurebase[0].db_measure.numeric.halfwave.primary.Ptot,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_GH( db_measurebase[0].db_measure.numeric.halfwave.primary.Ptot,      REG_ORDER_LITTLE_EDIAN),

    REG_READ_ALIGN_U64_EF( db_measurebase[0].db_measure.numeric.halfwave.primary.Qa,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_GH( db_measurebase[0].db_measure.numeric.halfwave.primary.Qa,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_EF( db_measurebase[0].db_measure.numeric.halfwave.primary.Qb,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_GH( db_measurebase[0].db_measure.numeric.halfwave.primary.Qb,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_EF( db_measurebase[0].db_measure.numeric.halfwave.primary.Qc,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_GH( db_measurebase[0].db_measure.numeric.halfwave.primary.Qc,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_EF( db_measurebase[0].db_measure.numeric.halfwave.primary.Qtot,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_GH( db_measurebase[0].db_measure.numeric.halfwave.primary.Qtot,      REG_ORDER_LITTLE_EDIAN),

    REG_READ_ALIGN_U64_EF( db_measurebase[0].db_measure.numeric.halfwave.primary.Sa,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_GH( db_measurebase[0].db_measure.numeric.halfwave.primary.Sa,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_EF( db_measurebase[0].db_measure.numeric.halfwave.primary.Sb,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_GH( db_measurebase[0].db_measure.numeric.halfwave.primary.Sb,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_EF( db_measurebase[0].db_measure.numeric.halfwave.primary.Sc,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_GH( db_measurebase[0].db_measure.numeric.halfwave.primary.Sc,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_EF( db_measurebase[0].db_measure.numeric.halfwave.primary.Stot,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_GH( db_measurebase[0].db_measure.numeric.halfwave.primary.Stot,      REG_ORDER_LITTLE_EDIAN),

    REG_READ_ALIGN_U16( db_measurebase[0].db_measure.numeric.halfwave.primary.PFa,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U16( db_measurebase[0].db_measure.numeric.halfwave.primary.PFb,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U16( db_measurebase[0].db_measure.numeric.halfwave.primary.PFc,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U16( db_measurebase[0].db_measure.numeric.halfwave.primary.PFtot,    REG_ORDER_LITTLE_EDIAN),
};



struct remap_Modbus_node remap_opwow_info_pv[] = {
    // METER TYPE
    REG_READ_WITH_ADDR_ALIGN_U16(metertype_grid,                    REG_ORDER_LITTLE_EDIAN), 

    // MODEL
    REG_READ_ALIGN_U16( db_cfg_info.device.device[0],                  REG_ORDER_BIG_EDIAN),
    REG_READ_ALIGN_U16( db_cfg_info.device.device[2],                  REG_ORDER_BIG_EDIAN),
    REG_READ_ALIGN_U16( db_cfg_info.device.device[4],                  REG_ORDER_BIG_EDIAN),
    REG_READ_ALIGN_U16( db_cfg_info.device.device[6],                  REG_ORDER_BIG_EDIAN),
    REG_READ_ALIGN_U16( db_cfg_info.device.device[8],                  REG_ORDER_BIG_EDIAN),
    REG_READ_ALIGN_U16( db_cfg_info.device.device[10],                 REG_ORDER_BIG_EDIAN),
    REG_READ_ALIGN_U16( db_cfg_info.device.device[12],                 REG_ORDER_BIG_EDIAN),
    REG_READ_ALIGN_U16( db_cfg_info.device.device[14],                 REG_ORDER_BIG_EDIAN),

    // SN
    REG_READ_ALIGN_U16( db_null.dummy,                          REG_ORDER_LITTLE_EDIAN),  
    REG_READ_ALIGN_U16( db_null.dummy,                          REG_ORDER_LITTLE_EDIAN),  
    REG_READ_ALIGN_U16( db_null.dummy,                          REG_ORDER_LITTLE_EDIAN),  
    REG_READ_ALIGN_U16( db_null.dummy,                          REG_ORDER_LITTLE_EDIAN),  
    REG_READ_ALIGN_U16( db_null.dummy,                          REG_ORDER_LITTLE_EDIAN),  
    REG_READ_ALIGN_U16( db_null.dummy,                          REG_ORDER_LITTLE_EDIAN),   
    REG_READ_ALIGN_U16( db_null.dummy,                          REG_ORDER_LITTLE_EDIAN),  
    REG_READ_ALIGN_U16( db_null.dummy,                          REG_ORDER_LITTLE_EDIAN),  

    REG_READ_WITH_ADDR_ALIGN_U16(softwarever,                    REG_ORDER_LITTLE_EDIAN), 
    REG_READ_WITH_ADDR_ALIGN_U16(softwarever,                    REG_ORDER_LITTLE_EDIAN), 
    REG_READ_WITH_ADDR_ALIGN_U16(softwarever,                    REG_ORDER_LITTLE_EDIAN), 
    REG_READ_WITH_ADDR_ALIGN_U16(softwarever,                    REG_ORDER_LITTLE_EDIAN), 

    REG_READ_WITH_ADDR_ALIGN_U16(hardwarever,                    REG_ORDER_LITTLE_EDIAN), 
    REG_READ_WITH_ADDR_ALIGN_U16(hardwarever,                    REG_ORDER_LITTLE_EDIAN), 
    REG_READ_WITH_ADDR_ALIGN_U16(hardwarever,                    REG_ORDER_LITTLE_EDIAN), 
    REG_READ_WITH_ADDR_ALIGN_U16(hardwarever,                    REG_ORDER_LITTLE_EDIAN), 
};


struct remap_Modbus_node remap_opwow_value_pv[] = {
    // ENERGY
    REG_READ_ALIGN_U64_EF(db_measurebase[1].db_measure.energy.realtime.kWhImp[0] , REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_GH(db_measurebase[1].db_measure.energy.realtime.kWhImp[0] , REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_EF(db_measurebase[1].db_measure.energy.realtime.kWhExp[0] , REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_GH(db_measurebase[1].db_measure.energy.realtime.kWhExp[0] , REG_ORDER_LITTLE_EDIAN),

    REG_READ_ALIGN_U64_EF(db_measurebase[1].db_measure.energy.realtime.kWhImp[1] , REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_GH(db_measurebase[1].db_measure.energy.realtime.kWhImp[1] , REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_EF(db_measurebase[1].db_measure.energy.realtime.kWhExp[1] , REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_GH(db_measurebase[1].db_measure.energy.realtime.kWhExp[1] , REG_ORDER_LITTLE_EDIAN),

    REG_READ_ALIGN_U64_EF(db_measurebase[1].db_measure.energy.realtime.kWhImp[2] , REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_GH(db_measurebase[1].db_measure.energy.realtime.kWhImp[2] , REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_EF(db_measurebase[1].db_measure.energy.realtime.kWhExp[2] , REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_GH(db_measurebase[1].db_measure.energy.realtime.kWhExp[2] , REG_ORDER_LITTLE_EDIAN),

    REG_READ_ALIGN_U64_EF(db_measurebase[1].db_measure.energy.realtime.kWhImp[3] , REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_GH(db_measurebase[1].db_measure.energy.realtime.kWhImp[3] , REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_EF(db_measurebase[1].db_measure.energy.realtime.kWhExp[3] , REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_GH(db_measurebase[1].db_measure.energy.realtime.kWhExp[3] , REG_ORDER_LITTLE_EDIAN),

    //  
    REG_READ_ALIGN_U32_CD( db_measurebase[1].db_measure.numeric.fullwave.primary.Va,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U32_CD( db_measurebase[1].db_measure.numeric.fullwave.primary.Vb,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U32_CD( db_measurebase[1].db_measure.numeric.fullwave.primary.Vc,      REG_ORDER_LITTLE_EDIAN),

    REG_READ_ALIGN_U32_CD( db_measurebase[1].db_measure.numeric.fullwave.primary.Ia,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U32_CD( db_measurebase[1].db_measure.numeric.fullwave.primary.Ib,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U32_CD( db_measurebase[1].db_measure.numeric.fullwave.primary.Ic,      REG_ORDER_LITTLE_EDIAN),

    REG_READ_ALIGN_U32_CD( db_measurebase[1].db_measure.numeric.fullwave.primary.Hz,      REG_ORDER_LITTLE_EDIAN), // 

    REG_READ_ALIGN_U64_EF( db_measurebase[1].db_measure.numeric.fullwave.primary.Pa,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_GH( db_measurebase[1].db_measure.numeric.fullwave.primary.Pa,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_EF( db_measurebase[1].db_measure.numeric.fullwave.primary.Pb,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_GH( db_measurebase[1].db_measure.numeric.fullwave.primary.Pb,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_EF( db_measurebase[1].db_measure.numeric.fullwave.primary.Pc,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_GH( db_measurebase[1].db_measure.numeric.fullwave.primary.Pc,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_EF( db_measurebase[1].db_measure.numeric.fullwave.primary.Ptot,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_GH( db_measurebase[1].db_measure.numeric.fullwave.primary.Ptot,      REG_ORDER_LITTLE_EDIAN),

    REG_READ_ALIGN_U64_EF( db_measurebase[1].db_measure.numeric.fullwave.primary.Qa,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_GH( db_measurebase[1].db_measure.numeric.fullwave.primary.Qa,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_EF( db_measurebase[1].db_measure.numeric.fullwave.primary.Qb,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_GH( db_measurebase[1].db_measure.numeric.fullwave.primary.Qb,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_EF( db_measurebase[1].db_measure.numeric.fullwave.primary.Qc,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_GH( db_measurebase[1].db_measure.numeric.fullwave.primary.Qc,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_EF( db_measurebase[1].db_measure.numeric.fullwave.primary.Qtot,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_GH( db_measurebase[1].db_measure.numeric.fullwave.primary.Qtot,      REG_ORDER_LITTLE_EDIAN),

    REG_READ_ALIGN_U64_EF( db_measurebase[1].db_measure.numeric.fullwave.primary.Sa,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_GH( db_measurebase[1].db_measure.numeric.fullwave.primary.Sa,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_EF( db_measurebase[1].db_measure.numeric.fullwave.primary.Sb,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_GH( db_measurebase[1].db_measure.numeric.fullwave.primary.Sb,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_EF( db_measurebase[1].db_measure.numeric.fullwave.primary.Sc,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_GH( db_measurebase[1].db_measure.numeric.fullwave.primary.Sc,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_EF( db_measurebase[1].db_measure.numeric.fullwave.primary.Stot,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U64_GH( db_measurebase[1].db_measure.numeric.fullwave.primary.Stot,      REG_ORDER_LITTLE_EDIAN),

    REG_READ_ALIGN_U16( db_measurebase[1].db_measure.numeric.fullwave.primary.PFa,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U16( db_measurebase[1].db_measure.numeric.fullwave.primary.PFb,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U16( db_measurebase[1].db_measure.numeric.fullwave.primary.PFc,      REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U16( db_measurebase[1].db_measure.numeric.fullwave.primary.PFtot,    REG_ORDER_LITTLE_EDIAN),
};


/***************公共寄存器段************************************************************************/

struct remap_Modbus_node remapDevInfo[] = {
    /* device */
    REG_READ_ALIGN_U16( db_cfg_info.device.SpecialSymbol,           REG_ORDER_LITTLE_EDIAN),    //0
    REG_READ_ALIGN_U16( db_cfg_info.device.device[0],               REG_ORDER_BIG_EDIAN),    //1
    REG_READ_ALIGN_U16( db_cfg_info.device.device[2],               REG_ORDER_BIG_EDIAN),    //2
    REG_READ_ALIGN_U16( db_cfg_info.device.device[4],               REG_ORDER_BIG_EDIAN),    //3
    REG_READ_ALIGN_U16( db_cfg_info.device.device[6],               REG_ORDER_BIG_EDIAN),    //4
    REG_READ_ALIGN_U16( db_cfg_info.device.device[8],               REG_ORDER_BIG_EDIAN),    //5
    REG_READ_ALIGN_U16( db_cfg_info.device.device[10],              REG_ORDER_BIG_EDIAN),    //6
    REG_READ_ALIGN_U16( db_cfg_info.device.device[12],              REG_ORDER_BIG_EDIAN),    //7
    REG_READ_ALIGN_U16( db_cfg_info.device.device[14],              REG_ORDER_BIG_EDIAN),    //8
    REG_READ_ALIGN_U16( db_cfg_info.device.encrypt,                 REG_ORDER_LITTLE_EDIAN),    //9
    REG_READ_ALIGN_U32_CD( db_cfg_info.device.pt,                   REG_ORDER_LITTLE_EDIAN),    //10
    REG_READ_ALIGN_U32_AB( db_cfg_info.device.pt,                   REG_ORDER_LITTLE_EDIAN),    //11
    REG_READ_ALIGN_U32_CD( db_cfg_info.device.ct,                   REG_ORDER_LITTLE_EDIAN),    //12
    REG_READ_ALIGN_U32_AB( db_cfg_info.device.ct,                   REG_ORDER_LITTLE_EDIAN),    //13
    REG_READ_ALIGN_U16( db_cfg_info.device.ProtocolVer,             REG_ORDER_LITTLE_EDIAN),    //14
    REG_READ_ALIGN_U16( db_cfg_info.device.SoftVersion,             REG_ORDER_LITTLE_EDIAN),    //15
    REG_READ_ALIGN_U16( db_cfg_info.device.TestVersion,             REG_ORDER_LITTLE_EDIAN),    //16
    REG_READ_ALIGN_U16( db_cfg_info.device.HardVersion,             REG_ORDER_LITTLE_EDIAN),    //17
    REG_READ_ALIGN_U16( db_cfg_info.device.CustomInfo[0],           REG_ORDER_LITTLE_EDIAN),    //18
    REG_READ_ALIGN_U16( db_cfg_info.device.CustomInfo[2],           REG_ORDER_LITTLE_EDIAN),    //19
    REG_READ_ALIGN_U16( db_cfg_info.device.CustomInfo[4],           REG_ORDER_LITTLE_EDIAN),    //20
    REG_READ_ALIGN_U16( db_cfg_info.device.CustomInfo[6],           REG_ORDER_LITTLE_EDIAN),    //21
    REG_READ_ALIGN_U16( db_cfg_info.device.CustomInfo[8],           REG_ORDER_LITTLE_EDIAN),    //22
    REG_READ_ALIGN_U16( db_cfg_info.device.CustomInfo[10],          REG_ORDER_LITTLE_EDIAN),    //23
    REG_READ_ALIGN_U16( db_cfg_info.device.CustomInfo[12],          REG_ORDER_LITTLE_EDIAN),    //24
    REG_READ_ALIGN_U16( db_cfg_info.device.CustomInfo[14],          REG_ORDER_LITTLE_EDIAN),    //25
    REG_READ_ALIGN_U16( db_cfg_info.device.CustomInfo[16],          REG_ORDER_LITTLE_EDIAN),    //26
    REG_READ_ALIGN_U16( db_cfg_info.device.CustomInfo[18],          REG_ORDER_LITTLE_EDIAN),    //27
    REG_READ_ALIGN_U16( db_cfg_info.device.CustomInfo[20],          REG_ORDER_LITTLE_EDIAN),    //28
    REG_READ_ALIGN_U16( db_cfg_info.device.CustomInfo[22],          REG_ORDER_LITTLE_EDIAN),    //29   
    REG_READ_ALIGN_U16( db_cfg_info.device.SNInfo[0 ],                 REG_ORDER_BIG_EDIAN),    //30
    REG_READ_ALIGN_U16( db_cfg_info.device.SNInfo[2 ],                 REG_ORDER_BIG_EDIAN),    //31
    REG_READ_ALIGN_U16( db_cfg_info.device.SNInfo[4 ],                 REG_ORDER_BIG_EDIAN),    //32
    REG_READ_ALIGN_U16( db_cfg_info.device.SNInfo[6 ],                 REG_ORDER_BIG_EDIAN),    //33
    REG_READ_ALIGN_U16( db_cfg_info.device.SNInfo[8 ],                 REG_ORDER_BIG_EDIAN),    //34
    REG_READ_ALIGN_U16( db_cfg_info.device.SNInfo[10],                 REG_ORDER_BIG_EDIAN),    //35
    REG_READ_ALIGN_U16( db_cfg_info.device.SNInfo[12],                 REG_ORDER_BIG_EDIAN),    //36   
    REG_READ_ALIGN_U16( db_cfg_info.device.SNInfo[14],                 REG_ORDER_BIG_EDIAN),    //37
    REG_READ_ALIGN_U16( db_cfg_info.device.SNInfo[16],                 REG_ORDER_BIG_EDIAN),    //38
    REG_READ_ALIGN_U16( db_cfg_info.device.SNInfo[18],                 REG_ORDER_BIG_EDIAN),    //39
    REG_READ_ALIGN_U16( db_cfg_info.device.SNInfo[20],                 REG_ORDER_BIG_EDIAN),    //40
    REG_READ_ALIGN_U16( db_cfg_info.device.SNInfo[22],                 REG_ORDER_BIG_EDIAN),    //41
    REG_READ_ALIGN_U16( db_cfg_info.device.SNInfo[24],                 REG_ORDER_BIG_EDIAN),    //42
    REG_READ_ALIGN_U16( db_cfg_info.version.CustomInfo[30],            REG_ORDER_BIG_EDIAN),    //43    
    REG_READ_ALIGN_U16( db_cfg_info.version.CustomInfo[32],            REG_ORDER_BIG_EDIAN),    //44
    REG_READ_ALIGN_U16( db_cfg_info.version.CustomInfo[34],            REG_ORDER_BIG_EDIAN),    //45
    REG_READ_ALIGN_U16( db_cfg_info.version.CustomInfo[36],            REG_ORDER_BIG_EDIAN),    //46
    REG_READ_ALIGN_U16( db_cfg_info.version.CustomInfo[38],            REG_ORDER_BIG_EDIAN),    //47
    REG_READ_ALIGN_U16( db_cfg_info.version.CustomInfo[40],            REG_ORDER_BIG_EDIAN),    //48
    REG_READ_ALIGN_U16( db_cfg_info.version.CustomInfo[42],            REG_ORDER_BIG_EDIAN),    //49
    REG_READ_ALIGN_U16( db_cfg_info.version.CustomInfo[44],            REG_ORDER_BIG_EDIAN),    //50
    REG_READ_ALIGN_U16( db_cfg_info.version.CustomInfo[46],            REG_ORDER_BIG_EDIAN),    //51
    REG_READ_ALIGN_U16( db_cfg_info.version.CustomInfo[48],            REG_ORDER_BIG_EDIAN),    //52
    REG_READ_ALIGN_U16( db_cfg_info.version.CustomInfo[50],            REG_ORDER_BIG_EDIAN),    //53
    REG_READ_ALIGN_U16( db_cfg_info.version.CustomInfo[52],            REG_ORDER_BIG_EDIAN),    //54
    REG_READ_ALIGN_U16( db_cfg_info.version.CustomInfo[54],            REG_ORDER_BIG_EDIAN),    //55
    REG_READ_ALIGN_U16( db_cfg_info.version.CustomInfo[56],            REG_ORDER_BIG_EDIAN),    //56
    REG_READ_ALIGN_U16( db_cfg_info.version.CustomInfo[58],            REG_ORDER_BIG_EDIAN),    //57
    REG_READ_ALIGN_U16( db_cfg_info.version.CustomInfo[60],            REG_ORDER_BIG_EDIAN),    //58
    REG_READ_ALIGN_U16( db_cfg_info.version.CustomInfo[62],            REG_ORDER_BIG_EDIAN),    //59
    REG_READ_ALIGN_U16( db_cfg_info.version.CustomInfo[64],            REG_ORDER_BIG_EDIAN),    //60
    REG_READ_ALIGN_U16( db_cfg_info.version.CustomInfo[66],            REG_ORDER_BIG_EDIAN),    //61
    REG_READ_ALIGN_U16( db_cfg_info.version.CustomInfo[68],            REG_ORDER_BIG_EDIAN),    //62
    REG_READ_ALIGN_U16( db_cfg_info.version.CustomInfo[70],            REG_ORDER_BIG_EDIAN),    //63
    REG_READ_ALIGN_U16( db_cfg_info.version.CustomInfo[72],            REG_ORDER_BIG_EDIAN),    //64
    REG_READ_ALIGN_U16( db_cfg_info.version.CustomInfo[74],            REG_ORDER_BIG_EDIAN),    //65
    REG_READ_ALIGN_U16( db_cfg_info.version.CustomInfo[76],            REG_ORDER_BIG_EDIAN),    //66
    REG_READ_ALIGN_U16( db_cfg_info.version.CustomInfo[78],            REG_ORDER_BIG_EDIAN),    //67
    REG_READ_ALIGN_U16( db_cfg_info.version.CustomInfo[80],            REG_ORDER_BIG_EDIAN),    //68
    REG_READ_ALIGN_U16( db_cfg_info.version.CustomInfo[82],            REG_ORDER_BIG_EDIAN),    //69
    REG_READ_ALIGN_U16( db_cfg_info.version.CustomInfo[84],            REG_ORDER_BIG_EDIAN),    //70
    REG_READ_ALIGN_U16( db_cfg_info.version.CustomInfo[86],            REG_ORDER_BIG_EDIAN),    //71
    REG_READ_ALIGN_U16( db_cfg_info.version.CustomInfo[88],            REG_ORDER_BIG_EDIAN),    //72
    REG_READ_ALIGN_U16( db_cfg_info.version.CustomInfo[90],            REG_ORDER_BIG_EDIAN),    //73
    REG_READ_ALIGN_U16( db_cfg_info.version.CustomInfo[92],            REG_ORDER_BIG_EDIAN),    //74
    REG_READ_ALIGN_U16( db_cfg_info.version.CustomInfo[94],            REG_ORDER_BIG_EDIAN),    //75
    REG_READ_ALIGN_U16( db_cfg_info.version.CustomInfo[96],            REG_ORDER_BIG_EDIAN),    //76
    REG_READ_ALIGN_U16( db_cfg_info.version.CustomInfo[98],            REG_ORDER_BIG_EDIAN),    //77
    REG_READ_ALIGN_U16( db_cfg_info.version.CustomInfo[100],           REG_ORDER_BIG_EDIAN),    //78
    REG_READ_ALIGN_U16( db_cfg_info.version.CustomInfo[102],           REG_ORDER_BIG_EDIAN),    //79
    REG_READ_ALIGN_U16( db_cfg_info.version.CustomInfo[104],           REG_ORDER_BIG_EDIAN),    //80
    REG_READ_ALIGN_U16( db_cfg_info.version.CustomInfo[106],           REG_ORDER_BIG_EDIAN),    //81
    REG_READ_ALIGN_U16( db_cfg_info.version.CustomInfo[108],           REG_ORDER_BIG_EDIAN),    //82
    REG_READ_ALIGN_U16( db_cfg_info.version.CustomInfo[110],           REG_ORDER_BIG_EDIAN),    //83
    REG_READ_ALIGN_U16( db_cfg_info.version.CustomInfo[112],           REG_ORDER_BIG_EDIAN),    //84
    REG_READ_ALIGN_U16( db_cfg_info.version.CustomInfo[114],           REG_ORDER_BIG_EDIAN),    //85
    REG_READ_ALIGN_U16( db_cfg_info.version.CustomInfo[116],           REG_ORDER_BIG_EDIAN),    //86
    REG_READ_ALIGN_U16( db_cfg_info.version.CustomInfo[118],           REG_ORDER_BIG_EDIAN),    //87
    REG_READ_ALIGN_U16( db_cfg_info.version.CustomInfo[120],           REG_ORDER_BIG_EDIAN),    //88
};



/* status*/
struct remap_Modbus_node remapStatusInfo[] = {
    REG_READ_ALIGN_U16( db_cfg_info.status.SimIsInsert,             REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U16( db_cfg_info.status.SimIsReg,                REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U16( db_cfg_info.status.SimCsq,                  REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U16( db_cfg_info.status.SimRsrp,                 REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U16( db_cfg_info.status.SimIccid[0],             REG_ORDER_BIG_EDIAN),
    REG_READ_ALIGN_U16( db_cfg_info.status.SimIccid[2],             REG_ORDER_BIG_EDIAN),
    REG_READ_ALIGN_U16( db_cfg_info.status.SimIccid[4],             REG_ORDER_BIG_EDIAN),
    REG_READ_ALIGN_U16( db_cfg_info.status.SimIccid[6],             REG_ORDER_BIG_EDIAN),
    REG_READ_ALIGN_U16( db_cfg_info.status.SimIccid[8],             REG_ORDER_BIG_EDIAN),
    REG_READ_ALIGN_U16( db_cfg_info.status.SimIccid[10],            REG_ORDER_BIG_EDIAN),
    REG_READ_ALIGN_U16( db_cfg_info.status.SimIccid[12],            REG_ORDER_BIG_EDIAN),
    REG_READ_ALIGN_U16( db_cfg_info.status.SimIccid[14],            REG_ORDER_BIG_EDIAN),
    REG_READ_ALIGN_U16( db_cfg_info.status.SimIccid[16],            REG_ORDER_BIG_EDIAN),
    REG_READ_ALIGN_U16( db_cfg_info.status.SimIccid[18],            REG_ORDER_BIG_EDIAN),
    REG_READ_ALIGN_U16( db_cfg_info.status.SimIccid[20],            REG_ORDER_BIG_EDIAN),
    REG_READ_ALIGN_U16( db_cfg_info.status.SimIccid[22],            REG_ORDER_BIG_EDIAN),
    REG_READ_ALIGN_U16( db_cfg_info.status.ModuleImei[0],           REG_ORDER_BIG_EDIAN),
    REG_READ_ALIGN_U16( db_cfg_info.status.ModuleImei[2],           REG_ORDER_BIG_EDIAN),
    REG_READ_ALIGN_U16( db_cfg_info.status.ModuleImei[4],           REG_ORDER_BIG_EDIAN),
    REG_READ_ALIGN_U16( db_cfg_info.status.ModuleImei[6],           REG_ORDER_BIG_EDIAN),
    REG_READ_ALIGN_U16( db_cfg_info.status.ModuleImei[8],           REG_ORDER_BIG_EDIAN),
    REG_READ_ALIGN_U16( db_cfg_info.status.ModuleImei[10],          REG_ORDER_BIG_EDIAN),
    REG_READ_ALIGN_U16( db_cfg_info.status.ModuleImei[12],          REG_ORDER_BIG_EDIAN),
    REG_READ_ALIGN_U16( db_cfg_info.status.ModuleImei[14],          REG_ORDER_BIG_EDIAN),
    REG_READ_ALIGN_U32_CD( db_cfg_info.status.MemFree,                 REG_ORDER_LITTLE_EDIAN),    //24~25, 内存剩余(kB)
    REG_READ_ALIGN_U32_AB( db_cfg_info.status.MemFree,                 REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U32_CD( db_cfg_info.status.DiskFree,                REG_ORDER_LITTLE_EDIAN),    //26~27, 磁盘剩余(kB)
    REG_READ_ALIGN_U32_AB( db_cfg_info.status.DiskFree,                REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U16( db_cfg_info.status.wifi_ready,              REG_ORDER_LITTLE_EDIAN),    //28, WiFi就绪
    REG_READ_ALIGN_U16( db_cfg_info.status.rn7302_status,           REG_ORDER_LITTLE_EDIAN),    //29, RN7302计量芯片状态，0=正常，1=异常
    REG_READ_ALIGN_U16( db_cfg_info.status.rtc_volt,                REG_ORDER_LITTLE_EDIAN),    //30, RTC电池电压，单位0.01V
};

/* product_test */
struct remap_Modbus_node remapTestInfo[] = {
    REG_READ_ALIGN_U16( db_cfg_info.product_test.key[0],            REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U16( db_cfg_info.product_test.key[1],            REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U16( db_cfg_info.product_test.poweroff,          REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U16( db_cfg_info.product_test.rtc,               REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U16( db_cfg_info.product_test.s1,                REG_ORDER_LITTLE_EDIAN),
    REG_READ_ALIGN_U16( db_cfg_info.product_test.testing,           REG_ORDER_LITTLE_EDIAN),
};

struct system_info
{
    uint32_t    unix_time;
    uint16_t    timezone;
};
extern struct system_info systeminfo;


struct remap_Modbus_node remapSystemInfo[] = {
    REG_READ_WITH_ADDR_ALIGN_U32_CD(systeminfo.unix_time,                REG_ORDER_LITTLE_EDIAN),  
    REG_READ_WITH_ADDR_ALIGN_U32_AB(systeminfo.unix_time,                REG_ORDER_LITTLE_EDIAN),   
};

struct remap_Modbus_node remapSystemTimeZone[] = {
    REG_READ_WITH_ADDR_ALIGN_U16(systeminfo.timezone,                    REG_ORDER_LITTLE_EDIAN),  
};

/********************************************************************************/


#endif

