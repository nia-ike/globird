/*
* Copyright (c) 
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


#ifndef _MB_REG_REMAP_H_
#define _MB_REG_REMAP_H_

#include <stdint.h>
#include <stddef.h>


#include <stdio.h>
#include <string.h>
#include "mb_cfg.h"
#include "db_struct.h"



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
#define REG_CFG_REMAP(t, s, m)                  {REG_ALIGN_U16(s.m), REG_CFG_OFFSET_IN_DISK(t, m), REG_LITTLE_EDIAN} 
#else
#define REG_CFG_REMAP(s, m)                     {REG_ALIGN_U16(s.m), REG_CFG_OFFSET_IN_DISK(s, m), REG_LITTLE_EDIAN} 
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
#define REG_LITTLE_EDIAN                    0
#define REG_BIG_EDIAN                       1



    

#endif

