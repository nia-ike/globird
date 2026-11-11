
#include <linux/i2c.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>

#define I2C_SLAVE			0x0703
#define I2C_SLAVE_FORCE		0x0706
#define I2C_FUNCS			0x0705
#define I2C_PEC				0x0708
#define I2C_SMBUS			0x0720
#define I2C_RDWR			0x0707

#define SD3078_I2C_BUS		0
#define SD3078_I2C_ADDR		0x32
#define SD3078_REG_VOLT_HI	0x1a	/* bit7 = 电压 bit8 */
#define SD3078_REG_VOLT_LO	0x1b	/* 电压低 8 位 */

struct i2c_smbus_ioctl_data {
	__u8 read_write;
	__u8 command;
	__u32 size;
	union i2c_smbus_data *data;
};

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

static int i2c_dev_open(int i2cbus, int addr)
{
	char filename[32];
	int fd;

	snprintf(filename, sizeof(filename), "/dev/i2c-%d", i2cbus);
	fd = open(filename, O_RDWR);
	if (fd < 0)
		return -1;

	if (ioctl(fd, I2C_SLAVE_FORCE, addr) < 0) {
		close(fd);
		return -1;
	}

	return fd;
}

/*********************************************************************************************************
** Descriptions:        SD3078 RTC 电池电压：0x1a[7]+0x1b，单位 0.01V
**                      0x100(=256)=2.56V；<0x100 可判电池异常
** Returned value:      0 成功；<0 失败
*********************************************************************************************************/
static int mb_i2c_rtc_volt_get(uint16_t *pVolt)
{
	int fd;
	union i2c_smbus_data data;
	int ret;
	uint16_t volt;

	if (pVolt == NULL)
		return -1;

	fd = i2c_dev_open(SD3078_I2C_BUS, SD3078_I2C_ADDR);
	if (fd < 0)
		return -1;

	ret = i2c_smbus_access(fd, I2C_SMBUS_READ, SD3078_REG_VOLT_HI,
			       I2C_SMBUS_BYTE_DATA, &data);
	if (ret < 0)
		goto out;

	volt = ((uint16_t)data.byte & 0x80) << 1;

	ret = i2c_smbus_access(fd, I2C_SMBUS_READ, SD3078_REG_VOLT_LO,
			       I2C_SMBUS_BYTE_DATA, &data);
	if (ret < 0)
		goto out;

	*pVolt = volt | (uint16_t)data.byte;
	ret = 0;

out:
	close(fd);
	return ret;
}
