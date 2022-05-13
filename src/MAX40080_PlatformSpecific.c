#include "MAX40080_PlatformSpecific.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <i2c/smbus.h>
#include <sys/ioctl.h>

static int fd = -1;
static int deviceAddress = MAX40080_I2C_7BIT_ADDRESS;
static int controllerNumber = 1;

MAX40080_Status MAX40080_PlatformSpecific_Init() {
	int status;
	char path[32];

	status = snprintf(path, sizeof(path), "/dev/i2c-%d", controllerNumber);
	// 10 is length of minimal output "/dev/i2c-0". If we get string 
	// less than 10 characters something went wrong and stdlib is buggy.
	// Function also returns negative number on error. This is handled by 
	// the same condition.
	if (status < 10 || status >= sizeof(path)) {
		return MAX40080_Status_InvalidOperation;
	}

	fd = open(path, O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "Error while opening %s. Details: %s\n", path, strerror(errno));
		return MAX40080_Status_I2CError;
	}

	status = ioctl(fd, I2C_SLAVE, deviceAddress);
	if (status < 0) {
		fprintf(stderr, "Error while setting I2C slave address. Details: %s\n", strerror(errno));
		return MAX40080_Status_I2CError;
	}

	return MAX40080_Status_Ok;
}

MAX40080_Status MAX40080_PlatformSpecific_Deinit() {
	close(fd);
	fd = -1;
	return MAX40080_Status_Ok;
}

MAX40080_Status MAX40080_PlatformSpecific_Read(uint8_t registerAddress, uint8_t* buffer, size_t bufferSize) {
	int status;
	struct i2c_msg messages[2];
	struct i2c_rdwr_ioctl_data data;

	messages[0].addr = MAX40080_I2C_7BIT_ADDRESS;
	messages[0].flags = 0;
	messages[0].buf = &registerAddress;
	messages[0].len = 1;

	messages[1].addr = MAX40080_I2C_7BIT_ADDRESS;
	messages[1].flags = I2C_M_RD;
	messages[1].buf = buffer;
	messages[1].len = bufferSize;

	data.msgs = messages;
	data.nmsgs = 2;

	status = ioctl(fd, I2C_RDWR, &data);
	if (status < 0) {
		int e = errno;
		if (e == EREMOTEIO) {
			return MAX40080_Status_I2CNack;
		} else if (e == ETIMEDOUT) {
			return MAX40080_Status_I2CTimeout;
		} else {
			return MAX40080_Status_I2CError;
		}
	}

	return MAX40080_Status_Ok;
}

MAX40080_Status MAX40080_PlatformSpecific_Write(uint8_t registerAddress, uint8_t* buffer, size_t bufferSize) {
	uint8_t bufferToTransmit[7];
	int status;

	if (bufferSize > 6) {
		return MAX40080_Status_NotImplemented;
	}

	bufferToTransmit[0] = registerAddress;

	for (size_t i = 0; i < bufferSize; i++) {
		bufferToTransmit[i + 1] = buffer[i];
	}

	status = write(fd, bufferToTransmit, bufferSize + 1);
	if (status != bufferSize + 1) {
		int e = errno;
		if (e == EREMOTEIO) {
			return MAX40080_Status_I2CNack;
		} else if (e == ETIMEDOUT) {
			return MAX40080_Status_I2CTimeout;
		} else {
			return MAX40080_Status_I2CError;
		}
	}

	return MAX40080_Status_Ok;
}

MAX40080_Status MAX40080_PlatformSpecific_QuickCommand() {
	int status;

	status = i2c_smbus_write_quick(fd, 0);
	if (status != 0) {
		int e = errno;
		if (e == EREMOTEIO) {
			return MAX40080_Status_I2CNack;
		} else if (e == ETIMEDOUT) {
			return MAX40080_Status_I2CTimeout;
		} else {
			return MAX40080_Status_I2CError;
		}
	}

	return MAX40080_Status_Ok;
}

MAX40080_Status MAX40080_PlatformSpecific_SetI2CSpeed(MAX40080_I2CSpeed speed) {
	return MAX40080_Status_NotSupported;
}

MAX40080_Status MAX40080_PlatformSpecific_SetI2CControllerNo(int number) {
	controllerNumber = number;
	return MAX40080_Status_Ok;
}

MAX40080_Status MAX40080_PlatformSpecific_SetI2CAddress(int address) {
	deviceAddress = address;
	return MAX40080_Status_Ok;
}
