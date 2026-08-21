
/*
* Copyright (c) 2019,�麣�Ƽ��ɷ����޹�˾
* All rights reserved.
* 
* �ļ����ƣ�algorithm_Current.h
* �ļ���ʶ��
* ��    ����
* 
* ��ǰ�汾��V1.0
* ��    �ߣ�raoy
* ����˵����
* ������ڣ�20190425
* ���л�����
����ϵͳ: RT-Thread
Ӳ��ƽ̨��STM32F030
�̼���  ��V3.1.2
* �޸ļ�¼ 
*  ����       ʱ��         �汾      �޸�����
*  raoy   2019.04.25       V1.00      ����
*/


#ifndef _ALGORITHM_FUNDWAVE_H_
#define _ALGORITHM_FUNDWAVE_H_

#include <stdint.h>
#include "db_struct.h"

int32_t rms_algorithm_fundwave(struct db_database_measure_base_type *pMeasure, struct db_basic_realtime_type *out, uint32_t pt, uint32_t ct);

#endif
