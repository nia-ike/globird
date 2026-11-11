/*************************************************
  Copyright (C), 2003-2017, pilot Co., Ltd.
  File name:      Rn7302File.c
  Author:       longzh    Version: 1.0       Date: 2017-6-9
  Description:  Rn7302?????д
  Others:         
  History:              
*************************************************/
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include "Rn7302File.h"
#include "debug.h"

#define _DEBUG DEBUG_OFF


// 驱动同时支持 IIO 的 sysfs 方式读取 以及 misc 的 fops方式读取
// 新的应用修改为 misc的fops方式，提高性能同时，在多个三相回路中的代码合并
static int misc_fd = -1;
static char current_device_path[256] = {0};  // 存储当前使用的设备路径

// 动态查找设备文件的辅助函数
/*************************************************************************/
/** Function Name: find_rn7302_device                                 **/    
/** Purpose:           find the rn7302 device file                      **/    
/** Params:            chip - the chip number                           **/    
/** Return:            0 on success, -1 otherwise                       **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int find_rn7302_device(int chip, char *device_path, size_t path_size)
{
    DIR *dir;
    struct dirent *entry;
    
    // 打开/dev目录
    dir = opendir("/dev");
    if (dir == NULL) {
        printf("Failed to open /dev directory: %s\n", strerror(errno));
        return -1;
    }
    
    // 遍历/dev目录查找rn7302-0.x设备
    while ((entry = readdir(dir)) != NULL) {
        // 检查文件名是否匹配rn7302-0.x格式
        if (strncmp(entry->d_name, "rn7302-0.", 9) == 0) {
            // 提取设备编号
            char *dot_pos = strchr(entry->d_name, '.');
            if (dot_pos != NULL) {
                int device_num = atoi(dot_pos + 1);
                if (device_num == chip) {
                    // 找到匹配的设备
                    snprintf(device_path, path_size, "/dev/%s", entry->d_name);
                    closedir(dir);
                    return 0;
                }
            }
        }
    }
    
    closedir(dir);
    
    // 如果没有找到匹配的设备，尝试默认路径作为容错处理
    snprintf(device_path, path_size, "/dev/rn7302-0.%d", chip);
    printf("Warning: Device /dev/rn7302-0.%d not found in /dev directory, trying default path\n", chip);
    
    return 0;  // 返回0表示尝试使用默认路径
}

// 调试函数：列出所有可用的rn7302设备
/*********************************************************************************************************
** Function name:       list_available_rn7302_devices
** Descriptions:        
** input parameters:             
** output parameters:   NONE
** Returned value:      
*********************************************************************************************************/
static void list_available_rn7302_devices(void)
{
    DIR *dir;
    struct dirent *entry;
    int device_count = 0;
    
    printf("Scanning /dev directory for rn7302 devices...\n");
    
    dir = opendir("/dev");
    if (dir == NULL) {
        printf("Failed to open /dev directory: %s\n", strerror(errno));
        return;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "rn7302-0.", 9) == 0) {
            printf("Found device: /dev/%s\n", entry->d_name);
            device_count++;
        }
    }
    
    closedir(dir);
    
    if (device_count == 0) {
        printf("No rn7302 devices found in /dev directory\n");
    } else {
        printf("Total found %d rn7302 device(s)\n", device_count);
    }
}

/*************************************************************************/
/** Function Name: MeterChipInit                                      **/    
/** Purpose:           initialize the rn7302 device                    **/    
/** Params:            chip - the chip number                           **/    
/** Return:            file descriptor on success, -1 otherwise        **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int MeterChipInit(int chip)
{
    int ret;
    char device_path[32] = {0};  // 局部变量存储动态找到的设备路径
    
    // 检查chip参数的有效性
    if(chip < 0)
    {
        printf("Invalid chip id = %d, it must be >= 0\n", chip);
        return -1;
    }

    list_available_rn7302_devices();  // 列出所有可用设备用于调试
    // 动态查找设备文件
    ret = find_rn7302_device(chip, device_path, sizeof(device_path));
    if(ret < 0)
    {
        printf("Failed to find device for chip %d\n", chip);
        
        return -1;
    }

    // 保存设备路径到全局变量
    strncpy(current_device_path, device_path, sizeof(current_device_path) - 1);
    current_device_path[sizeof(current_device_path) - 1] = '\0';

    // 尝试打开设备文件
    misc_fd = open(device_path, O_RDWR);
    if(misc_fd < 0)
    {
        printf("Failed to open device %s: %s\n", device_path, strerror(errno));
        return -1;
    }
    
    printf("Successfully opened device %s for chip %d\n", device_path, chip);
    return misc_fd;
}


/*************************************************************************/
/** Function Name: MeterChipDestory                                    **/    
/** Purpose:           close the rn7302 device                        **/    
/** Params:            None.                                           **/    
/** Return:            0 on success, -1 otherwise                       **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
void MeterChipDestory(void)
{
    close(misc_fd);
    misc_fd = -1;
}

/*************************************************************************/
/** Function Name: MeterChipReadReg                                    **/    
/** Purpose:           read the register value from the rn7302 device   **/    
/** Params:            reg - the register number                        **/    
/** Return:            0 on success, -1 otherwise                       **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int MeterChipReadReg(uint32_t reg, uint32_t nbyte, int32_t *pval)
{
	int ret;
	
    /**/
    lseek(misc_fd, reg, SEEK_SET);
    ret = read(misc_fd, pval, nbyte);
	if(ret < 0)
		return -1;
	
	return ret;
}


/*************************************************************************/
/** Function Name: MeterChipWriteReg                                  **/    
/** Purpose:           write the register value to the rn7302 device   **/    
/** Params:            reg - the register number                        **/    
/** Return:            0 on success, -1 otherwise                       **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int MeterChipWriteReg(uint32_t reg, uint32_t nbyte, uint32_t val)
{
	int ret;
	
	/**/
    lseek(misc_fd, reg, SEEK_SET);
    ret = write(misc_fd, &val, nbyte);
	if(ret < 0)
		return -1;
	
	return ret;
}




/*************************************************************************/
/** Function name:       MeterChipReadArea                               **/    
/** Purpose:           read the register value from the rn7302 device   **/    
/** Params:            reg - the register number                        **/    
/**                    nbyte - the number of bytes to read                 **/    
/** Output parameters:   pval - the register value                        **/    
/** Returned value:      0 on success, -1 otherwise                       **/    
/*************************************************************************/
int32_t MeterChipReadArea(uint32_t reg, uint32_t nbyte, uint32_t *pval)
{
	int ret;
	int saved_errno;

    /* only support burst read 8 regs, 4bytes every reg */
    if(nbyte != 32) {
        printf("[HW] MeterChipReadArea invalid nbyte=%u reg=0x%X\n", nbyte, reg);
        errno = EINVAL;
        return -1;
    }
    /**/
    lseek(misc_fd, reg, SEEK_SET);
    ret = read(misc_fd, pval, 8); // 驱动为读8个寄存器。这里是适配驱动要求的8，实际上驱动会按32操作。暂时不统一起来
    saved_errno = errno;
	if(ret < 0) {
        printf("[HW] MeterChipReadArea fail reg=0x%X ret=%d errno=%d(%s)\n",
               reg, ret, saved_errno, strerror(saved_errno));
        errno = saved_errno; /* printf 可能改写 errno，恢复驱动原始错误 */
		return -1;
    }
    if (ret != 32) {
        printf("[HW] MeterChipReadArea unexpected ret=%d (expect 32) reg=0x%X errno=%d(%s)\n",
               ret, reg, saved_errno, strerror(saved_errno));
        errno = (saved_errno != 0) ? saved_errno : EIO;
        return -1;
    }

	return ret;
}

/*************************************************************************/
/** Function Name: MeterChipGetDevicePath                                **/    
/** Purpose:           get the current device path                       **/    
/** Params:            None.                                           **/    
/** Return:            device path string or NULL if not initialized    **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
const char* MeterChipGetDevicePath(void)
{
    if (misc_fd < 0) {
        return NULL;  // 设备未初始化
    }
    return current_device_path;
}

/*************************************************************************/
/** Function Name: MeterChipGetFd                                        **/    
/** Purpose:           get the current device file descriptor              **/    
/** Params:            None.                                           **/    
/** Return:            device file descriptor or -1 if not initialized   **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int MeterChipGetFd(void)
{
    return misc_fd;  // 返回当前的文件描述符
}


