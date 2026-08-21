/*
* Copyright (c) 2013,珠海一构物联有限公司
* All rights reserved.
*
* 文件名称：db_cfg.h
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
*  chenzy   2013.11.08   V1.00      创建
*/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <dirent.h>
#include <sqlite3.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>
#include <sys/stat.h>  //stat
#include <sys/types.h> 

#include "dc_lz4.h"

/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
int dc_lz4_compress(uint8_t *psrc, uint32_t src_size, uint8_t *dst, uint32_t dst_size)
{
    uint32_t dstCapacity, compress_size;

    // LZ4 provides a function that will tell you the maximum size of compressed output based on input data via LZ4_compressBound().
    dstCapacity = LZ4_compressBound(src_size);
    if(dst_size < dstCapacity)
        return -1;

    // That's all the information and preparation LZ4 needs to compress *src into* compressed_data.
    // Invoke LZ4_compress_default now with our size values and pointers to our memory locations.
    // Save the return value for error checking.
    compress_size = LZ4_compress_default((const char *)psrc, (char *)dst, src_size, dstCapacity);
    
    return compress_size;
}



/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
int dc_lz4_decompress(uint8_t *psrc, uint32_t src_size, uint8_t *dst, uint32_t dst_size)
{
    int32_t decompressed_size;

    // The LZ4_decompress_safe function needs to know where the compressed data is, how many bytes long it is,
    // where the regen_buffer memory location is, and how large regen_buffer (uncompressed) output will be.
    // Again, save the return_value.
    decompressed_size = LZ4_decompress_safe((const char *)psrc, (char *)dst, src_size, dst_size);
    
    return decompressed_size;
}