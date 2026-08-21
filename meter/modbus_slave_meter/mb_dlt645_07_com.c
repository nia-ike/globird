#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "crc16.h"
#include "mb_dlt645_07_com.h"
#include "db_struct.h"
#include "mb_com_baseclass.h"


#define DLT645_SET_HIGH_BIT4    0x80
#define DLT645_NEGATIVE         0xa0

static int dlt645_energy_flip_handle_func(struct st_data_encode_table *ptx, uint16_t value, uint32_t *data);
static int dlt645_combined_energy_flip_handle_func(struct st_data_encode_table *ptx, uint16_t value, uint32_t *data);

ST_DATA_ENCODE_TABLE realtime_V[]=
{
    {0x0100, DLT645_RO, DLT645_U32, DLT645_CONST_2_BYTE, 1, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.numeric.fullwave.secondary.Va), 0},/*A相电压*/
    {0x0200, DLT645_RO, DLT645_U32, DLT645_CONST_2_BYTE, 1, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.numeric.fullwave.secondary.Vb), 0},/*B相电压*/
    {0x0300, DLT645_RO, DLT645_U32, DLT645_CONST_2_BYTE, 1, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.numeric.fullwave.secondary.Vc), 0},/*C相电压*/
};

ST_DATA_ENCODE_TABLE realtime_I[]=
{
    {0x0100, DLT645_RO, DLT645_U16, DLT645_CONST_3_BYTE, 3, 4, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.numeric.fullwave.secondary.Ia), 0},/*A相电流*/
    {0x0200, DLT645_RO, DLT645_U16, DLT645_CONST_3_BYTE, 3, 4, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.numeric.fullwave.secondary.Ib), 0},/*B相电流*/
    {0x0300, DLT645_RO, DLT645_U16, DLT645_CONST_3_BYTE, 3, 4, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.numeric.fullwave.secondary.Ic), 0},/*C相电流*/
};
ST_DATA_ENCODE_TABLE realtime_P[]=
{
    {0x0000, DLT645_RO, DLT645_S32, DLT645_CONST_3_BYTE, 4, 4, 2, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.numeric.fullwave.secondary.Ptot), 0},/*总有功功率*/
    {0x0100, DLT645_RO, DLT645_S32, DLT645_CONST_3_BYTE, 4, 4, 2, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.numeric.fullwave.secondary.Pa  ), 0},/*A相有功功率*/
    {0x0200, DLT645_RO, DLT645_S32, DLT645_CONST_3_BYTE, 4, 4, 2, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.numeric.fullwave.secondary.Pb  ), 0},/*B相有功功率*/
    {0x0300, DLT645_RO, DLT645_S32, DLT645_CONST_3_BYTE, 4, 4, 2, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.numeric.fullwave.secondary.Pc  ), 0},/*C相有功功率*/
};
ST_DATA_ENCODE_TABLE realtime_Q[]=
{
    {0x0000, DLT645_RO, DLT645_S32, DLT645_CONST_3_BYTE, 4, 4, 2, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.numeric.fullwave.secondary.Qtot), 0},/*总无功功率*/
    {0x0100, DLT645_RO, DLT645_S32, DLT645_CONST_3_BYTE, 4, 4, 2, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.numeric.fullwave.secondary.Qa  ), 0},/*A相无功功率*/
    {0x0200, DLT645_RO, DLT645_S32, DLT645_CONST_3_BYTE, 4, 4, 2, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.numeric.fullwave.secondary.Qb  ), 0},/*B相无功功率*/
    {0x0300, DLT645_RO, DLT645_S32, DLT645_CONST_3_BYTE, 4, 4, 2, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.numeric.fullwave.secondary.Qc  ), 0},/*C相功功功率*/
};
ST_DATA_ENCODE_TABLE realtime_S[]=
{
    {0x0000, DLT645_RO, DLT645_U32, DLT645_CONST_3_BYTE, 4, 4, 2, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.numeric.fullwave.secondary.Stot), 0},/*总视在功率*/
    {0x0100, DLT645_RO, DLT645_U32, DLT645_CONST_3_BYTE, 4, 4, 2, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.numeric.fullwave.secondary.Sa  ), 0},/*A相视在功率*/
    {0x0200, DLT645_RO, DLT645_U32, DLT645_CONST_3_BYTE, 4, 4, 2, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.numeric.fullwave.secondary.Sb  ), 0},/*B相视在功率*/
    {0x0300, DLT645_RO, DLT645_U32, DLT645_CONST_3_BYTE, 4, 4, 2, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.numeric.fullwave.secondary.Sc  ), 0},/*C相视在功率*/
};
ST_DATA_ENCODE_TABLE realtime_PF[]=
{
    {0x0000, DLT645_RO, DLT645_S16, DLT645_CONST_2_BYTE, 3, 3, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.numeric.fullwave.primary.PFtot), 0},/*总功率因素*/
    {0x0100, DLT645_RO, DLT645_S16, DLT645_CONST_2_BYTE, 3, 3, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.numeric.fullwave.primary.PFa  ), 0},/*A相功率因素*/
    {0x0200, DLT645_RO, DLT645_S16, DLT645_CONST_2_BYTE, 3, 3, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.numeric.fullwave.primary.PFb  ), 0},/*B相功率因素*/
    {0x0300, DLT645_RO, DLT645_S16, DLT645_CONST_2_BYTE, 3, 3, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.numeric.fullwave.primary.PFc  ), 0},/*C相功率因素*/
};

//标准的DLT645-2007角度是电压与电流之间的夹角
ST_DATA_ENCODE_TABLE realtime_angle_V[]=		
{
	{0x0100, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 1, 2, 2, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.numeric.fullwave.primary.J_Va), 0},/*A相电压角度*/
	{0x0200, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 1, 2, 2, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.numeric.fullwave.primary.J_Vb), 0},/*B相电压角度*/
	{0x0300, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 1, 2, 2, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.numeric.fullwave.primary.J_Vc), 0},/*C相电压角度*/
};

ST_DATA_ENCODE_TABLE realtime_harmonic_Va[]=
{
	{0x0101, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVa[1 ]), 0},/*A 相电压 1  次谐波含量*/
	{0x0102, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVa[2 ]), 0},/*A 相电压 2  次谐波含量*/
	{0x0103, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVa[3 ]), 0},/*A 相电压 3  次谐波含量*/
	{0x0104, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVa[4 ]), 0},/*A 相电压 4  次谐波含量*/
	{0x0105, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVa[5 ]), 0},/*A 相电压 5  次谐波含量*/
	{0x0106, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVa[6 ]), 0},/*A 相电压 6  次谐波含量*/
	{0x0107, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVa[7 ]), 0},/*A 相电压 7  次谐波含量*/
	{0x0108, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVa[8 ]), 0},/*A 相电压 8  次谐波含量*/
	{0x0109, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVa[9 ]), 0},/*A 相电压 9  次谐波含量*/
	{0x010A, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVa[10]), 0},/*A 相电压 10 次谐波含量*/
	{0x010B, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVa[11]), 0},/*A 相电压 11 次谐波含量*/
	{0x010C, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVa[12]), 0},/*A 相电压 12 次谐波含量*/
	{0x010D, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVa[13]), 0},/*A 相电压 13 次谐波含量*/
	{0x010E, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVa[14]), 0},/*A 相电压 14 次谐波含量*/
	{0x010F, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVa[15]), 0},/*A 相电压 15 次谐波含量*/
	{0x0110, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVa[16]), 0},/*A 相电压 16 次谐波含量*/
	{0x0111, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVa[17]), 0},/*A 相电压 17 次谐波含量*/
	{0x0112, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVa[18]), 0},/*A 相电压 18 次谐波含量*/
	{0x0113, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVa[19]), 0},/*A 相电压 19 次谐波含量*/
	{0x0114, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVa[20]), 0},/*A 相电压 20 次谐波含量*/
	{0x0115, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVa[21]), 0},/*A 相电压 21 次谐波含量*/
};
ST_DATA_ENCODE_TABLE realtime_harmonic_Vb[]=
{
	{0x0201, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVb[1 ]), 0},/*B 相电压 1  次谐波含量*/
	{0x0202, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVb[2 ]), 0},/*B 相电压 2  次谐波含量*/
	{0x0203, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVb[3 ]), 0},/*B 相电压 3  次谐波含量*/
	{0x0204, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVb[4 ]), 0},/*B 相电压 4  次谐波含量*/
	{0x0205, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVb[5 ]), 0},/*B 相电压 5  次谐波含量*/
	{0x0206, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVb[6 ]), 0},/*B 相电压 6  次谐波含量*/
	{0x0207, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVb[7 ]), 0},/*B 相电压 7  次谐波含量*/
	{0x0208, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVb[8 ]), 0},/*B 相电压 8  次谐波含量*/
	{0x0209, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVb[9 ]), 0},/*B 相电压 9  次谐波含量*/
	{0x020A, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVb[10]), 0},/*B 相电压 10 次谐波含量*/
	{0x020B, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVb[11]), 0},/*B 相电压 11 次谐波含量*/
	{0x020C, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVb[12]), 0},/*B 相电压 12 次谐波含量*/
	{0x020D, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVb[13]), 0},/*B 相电压 13 次谐波含量*/
	{0x020E, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVb[14]), 0},/*B 相电压 14 次谐波含量*/
	{0x020F, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVb[15]), 0},/*B 相电压 15 次谐波含量*/
	{0x0210, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVb[16]), 0},/*B 相电压 16 次谐波含量*/
	{0x0211, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVb[17]), 0},/*B 相电压 17 次谐波含量*/
	{0x0212, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVb[18]), 0},/*B 相电压 18 次谐波含量*/
	{0x0213, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVb[19]), 0},/*B 相电压 19 次谐波含量*/
	{0x0214, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVb[20]), 0},/*B 相电压 20 次谐波含量*/
	{0x0215, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVb[21]), 0},/*B 相电压 21 次谐波含量*/
};
ST_DATA_ENCODE_TABLE realtime_harmonic_Vc[]=
{
	{0x0301, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVc[1 ]),   0},/*C 相电压 1  次谐波含量*/
	{0x0302, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVc[2 ]), 0},/*C 相电压 2  次谐波含量*/
	{0x0303, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVc[3 ]), 0},/*C 相电压 3  次谐波含量*/
	{0x0304, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVc[4 ]), 0},/*C 相电压 4  次谐波含量*/
	{0x0305, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVc[5 ]), 0},/*C 相电压 5  次谐波含量*/
	{0x0306, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVc[6 ]), 0},/*C 相电压 6  次谐波含量*/
	{0x0307, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVc[7 ]), 0},/*C 相电压 7  次谐波含量*/
	{0x0308, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVc[8 ]), 0},/*C 相电压 8  次谐波含量*/
	{0x0309, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVc[9 ]), 0},/*C 相电压 9  次谐波含量*/
	{0x030A, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVc[10]), 0},/*C 相电压 10 次谐波含量*/
	{0x030B, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVc[11]), 0},/*C 相电压 11 次谐波含量*/
	{0x030C, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVc[12]), 0},/*C 相电压 12 次谐波含量*/
	{0x030D, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVc[13]), 0},/*C 相电压 13 次谐波含量*/
	{0x030E, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVc[14]), 0},/*C 相电压 14 次谐波含量*/
	{0x030F, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVc[15]), 0},/*C 相电压 15 次谐波含量*/
	{0x0310, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVc[16]), 0},/*C 相电压 16 次谐波含量*/
	{0x0311, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVc[17]), 0},/*C 相电压 17 次谐波含量*/
	{0x0312, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVc[18]), 0},/*C 相电压 18 次谐波含量*/
	{0x0313, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVc[19]), 0},/*C 相电压 19 次谐波含量*/
	{0x0314, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVc[20]), 0},/*C 相电压 20 次谐波含量*/
	{0x0315, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRVc[21]), 0},/*C 相电压 21 次谐波含量*/
}; 
ST_DATA_ENCODE_TABLE realtime_harmonic_Ia[]=
{
	{0x0101, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIa[1 ]),   0},/*A 相电流 1  次谐波含量*/
	{0x0102, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIa[2 ]), 0},/*A 相电流 2  次谐波含量*/
	{0x0103, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIa[3 ]), 0},/*A 相电流 3  次谐波含量*/
	{0x0104, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIa[4 ]), 0},/*A 相电流 4  次谐波含量*/
	{0x0105, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIa[5 ]), 0},/*A 相电流 5  次谐波含量*/
	{0x0106, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIa[6 ]), 0},/*A 相电流 6  次谐波含量*/
	{0x0107, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIa[7 ]), 0},/*A 相电流 7  次谐波含量*/
	{0x0108, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIa[8 ]), 0},/*A 相电流 8  次谐波含量*/
	{0x0109, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIa[9 ]), 0},/*A 相电流 9  次谐波含量*/
	{0x010A, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIa[10]), 0},/*A 相电流 10 次谐波含量*/
	{0x010B, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIa[11]), 0},/*A 相电流 11 次谐波含量*/
	{0x010C, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIa[12]), 0},/*A 相电流 12 次谐波含量*/
	{0x010D, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIa[13]), 0},/*A 相电流 13 次谐波含量*/
	{0x010E, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIa[14]), 0},/*A 相电流 14 次谐波含量*/
	{0x010F, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIa[15]), 0},/*A 相电流 15 次谐波含量*/
	{0x0110, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIa[16]), 0},/*A 相电流 16 次谐波含量*/
	{0x0111, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIa[17]), 0},/*A 相电流 17 次谐波含量*/
	{0x0112, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIa[18]), 0},/*A 相电流 18 次谐波含量*/
	{0x0113, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIa[19]), 0},/*A 相电流 19 次谐波含量*/
	{0x0114, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIa[20]), 0},/*A 相电流 20 次谐波含量*/
	{0x0115, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIa[21]), 0},/*A 相电流 21 次谐波含量*/
};
ST_DATA_ENCODE_TABLE realtime_harmonic_Ib[]=
{
	{0x0201, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIb[1 ]), 0},/*B 相电流 1  次谐波含量*/
	{0x0202, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIb[2 ]), 0},/*B 相电流 2  次谐波含量*/
	{0x0203, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIb[3 ]), 0},/*B 相电流 3  次谐波含量*/
	{0x0204, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIb[4 ]), 0},/*B 相电流 4  次谐波含量*/
	{0x0205, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIb[5 ]), 0},/*B 相电流 5  次谐波含量*/
	{0x0206, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIb[6 ]), 0},/*B 相电流 6  次谐波含量*/
	{0x0207, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIb[7 ]), 0},/*B 相电流 7  次谐波含量*/
	{0x0208, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIb[8 ]), 0},/*B 相电流 8  次谐波含量*/
	{0x0209, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIb[9 ]), 0},/*B 相电流 9  次谐波含量*/
	{0x020A, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIb[10]), 0},/*B 相电流 10 次谐波含量*/
	{0x020B, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIb[11]), 0},/*B 相电流 11 次谐波含量*/
	{0x020C, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIb[12]), 0},/*B 相电流 12 次谐波含量*/
	{0x020D, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIb[13]), 0},/*B 相电流 13 次谐波含量*/
	{0x020E, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIb[14]), 0},/*B 相电流 14 次谐波含量*/
	{0x020F, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIb[15]), 0},/*B 相电流 15 次谐波含量*/
	{0x0210, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIb[16]), 0},/*B 相电流 16 次谐波含量*/
	{0x0211, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIb[17]), 0},/*B 相电流 17 次谐波含量*/
	{0x0212, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIb[18]), 0},/*B 相电流 18 次谐波含量*/
	{0x0213, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIb[19]), 0},/*B 相电流 19 次谐波含量*/
	{0x0214, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIb[20]), 0},/*B 相电流 20 次谐波含量*/
	{0x0215, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIb[21]), 0},/*B 相电流 21 次谐波含量*/
};
ST_DATA_ENCODE_TABLE realtime_harmonic_Ic[]=
{
	{0x0301, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIc[1 ]), 0},/*C 相电流 1  次谐波含量*/
	{0x0302, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIc[2 ]), 0},/*C 相电流 2  次谐波含量*/
	{0x0303, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIc[3 ]), 0},/*C 相电流 3  次谐波含量*/
	{0x0304, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIc[4 ]), 0},/*C 相电流 4  次谐波含量*/
	{0x0305, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIc[5 ]), 0},/*C 相电流 5  次谐波含量*/
	{0x0306, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIc[6 ]), 0},/*C 相电流 6  次谐波含量*/
	{0x0307, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIc[7 ]), 0},/*C 相电流 7  次谐波含量*/
	{0x0308, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIc[8 ]), 0},/*C 相电流 8  次谐波含量*/
	{0x0309, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIc[9 ]), 0},/*C 相电流 9  次谐波含量*/
	{0x030A, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIc[10]), 0},/*C 相电流 10 次谐波含量*/
	{0x030B, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIc[11]), 0},/*C 相电流 11 次谐波含量*/
	{0x030C, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIc[12]), 0},/*C 相电流 12 次谐波含量*/
	{0x030D, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIc[13]), 0},/*C 相电流 13 次谐波含量*/
	{0x030E, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIc[14]), 0},/*C 相电流 14 次谐波含量*/
	{0x030F, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIc[15]), 0},/*C 相电流 15 次谐波含量*/
	{0x0310, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIc[16]), 0},/*C 相电流 16 次谐波含量*/
	{0x0311, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIc[17]), 0},/*C 相电流 17 次谐波含量*/
	{0x0312, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIc[18]), 0},/*C 相电流 18 次谐波含量*/
	{0x0313, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIc[19]), 0},/*C 相电流 19 次谐波含量*/
	{0x0314, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIc[20]), 0},/*C 相电流 20 次谐波含量*/
	{0x0315, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.powerquality.harmonic.HR.HRIc[21]), 0},/*C 相电流 21 次谐波含量*/
};
ST_DATA_ENCODE_TABLE realtime_other[]=
{
	{0x0001, DLT645_RO, DLT645_U32, DLT645_CONST_3_BYTE, 3, 3, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.numeric.fullwave.secondary.In), 	0},/*零线电流*/
	{0x0002, DLT645_RO, DLT645_U16, DLT645_CONST_2_BYTE, 2, 2, 1, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.numeric.fullwave.primary.Hz), 		0},/*频率*/
	{0x0004, DLT645_RO, DLT645_S32, DLT645_CONST_3_BYTE, 1, 1, 2, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.statistical.demand.realtime.Ptot), 		0},/*当前有功需量*/
};


ST_DATA_ENCODE_TABLE realtime_energy_KwhTot[]=
{
	{0x0000, DLT645_RO, DLT645_U32, DLT645_CONST_4_BYTE, 2, 1, 2, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.energy.realtime.kWhTot), 0},/*有功总电能*/
};
ST_DATA_ENCODE_TABLE realtime_energy_KwhImp[]=
{
	{0x0000, DLT645_RO, DLT645_U32, DLT645_CONST_4_BYTE, 2, 1, 2, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.energy.realtime.kWhImp[3]), 0},/*正向有功总电能*/
};
ST_DATA_ENCODE_TABLE realtime_energy_KwhExp[]=
{
	{0x0000, DLT645_RO, DLT645_U32, DLT645_CONST_4_BYTE, 2, 1, 2, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.energy.realtime.kWhExp[3]), 0},/*反向有功总电能*/
};

ST_DATA_ENCODE_TABLE realtime_energy_Kvah[]=
{
	{0x0000, DLT645_RO, DLT645_U32, DLT645_CONST_4_BYTE, 2, 1, 2, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.energy.realtime.kVAh[3]), 0},/*正向视在总电能*/
};

ST_DATA_ENCODE_TABLE multirate_energy_KwhImp[]=
{
    {0x0100, DLT645_RO, DLT645_U32, DLT645_CONST_4_BYTE, 2, 1, 2, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.energy.multirate.rate[0].kWhImp), 0},
    {0x0200, DLT645_RO, DLT645_U32, DLT645_CONST_4_BYTE, 2, 1, 2, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.energy.multirate.rate[1].kWhImp), 0},
    {0x0300, DLT645_RO, DLT645_U32, DLT645_CONST_4_BYTE, 2, 1, 2, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.energy.multirate.rate[2].kWhImp), 0},
    {0x0400, DLT645_RO, DLT645_U32, DLT645_CONST_4_BYTE, 2, 1, 2, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.energy.multirate.rate[3].kWhImp), 0},
};

ST_DATA_ENCODE_TABLE multirate_energy_KwhExp[]=
{
    {0x0100, DLT645_RO, DLT645_U32, DLT645_CONST_4_BYTE, 2, 1, 2, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.energy.multirate.rate[0].kWhExp), 0},
    {0x0200, DLT645_RO, DLT645_U32, DLT645_CONST_4_BYTE, 2, 1, 2, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.energy.multirate.rate[1].kWhExp), 0},
    {0x0300, DLT645_RO, DLT645_U32, DLT645_CONST_4_BYTE, 2, 1, 2, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.energy.multirate.rate[2].kWhExp), 0},
    {0x0400, DLT645_RO, DLT645_U32, DLT645_CONST_4_BYTE, 2, 1, 2, REG_OFFSET_OF_VMEM(db_measurebase[0].db_measure.energy.multirate.rate[3].kWhExp), 0},
};

#define MB_SECTION_GET_REG_NUM(x)                sizeof(x)/sizeof(struct st_data_encode_table)

static const ST_REG_ATTR gRegTable[] = {
/*数据标识D3 D2     							存储位置基址                                  		 寄存器个数				write 回调*/
    { 0x02,0x01,(struct st_data_encode_table*)&realtime_V[0],    			MB_SECTION_GET_REG_NUM(realtime_V),        		NULL}, // realtime data
    { 0x02,0x02,(struct st_data_encode_table*)&realtime_I[0],    			MB_SECTION_GET_REG_NUM(realtime_I),        		NULL}, // realtime data
    { 0x02,0x03,(struct st_data_encode_table*)&realtime_P[0],    			MB_SECTION_GET_REG_NUM(realtime_P),        		NULL}, // realtime data
    { 0x02,0x04,(struct st_data_encode_table*)&realtime_Q[0],    			MB_SECTION_GET_REG_NUM(realtime_Q),        		NULL}, // realtime data
    { 0x02,0x05,(struct st_data_encode_table*)&realtime_S[0],    			MB_SECTION_GET_REG_NUM(realtime_S),        		NULL}, // realtime data
    { 0x02,0x06,(struct st_data_encode_table*)&realtime_PF[0],   			MB_SECTION_GET_REG_NUM(realtime_PF),       		NULL}, // realtime data
//    { 0x02,0x07,(struct st_data_encode_table*)&realtime_angle_V[0],    		MB_SECTION_GET_REG_NUM(realtime_angle_V),  		NULL}, // realtime data
    { 0x02,0x0A,(struct st_data_encode_table*)&realtime_harmonic_Va[0], 	MB_SECTION_GET_REG_NUM(realtime_harmonic_Va),   NULL}, // realtime data
    { 0x02,0x0A,(struct st_data_encode_table*)&realtime_harmonic_Vb[0], 	MB_SECTION_GET_REG_NUM(realtime_harmonic_Vb),   NULL}, // realtime data
    { 0x02,0x0A,(struct st_data_encode_table*)&realtime_harmonic_Vc[0], 	MB_SECTION_GET_REG_NUM(realtime_harmonic_Vc),   NULL}, // realtime data
    { 0x02,0x0B,(struct st_data_encode_table*)&realtime_harmonic_Ia[0], 	MB_SECTION_GET_REG_NUM(realtime_harmonic_Ia),   NULL}, // realtime data
    { 0x02,0x0B,(struct st_data_encode_table*)&realtime_harmonic_Ib[0], 	MB_SECTION_GET_REG_NUM(realtime_harmonic_Ib),   NULL}, // realtime data
    { 0x02,0x0B,(struct st_data_encode_table*)&realtime_harmonic_Ic[0], 	MB_SECTION_GET_REG_NUM(realtime_harmonic_Ic),   NULL}, // realtime data
    { 0x02,0x80,(struct st_data_encode_table*)&realtime_other[0],    		MB_SECTION_GET_REG_NUM(realtime_other),        	NULL}, // realtime data
 
    { 0x00,0x00,(struct st_data_encode_table*)&realtime_energy_KwhTot[0], 	MB_SECTION_GET_REG_NUM(realtime_energy_KwhTot), dlt645_combined_energy_flip_handle_func}, // realtime data
    { 0x00,0x01,(struct st_data_encode_table*)&realtime_energy_KwhImp[0], 	MB_SECTION_GET_REG_NUM(realtime_energy_KwhImp), dlt645_energy_flip_handle_func}, // realtime data
    { 0x00,0x02,(struct st_data_encode_table*)&realtime_energy_KwhExp[0], 	MB_SECTION_GET_REG_NUM(realtime_energy_KwhExp), dlt645_energy_flip_handle_func}, // realtime data
    { 0x00,0x09,(struct st_data_encode_table*)&realtime_energy_Kvah[0],   	MB_SECTION_GET_REG_NUM(realtime_energy_Kvah),   dlt645_combined_energy_flip_handle_func}, // realtime data

    { 0x00,0x01,(struct st_data_encode_table*)&multirate_energy_KwhImp[0], 	MB_SECTION_GET_REG_NUM(multirate_energy_KwhImp), dlt645_energy_flip_handle_func}, 
    { 0x00,0x02,(struct st_data_encode_table*)&multirate_energy_KwhExp[0], 	MB_SECTION_GET_REG_NUM(multirate_energy_KwhExp), dlt645_energy_flip_handle_func}, 

};

/*************************************************
    函数名称:       dlt645_combined_energy_flip_handle_func
    功能描述:       把电能按照DLT645最大值进行翻转
    子 函 数:       
    调用限制:       无
    输入参数:       struct st_data_encode_table *ptx, uint16_t value, uint32_t *data       
    输出参数:       
    返 回 值:       转换后数值
    说    明:      
*************************************************/
static int dlt645_combined_energy_flip_handle_func(struct st_data_encode_table *ptx, uint16_t value, uint32_t *data)
{
    #define DLT645_COMBINED_ENERGY_MAX   7999999
    uint32_t tmp = 0;

    tmp = *data;
    tmp %= (DLT645_COMBINED_ENERGY_MAX + 1);

    *data = tmp;

    return 0;
}


/*************************************************
    函数名称:       dlt645_energy_flip_handle_func
    功能描述:       把电能按照DLT645最大值进行翻转
    子 函 数:       
    调用限制:       无
    输入参数:       struct st_data_encode_table *ptx, uint16_t value, uint32_t *data       
    输出参数:       
    返 回 值:       转换后数值
    说    明:      
*************************************************/
static int dlt645_energy_flip_handle_func(struct st_data_encode_table *ptx, uint16_t value, uint32_t *data)
{
    #define DLT645_ENERGY_MAX   9999999
    uint32_t tmp = 0;

    tmp = *data;
    tmp %= (DLT645_ENERGY_MAX + 1);

    *data = tmp;

    return 0;
}


/*************************************************
    函数名称:       ascii2hex
    功能描述:       把ASCII转成数字
    子 函 数:       
    调用限制:       无
    输入参数:       uint8_t ascii       
    输出参数:       
    返 回 值:       转换后数字
    说    明:      
*************************************************/
static uint8_t ascii2hex(uint8_t ascii)
{
    uint8_t hex_value = 0;

    if((ascii >= '0') && (ascii <= '9'))
    {
        hex_value = ascii - '0';
    }
    else if((ascii >= 'A') && (ascii <= 'F'))
    {
        hex_value = ascii - 'A' + 0x0A;
    }
    else if((ascii >= 'a') && (ascii <= 'f'))
    {
        hex_value = ascii - 'a' + 0x0A;
    }
    return hex_value;
}

/*************************************************
    函数名称:       Dlt645_load_data_to_msg
    功能描述:       把数据转化为BCD码后载入响应包
    子 函 数:       
    调用限制:       无
    输入参数:       pNodeData，后半部分数据处理（除了头部ST_DLT645_2007_HEAD）
                    pData,载入数据的起始指针；数据缓存的边界控制由上层软件控制
    输出参数:       
    返 回 值:       返回报文的长度
    说    明:       读数据
*************************************************/
int32_t Dlt645_load_data_to_msg(/*const*/ ST_NodeData *pNodeData,uint8_t *pData)
{
	uint16_t i = 0;
	uint32_t uiNodeVar = 0;
    int32_t s32Var = 0;
    int16_t s16Var = 0;
	uint32_t uiTemp = 0;
    uint8_t Flag = 0;//0表示正,1表示负数
	if((NULL == pNodeData)
		||(NULL ==pData))
	{
		return -1;
	}

	if(DLT645_S32 == pNodeData->RdType)
	{
		s32Var = U8Buf_swap32(pNodeData->pData);  
		if(s32Var < 0)
		{
			uiNodeVar = -s32Var;
			Flag = DLT645_NEGATIVE;
		}
		else 
		{
			uiNodeVar = s32Var;
		}
//		printf("s32Var:%d ,uiNodeVar:%d, %d ,%d ,%d ,%d\n ",s32Var,uiNodeVar,pNodeData->pData[0],pNodeData->pData[1],pNodeData->pData[2],pNodeData->pData[3]);
	}
	else if(DLT645_U32 == pNodeData->RdType)
	{
		uiNodeVar = U8Buf_swap32(pNodeData->pData);	
	}
	else if(DLT645_S16 == pNodeData->RdType)
	{
		s16Var = U8Buf_swap16(pNodeData->pData);	
		if(s16Var < 0)
		{
			uiNodeVar = -s16Var;
			Flag = DLT645_NEGATIVE;
		}
		else 
		{
			uiNodeVar = s16Var;
		}
	}  
	else if(DLT645_U16 == pNodeData->RdType)
	{
		uiNodeVar = U8Buf_swap16(pNodeData->pData);	
	}
//	printf("uiNodeVar:%d, pNodeData->RdSize:%d, pNodeData->RdType:%d \n",uiNodeVar,pNodeData->RdSize,pNodeData->RdType);
//	printf("%s %d\r\n", __FILE__, __LINE__);

	 //需要将数组中的数字改为BCD码
	if(DLT645_P_ARRAY == pNodeData->RdType)
	{	 
		pData = pNodeData->pData;
	}
	else
	{
		for(i = 0; i < pNodeData->RdSize ;i++)
		{
			uiTemp = uiNodeVar%100;
			pData[i] = ((uiTemp/10)<<4) + ((uiTemp%10) & 0x0F);
			uiNodeVar /= 100;
//			printf("%x<->%x ",pNodeData->pData[i],pData[i]);			 
		}
//		printf("\n");
	}
    /*设置最高位为1*/
	if((DLT645_NEGATIVE == Flag)
        &&(0 != pNodeData->RdSize ))
    {
        pData[pNodeData->RdSize - 1] |= DLT645_SET_HIGH_BIT4;
    }

	return 0;
}

/*************************************************
    函数名称:       Dlt645_get_table_data
    功能描述:       查表
    子 函 数:       
    调用限制:       无
    输入参数:       DataId ：数据标识;数据域里的数据要-0x33
                    pResult:从表里读出来的数据值
    输出参数:       trans，返回报文的指针
    返 回 值:       返回报文的长度
    说    明:       读数据
*************************************************/
int32_t Dlt645_get_table_data(uint32_t OperateCode, ST_NodeData *pNodeData, uint8_t *pOutData)
{
	uint16_t i = 0, j = 0, m = 0;
	uint32_t uiDataId = 0;
    uint16_t DataId0302=0, OperateCode0302=0, OperateCode01=0, OperateCode0100=0, OperateCode00=0;
	uint16_t size = 0,num = 0,RdSize=0;
	uint8_t tmp[256]={0};
	uint8_t *pValue;
	uint8_t tmpValue[256]={0};
	uint32_t temp=0,value;
	uint32_t uiNodeVar = 0;
    int32_t s32Var = 0;
    int16_t s16Var = 0;
	uint32_t uiTemp = 0;
	int32_t sRet = 0;
	uint16_t factor=0;
	temp = *pNodeData->pData;

	OperateCode0302 = (OperateCode >> 16) & 0xFFFF;
	OperateCode0100 = OperateCode & 0xFFFF;
	OperateCode01 = (OperateCode >> 8) & 0xFF;
	OperateCode00 = OperateCode  & 0xFF;
    for(i=0; i<sizeof(gRegTable)/sizeof(gRegTable[0]); i++)
	{
		DataId0302 = ((gRegTable[i].DataDI3<<8) + (gRegTable[i].DataDI2 & 0xFFFF));
		if(OperateCode0302 == DataId0302)
		{
//			printf("%s %d\r\n", __FILE__, __LINE__);
//			printf("OperateCode0302:%04X, OperateCode0100:%04X, OperateCode01:%02X \n",OperateCode0302,OperateCode0100,OperateCode01);
			if((OperateCode01 == 0xFF) || (OperateCode00 == 0xFF) )
			{
				for(j=0; j< gRegTable[i].reg_len; j++)
				{
					pNodeData->RdSize=gRegTable[i].p_data[j].RdSize;
					pNodeData->RdType = gRegTable[i].p_data[j].RdType;
					size = gRegTable[i].p_data[j].origin_offset * 2;
					RdSize = gRegTable[i].p_data[j].RdSize;
					num += RdSize;
					memcpy(pNodeData->pData,tmp,pNodeData->RdSize*2);
					switch (gRegTable[i].p_data[j].origin_offset)
					{
					case 1:
                        value = *gRegTable[i].p_data[j].node;
						break;
					case 2:
                        if(NULL == gRegTable[i].handle)
                        {
                            value = *gRegTable[i].p_data[j].node + ((*(gRegTable[i].p_data[j].node + 1)) << 16);
                        }
                        else
                        {
                            value = *gRegTable[i].p_data[j].node + ((*(gRegTable[i].p_data[j].node + 1)) << 16);
                            gRegTable[i].handle(NULL, 0, &value);
                        }
						break;						
					default:
						value = *gRegTable[i].p_data[j].node;
						break;
					}					
					for(m=0; m<size; m++)
					{
						if(gRegTable[i].p_data[j].factor ==  gRegTable[i].p_data[j].Decimal)
						{	
							pNodeData->pData[m] = (value >> m*8) & 0xFF;															
						}
						else if(gRegTable[i].p_data[j].factor > gRegTable[i].p_data[j].Decimal)
						{
							factor = gRegTable[i].p_data[j].factor - gRegTable[i].p_data[j].Decimal;
							pNodeData->pData[m] = ((int32_t)((int32_t)value / pow(10,factor)) >> m*8) & 0xFF;
						}
						else if(gRegTable[i].p_data[j].factor < gRegTable[i].p_data[j].Decimal)
						{
							factor = gRegTable[i].p_data[j].Decimal - gRegTable[i].p_data[j].factor;
							pNodeData->pData[m] = ((int32_t)(value * pow(10,factor)) >> m*8) & 0xFF;
						}
					}
					sRet = Dlt645_load_data_to_msg(pNodeData,pOutData);
					if(0 != sRet)
					{/*不支持的数据记录*/
						return 0;
					}
//					printf("tmpValue \n");
					for(m=0; m<RdSize; m++)
					{
						tmpValue[j*RdSize+m] = pOutData[m];
//						printf("%d<->%d ",tmpValue[m],pOutData[m]);	
					}
//					printf("\n");

				}
				pNodeData->RdType = DLT645_P_ARRAY;
				pNodeData->RdSize = num;
				for(m=0; m<num; m++)
				{
					memcpy(pOutData,tmpValue,num);
//					printf("%02d->%02d ",tmpValue[m],pOutData[m]);	
				}
//				printf("\n");	
						
				return 0;
			}
			else
			{
				for(j=0; j< gRegTable[i].reg_len; j++)
				{
					if(OperateCode0100 == gRegTable[i].p_data[j].DataId)
					{/*找到对应的记录*/
						pNodeData->RdSize=gRegTable[i].p_data[j].RdSize;
						size = gRegTable[i].p_data[j].origin_offset * 2;
//						printf("tmpValue:%d ,%d\n",gRegTable[i].p_data[j].origin_offset,pNodeData->RdSize);
						memcpy(pNodeData->pData,tmp,pNodeData->RdSize*2);
						switch (gRegTable[i].p_data[j].origin_offset)
						{
						case 1:
							value = *gRegTable[i].p_data[j].node;
//							printf("value:%d \n",value);
							break;
						case 2:
                            if(NULL == gRegTable[i].handle)
                            {
                                value = *gRegTable[i].p_data[j].node + ((*(gRegTable[i].p_data[j].node + 1)) << 16);
                            }
                            else
                            {
                                value = *gRegTable[i].p_data[j].node + ((*(gRegTable[i].p_data[j].node + 1)) << 16);
                                gRegTable[i].handle(NULL, 0, &value);
                            }
							break;						
						default:
							value = *gRegTable[i].p_data[j].node;
							break;
						}

						for(m=0; m < size; m++)
						{
							if(gRegTable[i].p_data[j].factor == gRegTable[i].p_data[j].Decimal)
							{	
								pNodeData->pData[m] = (value >> m*8) & 0xFF;
							}
							else if(gRegTable[i].p_data[j].factor > gRegTable[i].p_data[j].Decimal)
							{
								factor = gRegTable[i].p_data[j].factor - gRegTable[i].p_data[j].Decimal;
								pNodeData->pData[m] = ((int32_t)((int32_t)value / pow(10,factor)) >> m*8) & 0xFF;
							}
							else if(gRegTable[i].p_data[j].factor < gRegTable[i].p_data[j].Decimal)
							{
								factor = gRegTable[i].p_data[j].Decimal - gRegTable[i].p_data[j].factor;
								pNodeData->pData[m] = ((int32_t)(value * pow(10,factor)) >> m*8) & 0xFF;
							}
						}
						pNodeData->RdType = gRegTable[i].p_data[j].RdType;
						sRet = Dlt645_load_data_to_msg(pNodeData,pOutData);
						if(0 != sRet)
						{/*不支持的数据记录*/
							return 0;
						}
/*
						size = gRegTable[i].p_data[j].RdSize;
						for(m=0; m<size; m++)
						{
							printf("-->%02d ",pNodeData->pData[m]);	
						}
						printf("\n");
						printf("RdSize:%d, RdType:%d, node:%d \n",gRegTable[i].p_data[j].RdSize,gRegTable[i].p_data[j].RdType,*gRegTable[i].p_data[j].node);
						printf("pNodeData->RdSize:%d, pNodeData->RdType:%d, pNodeData->pData:%d\n",pNodeData->RdSize,pNodeData->RdType,*pNodeData->pData);
*/						
						return 0;
					}						
				}
			}			
		}
	}
	return -1;
}

/*******************************************************************************************************************
.  DLT645包帧标准格式
·　帧格式：| 帧起始符 | 地址域 | 帧起始符 | 控制码 | 数据域长度 | 数据域 | 校验码 | 结束符

·　DLT645读数据的包格式如下：
·　功能：请求读电能表数据
·　控制码：C=11H
·　数据域长度：L=04H+m（数据长度）
·　（m=0）： 数据域：DIODI1DI2DI3（数据标识）          
·　（m=1）： 数据域：DIODI1DI2DI3（数据标识）+ N（负荷记录块数）
·　（m=6）： 数据域：DIODI1DI2DI3（数据标识） + N（负荷记录块数）+分|时|日|月|年
************************************************************************************************************************/

/****************************************************************************************************
    函数名称:       *Dlt645_R_callfunc
    功能描述:       回调函数实现DLT645读数据的功能，用户根据函数里的数据标识DataID，获取对应的数据项，输出到pOutData
	                缓存里；
    输入参数:  输入参数:     
	   <1> DLT645标准说明
		   DataID:
				1）为《DLT645-2007多功能电能表通信协议数据标识编码表》里的数据标识;
				2）即为包格式里的----DIODI1DI2DI3（数据标识）
				3）数据标识能标记唯一的标量(如：DataID:0x00000000;此数据标识表示当前组合有功费率电能
 		
		pInParam: 这取决于InLen的长度；
		         1）InLen=0时没有pInParam=NULL；
		         2）InLen=1时pInParam的数据长度为1；值为N（负荷记录块数)
				 6）InLen=6时pInParam的数据长度为6；pInParam的组织形式为 “N（负荷记录块数）+分|时|日|月|年”
		InLen  :   要刷新的数据的长度;对应写帧格式里：m(数据长度)
		
		pOutData：
		        1）为设备读取出来数据存放缓存区；pOutData的格式参考《DLT645-2007多功能电能表通信协议数据标识编码表》
		        2） 格式一般用BCD编码；
				
		OutLen ：为此缓存的大小；用此判断是否越界;
		
    输出参数:     
    返 回 值:    读出来的数据长度
    说    明:   输出的数据要满足《DLT645-2007多功能电能表通信协议数据标识编码表》要求；采用BCD碥码
*******************************************************************************************************/
uint16_t Dlt645_user_R_deal(uint32_t OperateCode,const void *pInParam,uint16_t InLen,void *pOutParam,uint16_t OutLen)
{
    uint16_t usRetOutLen = 0;
	int32_t sRet = 0;
	ST_Udefine_Data_Form stDataForm;
	ST_NodeData stNodeData ={DLT645_U8,NULL,0};

	uint8_t tmpValue[256];
	stNodeData.pData = &tmpValue[0];

	if(NULL == pOutParam)
	{//pInParam可为NULL
		return 0;
	}	

	sRet = Dlt645_get_table_data(OperateCode,&stNodeData,pOutParam);
//	printf("%s %d\r\n", __FILE__, __LINE__);

	if(0 != sRet)
	{/*不支持的数据记录*/
		return 0;
	}	
//	printf("%s %d\r\n", __FILE__, __LINE__);

	usRetOutLen = stNodeData.RdSize;

	return usRetOutLen;
}

/*************************************************
    函数名称:       Dlt645_register_info
    功能描述:       注册配置表
    子 函 数:       
    调用限制:       无
    输入参数:     
    输出参数:     
    返 回 值:       
    说    明:   
*************************************************/
int32_t Dlt645_register_info(void* pstParameters)
{
    #define NEW_SNINFO_LENGTH   (13)
    #define OLD_SNINFO_LENGTH   (14)

    uint16_t i =0;
	uint16_t SNInfo_len = 0;
    uint16_t usTemp = 0;
    int32_t sRet = 0;

    ST_USER_CFG_INTF stDlt645UserCfg;
    struct db_database_type *pstVmem = (struct db_database_type *)pstParameters;
    struct db_info_device_type *pstInfo = &(pstVmem->db_cfg_info.device);
    struct db_cfg_user_para  *pCfgUser = &(pstVmem->db_measurebase[0].db_cfg.user);

    /* 配置DLT645地址 */
    /* id example TSM2303290001 */
    if(NULL != pstInfo->SNInfo)
    {
        SNInfo_len = strlen(pstInfo->SNInfo) - 1;

        if(NEW_SNINFO_LENGTH == SNInfo_len)
        {
            for(i = 0; i < DLT645_COMM_ADDR_SIZE - 1; i++)	
            {
                stDlt645UserCfg.CommAddr[i] = ascii2hex(pstInfo->SNInfo[SNInfo_len - 2*i - 1]) * 16 + ascii2hex(pstInfo->SNInfo[SNInfo_len - 2*i]);
            }
        }
        else if(OLD_SNINFO_LENGTH == SNInfo_len) //兼容已出厂的SN数字长度为11位的EG0700
        {
            for(i = 0; i < DLT645_COMM_ADDR_SIZE - 1; i++)	
            {
                stDlt645UserCfg.CommAddr[i] = ascii2hex(pstInfo->SNInfo[SNInfo_len - 2*i - 1]) * 16 + ascii2hex(pstInfo->SNInfo[SNInfo_len - 2*i]);
            }
            stDlt645UserCfg.CommAddr[5] = ascii2hex(pstInfo->SNInfo[SNInfo_len - 2*i]);
        }
    }

    /*配置DLT645密码*/
    for(i = 0; i < (DLT645_W_PASSWORD_SIZE/2); i++)	
    {
        usTemp = 0;
        stDlt645UserCfg.WPassword[i*2] = (usTemp & 0xff);
        stDlt645UserCfg.WPassword[i*2 +1] = (uint8_t)((usTemp >> 8) & 0xff);
    }
//	stDlt645UserCfg.WPassword[0] = pCfgUser->base.PassWord.Value;
//	printf("stDlt645UserCfg.WPassword:%d \n",stDlt645UserCfg.WPassword[0]);

    stDlt645UserCfg.pR_Fun = Dlt645_user_R_deal;
    stDlt645UserCfg.pW_Fun = NULL;

    sRet = Dlt645_user_register_intf(&stDlt645UserCfg);
    return sRet ;
}

/*********************************************************************************************************
** Function name:       mb_msg_process_Dlt645_07
** Descriptions:        modbus 消息处理函数
** input parameters:    pRecvBuff 接收到的数据
**                      recvLen   接收到的数据长度
** output parameters:   None
** Returned value:      > 0:正常    其他:异常
*********************************************************************************************************/
int mb_msg_process_Dlt645_07(struct mb_comm_ctx_t *pCtx, uint8_t *pRecvBuff, uint16_t recvLen)
{
	int32_t sRet = 0;
	int i=0;
/*	
	for(i=0; i<pCtx->recvLen; i++)
	{
		printf("%02X ",pCtx->recvBuff[i]);
	}
	printf("\n");
*/
    sRet = Dlt645_analyze_packet_deal(pCtx->recvBuff, pCtx->recvLen, pCtx->sendBuff, pCtx->sendLen);

/*	
	for(i=0; i<sRet; i++)
	{
		printf("%02X ",pCtx->sendBuff[i]);
	}
	printf("\n");
*/
    if (sRet > 0)
    {
        pCtx->pcombase->write(pCtx, &pCtx->sendBuff[0], sRet);
    }
	return sRet;
}


/*********************************************************************************************************
** Function name:       DLT645_config_init
** Descriptions:        DLT645_07协议配置初始化,主要是从EEPROM中加载配置，并判断配置是否正确
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void  DLT645_07_config_init(void *vmem)
{
    int i, j;

    /*
        由于使用动态获取内存作为结构存取，不能在remap结构体中初始化定义 vmem地址，
        因此在此初始化时输入vmem
        注意：
            这里是为了保持结构体类型一致，牺牲点空间和启动时间。运行时可节省时间
            同时保持接头体一致，也是为了框架方便移植到静态定义数据结构的情况  
    */

    for(i=0; i<sizeof(gRegTable)/sizeof(gRegTable[0]); i++)
    {
        for(j=0; j<gRegTable[i].reg_len; j++)
        {
            if(gRegTable[i].p_data)
            {
                gRegTable[i].p_data[j].node = ((uint16_t *)vmem) + gRegTable[i].p_data[j].offset;
            }
        }
    }

	Dlt645_register_info(vmem);
}
