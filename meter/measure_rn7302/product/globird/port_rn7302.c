/**
 * @file        port_rn7302.c
 * @brief       measure_rn7302 ↔ RN7302 设备文件对接（misc fops）
 *
 * 自 measure_process/input_hal/Rn7302File.c 移植：动态查找
 * /dev/rn7302-0.x，经 lseek+read/write 访问寄存器。
 *
 * Copyright (c) 2026, 广东碳索智控科技有限公司. All rights reserved.
 *
 * @par 修订日志
 *  日期        修订人    版本    修订内容
 * --------------------------------------------------------------
 *  2026-10-28  yansh    1.0.0   端口 RN7302
 *  2026-10-29  yansh    1.0.1   移植 Rn7302File misc 读写接口
 */
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include "port_rn7302.h"

/** 驱动 misc fd；未打开为 -1 */
static int misc_fd = -1;
/** 当前打开的设备路径（"/dev/" + NAME_MAX） */
static char current_device_path[5 + 255 + 1] = {0};

/**
 * @brief   在 /dev 下查找 rn7302-0.<chip>；找不到则填默认路径
 * @param[in]   chip          芯片号
 * @param[out]  device_path   设备路径
 * @param[in]   path_size     路径大小
 * @return  0 成功（含回退默认路径）；-1 无法扫描 /dev
 */
static int find_rn7302_device(int chip, char *device_path, size_t path_size)
{
    DIR *dir;
    struct dirent *entry;

    dir = opendir("/dev");
    if (dir == NULL) {
        printf("Failed to open /dev directory: %s\n", strerror(errno));
        return -1;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "rn7302-0.", 9) != 0) {
            continue;
        }
        {
            char *dot_pos = strchr(entry->d_name, '.');
            if (dot_pos != NULL && atoi(dot_pos + 1) == chip) {
                if (strlen(entry->d_name) + 5 >= path_size) {
                    closedir(dir);
                    return -1;
                }
                snprintf(device_path, path_size, "/dev/%s", entry->d_name);
                closedir(dir);
                return 0;
            }
        }
    }

    closedir(dir);

    snprintf(device_path, path_size, "/dev/rn7302-0.%d", chip);
    printf("Warning: Device /dev/rn7302-0.%d not found in /dev directory, "
           "trying default path\n", chip);
    return 0;
}

/**
 * @brief       列出 /dev 下全部 rn7302-0.*
 * @return      无
 */
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

/**
 * @brief       毫秒级延时
 * @param[in]   ms    延时毫秒数
 * @return      无
 */
void MeterChipDelayms(uint32_t ms)
{
    usleep(ms * 1000);
}

/**
 * @brief       初始化 RN7302 设备
 * @param[in]   chip          芯片号
 * @return      成功返回 fd（>=0）；失败 -1
 */
int MeterChipInit(int chip)
{
    int ret;
    char device_path[sizeof(current_device_path)] = {0};

    if (chip < 0) {
        printf("Invalid chip id = %d, it must be >= 0\n", chip);
        return -1;
    }

    list_available_rn7302_devices();

    ret = find_rn7302_device(chip, device_path, sizeof(device_path));
    if (ret < 0) {
        printf("Failed to find device for chip %d\n", chip);
        return -1;
    }

    strncpy(current_device_path, device_path, sizeof(current_device_path) - 1);
    current_device_path[sizeof(current_device_path) - 1] = '\0';

    misc_fd = open(device_path, O_RDWR);
    if (misc_fd < 0) {
        printf("Failed to open device %s: %s\n", device_path, strerror(errno));
        current_device_path[0] = '\0';
        return -1;
    }

    printf("Successfully opened device %s for chip %d\n", device_path, chip);
    return misc_fd;
}

/**
 * @brief       关闭 RN7302 设备
 */
void MeterChipDestory(void)
{
    if (misc_fd >= 0) {
        close(misc_fd);
        misc_fd = -1;
    }
    current_device_path[0] = '\0';
}

/**
 * @brief       读取寄存器
 * @param[in]   reg    寄存器地址
 * @param[in]   nbyte  字节数
 * @param[out]  pval   读取值
 * @return      0 成功；-1 失败
 */
int MeterChipReadReg(uint32_t reg, uint32_t nbyte, int32_t *pval)
{
    int ret;

    if (misc_fd < 0 || pval == NULL || nbyte == 0U) {
        return -1;
    }

    if (lseek(misc_fd, (off_t)reg, SEEK_SET) < 0) {
        return -1;
    }

    ret = (int)read(misc_fd, pval, nbyte);
    if (ret < 0) {
        return -1;
    }

    return 0;
}

/**
 * @brief       写寄存器
 * @param[in]   reg    寄存器地址
 * @param[in]   nbyte  字节数
 * @param[in]   val    写入值
 * @return      0 成功；-1 失败
 */
int MeterChipWriteReg(uint32_t reg, uint32_t nbyte, uint32_t val)
{
    int ret;

    if (misc_fd < 0 || nbyte == 0U) {
        return -1;
    }

    if (lseek(misc_fd, (off_t)reg, SEEK_SET) < 0) {
        return -1;
    }

    ret = (int)write(misc_fd, &val, nbyte);
    if (ret < 0) {
        return -1;
    }

    return 0;
}

/**
 * @brief       读取寄存器区域
 * @param[in]   reg    寄存器地址
 * @param[in]   nbyte  字节数
 * @param[out]  pval   读取值
 * @return      0 成功；-1 失败
 */
int32_t MeterChipReadArea(uint32_t reg, uint32_t nbyte, uint32_t *pval)
{
    int ret;
    int saved_errno;

    if (misc_fd < 0 || pval == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* only support burst read 8 regs, 4 bytes every reg */
    if (nbyte != 32U) {
        printf("[HW] MeterChipReadArea invalid nbyte=%u reg=0x%X\n", nbyte, reg);
        errno = EINVAL;
        return -1;
    }

    if (lseek(misc_fd, (off_t)reg, SEEK_SET) < 0) {
        return -1;
    }

    /* 驱动按读 8 个寄存器约定；实际返回 32 字节 */
    ret = (int)read(misc_fd, pval, 8);
    saved_errno = errno;
    if (ret < 0) {
        printf("[HW] MeterChipReadArea fail reg=0x%X ret=%d errno=%d(%s)\n",
               reg, ret, saved_errno, strerror(saved_errno));
        errno = saved_errno;
        return -1;
    }
    if (ret != 32) {
        printf("[HW] MeterChipReadArea unexpected ret=%d (expect 32) "
               "reg=0x%X errno=%d(%s)\n",
               ret, reg, saved_errno, strerror(saved_errno));
        errno = (saved_errno != 0) ? saved_errno : EIO;
        return -1;
    }

    return ret;
}

/**
 * @brief       获取当前设备路径
 * @return      设备路径
 */
const char *MeterChipGetDevicePath(void)
{
    if (misc_fd < 0) {
        return NULL;
    }
    return current_device_path;
}

/**
 * @brief       获取当前设备 fd
 * @return      设备 fd
 */
int MeterChipGetFd(void)
{
    return misc_fd;
}
