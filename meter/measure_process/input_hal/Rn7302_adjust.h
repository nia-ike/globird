/*************************************************
  Copyright (C), 2003-2017, pilot Co., Ltd.
  File name:      Rn7302.h
  Author:       longzh    Version: 1.0       Date: 2017-6-9
  Description:  Rn7302 ����
  Others:         
  History:              
*************************************************/
#ifndef __RN7302_ADJUST_H__
#define __RN7302_ADJUST_H__
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



struct db_database_measure_base_type;


int32_t Rn7302Adjust(struct db_database_measure_base_type *pMeasure);
int32_t Rn7302Adjust_Offset_I(struct db_database_measure_base_type *pMeasure);
void Rn7302_AdjustFactor(struct db_database_measure_base_type *pMeasure);
void Rn7302_RST_AdjustFactor(struct db_database_measure_base_type *pMeasure);


#ifdef  __cplusplus
}
#endif

#endif

