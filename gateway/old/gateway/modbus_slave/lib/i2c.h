
#include <linux/i2c.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/ioctl.h>

#define I2C_SLAVE			0x0703
#define I2C_SLAVE_FORCE		0x0706
#define I2C_FUNCS			0x0705
#define I2C_PEC				0x0708
#define I2C_SMBUS			0x0720
#define I2C_RDWR			0x0707

struct i2c_smbus_ioctl_data {
	__u8 read_write;
	__u8 command;
	__u32 size;
	union i2c_smbus_data *data;
};

/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int32_t i2c_smbus_access(int fd, char read_write, uint8_t cmd,
				int size, union i2c_smbus_data *data)
{
	struct i2c_smbus_ioctl_data args;

	args.read_write = read_write;
	args.command = cmd;
	args.size = size;
	args.data = data;

	return ioctl(fd, I2C_SMBUS, &args);
}

/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int i2c_dev_open(int i2cbus, int addr)
{
	char filename[sizeof("/dev/i2c-%d") + sizeof(int)*3];
	int fd;
    int ret;

	sprintf(filename, "/dev/i2c-%d", i2cbus);
	fd = open(filename, O_RDWR);
	if (fd < 0) {
		printf("can't open '%s'", filename);
		return -1;
	}

    ret = ioctl(fd, I2C_SLAVE_FORCE, addr);
	if(ret < 0)
		return -1;

	return fd;
}

/*********************************************************************************************************
** Function name:       
** Descriptions:        PMAC770 采用SD3078的时钟芯片，该芯片的0x1a寄存器 bit7 代表电池电压的第9位。0x1b寄存器是电池电压的低8位字节
                        生产测试使用该位判断电池电压是否异常。为了快速检测，只取第9位判断是否为1，来判断电池是否异常
                        比如  0x100 为 256 = 2.56V，  < 0x100 就认为电池异常
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_i2c_rtc_volt_get(uint16_t *pVolt)
{
    int fd;
    union i2c_smbus_data data;
    int ret = -1;
	uint16_t volt;

    fd = i2c_dev_open(0, 0x32);
    if(fd < 0)
        return -1;

    ret = i2c_smbus_access(fd, I2C_SMBUS_READ, 0x1a, I2C_SMBUS_BYTE_DATA, &data) ;
	if(ret < 0)
		goto __error;

	volt = ((uint16_t)data.byte & 0x80) << 1;

	ret = i2c_smbus_access(fd, I2C_SMBUS_READ, 0x1b, I2C_SMBUS_BYTE_DATA, &data) ;
	if(ret < 0)
		goto __error;

	*pVolt = volt | data.byte;
	
__error:
    close(fd);

    return ret;
}