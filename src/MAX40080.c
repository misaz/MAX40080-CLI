#include "MAX40080.h"
#include "MAX40080_PlatformSpecific.h"

static MAX40080_Configuration MAX40080_CurrentConfig;
static MAX40080_FifoConfiguration MAX40080_CurrentFifoConfig;

static const uint8_t MAX40080_Crc8Table[] = {
	0x00, 0x07, 0x0E, 0x09, 0x1C, 0x1B, 0x12, 0x15, 0x38, 0x3F, 0x36, 0x31, 0x24, 0x23, 0x2A, 0x2D,
	0x70, 0x77, 0x7E, 0x79, 0x6C, 0x6B, 0x62, 0x65, 0x48, 0x4F, 0x46, 0x41, 0x54, 0x53, 0x5A, 0x5D,
	0xE0, 0xE7, 0xEE, 0xE9, 0xFC, 0xFB, 0xF2, 0xF5, 0xD8, 0xDF, 0xD6, 0xD1, 0xC4, 0xC3, 0xCA, 0xCD,
	0x90, 0x97, 0x9E, 0x99, 0x8C, 0x8B, 0x82, 0x85, 0xA8, 0xAF, 0xA6, 0xA1, 0xB4, 0xB3, 0xBA, 0xBD,
	0xC7, 0xC0, 0xC9, 0xCE, 0xDB, 0xDC, 0xD5, 0xD2, 0xFF, 0xF8, 0xF1, 0xF6, 0xE3, 0xE4, 0xED, 0xEA,
	0xB7, 0xB0, 0xB9, 0xBE, 0xAB, 0xAC, 0xA5, 0xA2, 0x8F, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9D, 0x9A,
	0x27, 0x20, 0x29, 0x2E, 0x3B, 0x3C, 0x35, 0x32, 0x1F, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0D, 0x0A,
	0x57, 0x50, 0x59, 0x5E, 0x4B, 0x4C, 0x45, 0x42, 0x6F, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7D, 0x7A,
	0x89, 0x8E, 0x87, 0x80, 0x95, 0x92, 0x9B, 0x9C, 0xB1, 0xB6, 0xBF, 0xB8, 0xAD, 0xAA, 0xA3, 0xA4,
	0xF9, 0xFE, 0xF7, 0xF0, 0xE5, 0xE2, 0xEB, 0xEC, 0xC1, 0xC6, 0xCF, 0xC8, 0xDD, 0xDA, 0xD3, 0xD4,
	0x69, 0x6E, 0x67, 0x60, 0x75, 0x72, 0x7B, 0x7C, 0x51, 0x56, 0x5F, 0x58, 0x4D, 0x4A, 0x43, 0x44,
	0x19, 0x1E, 0x17, 0x10, 0x05, 0x02, 0x0B, 0x0C, 0x21, 0x26, 0x2F, 0x28, 0x3D, 0x3A, 0x33, 0x34,
	0x4E, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5C, 0x5B, 0x76, 0x71, 0x78, 0x7F, 0x6A, 0x6D, 0x64, 0x63,
	0x3E, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2C, 0x2B, 0x06, 0x01, 0x08, 0x0F, 0x1A, 0x1D, 0x14, 0x13,
	0xAE, 0xA9, 0xA0, 0xA7, 0xB2, 0xB5, 0xBC, 0xBB, 0x96, 0x91, 0x98, 0x9F, 0x8A, 0x8D, 0x84, 0x83,
	0xDE, 0xD9, 0xD0, 0xD7, 0xC2, 0xC5, 0xCC, 0xCB, 0xE6, 0xE1, 0xE8, 0xEF, 0xFA, 0xFD, 0xF4, 0xF3
};

MAX40080_Status MAX40080_Init() {
	MAX40080_Status status;

	status = MAX40080_PlatformSpecific_Init();
	if (status) {
		return status;
	}

	// at init time we dont know if PEC is enabled or disabled. Suppose it is disabled.
	MAX40080_CurrentConfig.packetErrorChecking = MAX40080_PacketErrorChecking_Disabled;

	// now we can read configuration (but ignore PEC even it is enabled)
	status = MAX40080_GetConfiguration(&MAX40080_CurrentConfig);
	if (status) {
		return status;
	}

	// MAX40080_CurrentConfig.packetErrorChecking field was updated now so future requests will or 
	// will not correctly check PEC according to actual settings.

	return MAX40080_Status_Ok;
}

MAX40080_Status MAX40080_Deinit() {
	return MAX40080_PlatformSpecific_Deinit();
}

static uint8_t MAX40080_CalculateCrc(uint8_t* buffer, size_t bufferSize) {
	uint8_t crc = 0;

	while (bufferSize--) {
		crc = MAX40080_Crc8Table[(*buffer++) ^ crc];
	}

	return crc;
}

static MAX40080_Status MAX40080_CheckCrc(uint8_t* buffer, size_t bufferSize, uint8_t receivedCrc) {
	if (MAX40080_CalculateCrc(buffer, bufferSize) != receivedCrc) {
		return MAX40080_Status_PacketErrorCheckFailed;
	} else {
		return MAX40080_Status_Ok;
	}
}

static MAX40080_Status MAX40080_ReadRegister8(uint8_t registerAddress, uint8_t* registerValue) {
	MAX40080_Status status;

	// devAddr+WR (1 Byte) + registerAddress (1 Byte) + devAddr+RD (1 Byte) + registerValue (1 Byte) + CRC (1 Byte)
	uint8_t crcBuffer[5];

	size_t readSize = sizeof(uint16_t);
	if (MAX40080_CurrentConfig.packetErrorChecking == MAX40080_PacketErrorChecking_Enabled) {
		readSize++;
	}

	crcBuffer[0] = MAX40080_I2C_WRITE_ADDRESS;
	crcBuffer[1] = registerAddress;
	crcBuffer[2] = MAX40080_I2C_READ_ADDRESS;

	status = MAX40080_PlatformSpecific_Read(registerAddress, crcBuffer + 3, readSize);
	if (status) {
		return status;
	}

	if (MAX40080_CurrentConfig.packetErrorChecking == MAX40080_PacketErrorChecking_Enabled) {
		// checking input: devAddr+WR (1 Byte) + registerAddress (1 Byte) + devAddr+RD (1 Byte) + registerValue (1 Byte)
		status = MAX40080_CheckCrc(crcBuffer, 4, crcBuffer[4]);
		if (status) {
			return status;
		}
	}

	*registerValue =
		(((uint8_t)crcBuffer[1]) << 0);

	return MAX40080_Status_Ok;
}

static MAX40080_Status MAX40080_ReadRegister16(uint8_t registerAddress, uint16_t* registerValue) {
	MAX40080_Status status;

	// devAddr+WR (1 Byte) + registerAddress (1 Byte) + devAddr+RD (1 Byte) + registerValue (2 Bytes) + CRC (1 Byte)
	uint8_t crcBuffer[6];

	size_t readSize = sizeof(uint16_t);
	if (MAX40080_CurrentConfig.packetErrorChecking == MAX40080_PacketErrorChecking_Enabled) {
		readSize++;
	}

	crcBuffer[0] = MAX40080_I2C_WRITE_ADDRESS;
	crcBuffer[1] = registerAddress;
	crcBuffer[2] = MAX40080_I2C_READ_ADDRESS;

	status = MAX40080_PlatformSpecific_Read(registerAddress, crcBuffer + 3, readSize);
	if (status) {
		return status;
	}

	if (MAX40080_CurrentConfig.packetErrorChecking == MAX40080_PacketErrorChecking_Enabled) {
		// checking input: devAddr+WR (1 Byte) + registerAddress (1 Byte) + devAddr+RD (1 Byte) + registerValue (2 Bytes)
		status = MAX40080_CheckCrc(crcBuffer, 5, crcBuffer[5]);
		if (status) {
			return status;
		}
	}

	*registerValue =
		(((uint16_t)crcBuffer[3]) << 0) |
		(((uint16_t)crcBuffer[4]) << 8);

	return MAX40080_Status_Ok;
}

static MAX40080_Status MAX40080_ReadRegister32(uint8_t registerAddress, uint32_t* registerValue) {
	MAX40080_Status status;

	// devAddr+WR (1 Byte) + registerAddress (1 Byte) + devAddr+RD (1 Byte) + registerValue (4 Bytes) + CRC (1 Byte)
	uint8_t crcBuffer[8];

	size_t readSize = sizeof(uint32_t);
	if (MAX40080_CurrentConfig.packetErrorChecking == MAX40080_PacketErrorChecking_Enabled) {
		readSize++;
	}

	crcBuffer[0] = MAX40080_I2C_WRITE_ADDRESS;
	crcBuffer[1] = registerAddress;
	crcBuffer[2] = MAX40080_I2C_READ_ADDRESS;

	status = MAX40080_PlatformSpecific_Read(registerAddress, crcBuffer + 3, readSize);
	if (status) {
		return status;
	}

	if (MAX40080_CurrentConfig.packetErrorChecking == MAX40080_PacketErrorChecking_Enabled) {
		// checking input: devAddr+WR (1 Byte) + registerAddress (1 Byte) + devAddr+RD (1 Byte) + registerValue (4 Bytes)
		status = MAX40080_CheckCrc(crcBuffer, 7, crcBuffer[7]);
		if (status) {
			return status;
		}
	}

	*registerValue =
		(((uint32_t)crcBuffer[3]) << 0) |
		(((uint32_t)crcBuffer[4]) << 8) |
		(((uint32_t)crcBuffer[5]) << 16) |
		(((uint32_t)crcBuffer[6]) << 24);

	return MAX40080_Status_Ok;
}

static MAX40080_Status MAX40080_WriteRegister8(uint8_t registerAddress, uint8_t registerValue) {
	MAX40080_Status status;
	uint8_t crcBuffer[4]; // devAddr+WR (1 Byte) + registerAddress (1 Byte) + registerValue (1 Bytes) + CRC (1 Byte)

	crcBuffer[0] = MAX40080_I2C_WRITE_ADDRESS;
	crcBuffer[1] = registerAddress;
	crcBuffer[2] = registerValue;

	size_t writeSize = sizeof(uint8_t);

	if (MAX40080_CurrentConfig.packetErrorChecking == MAX40080_PacketErrorChecking_Enabled) {
		crcBuffer[3] = MAX40080_CalculateCrc(crcBuffer, 3);
		writeSize++;
	}

	status = MAX40080_PlatformSpecific_Write(registerAddress, crcBuffer + 2, writeSize);
	if (status) {
		return status;
	}

	return MAX40080_Status_Ok;
}

static MAX40080_Status MAX40080_WriteRegister16(uint8_t registerAddress, uint16_t registerValue) {
	MAX40080_Status status;
	uint8_t crcBuffer[5]; // devAddr+WR (1 Byte) + registerAddress (1 Byte) + registerValue (2 Bytes) + CRC (1 Byte)

	crcBuffer[0] = MAX40080_I2C_WRITE_ADDRESS;
	crcBuffer[1] = registerAddress;
	crcBuffer[2] = registerValue & 0xFF;
	crcBuffer[3] = (registerValue & 0xFF00) >> 8;

	size_t writeSize = sizeof(uint16_t);

	if (MAX40080_CurrentConfig.packetErrorChecking == MAX40080_PacketErrorChecking_Enabled) {
		crcBuffer[4] = MAX40080_CalculateCrc(crcBuffer, 4);
		writeSize++;
	}

	status = MAX40080_PlatformSpecific_Write(registerAddress, crcBuffer + 2, writeSize);
	if (status) {
		return status;
	}

	return MAX40080_Status_Ok;
}

MAX40080_Status MAX40080_GetDefaultConfiguration(MAX40080_Configuration* config) {
	config->operatingMode = MAX40080_OperationMode_Standby;
	config->i2cTimeoutSettings = MAX40080_I2CTimeoutSettings_Enabled;
	config->alertResponseTime = MAX40080_AlertResponseTime_Unfiltered;
	config->packetErrorChecking = MAX40080_PacketErrorChecking_Enabled;
	config->inputRange = MAX40080_InputRange_10mV;
	config->stayHsMode = MAX40080_StayHsMode_ExitAtStop;
	config->adcSampleRate = MAX40080_AdcSampleRate_Either_at_15_ksps;
	config->digitalFilter = MAX40080_DigitalFilter_NoAverage;

	return MAX40080_Status_Ok;
}

MAX40080_Status MAX40080_GetConfiguration(MAX40080_Configuration* config) {
	MAX40080_Status status;
	uint16_t configRegVal;

	status = MAX40080_ReadRegister16(MAX40080_REG_CONFIG, &configRegVal);
	if (status) {
		return status;
	}

	config->operatingMode = MAX40080_GET_FIELD(MAX40080_CONFIG_MODE_FIELD, configRegVal);
	config->i2cTimeoutSettings = MAX40080_GET_FIELD(MAX40080_CONFIG_I2C_TIMEOUT_FIELD, configRegVal);
	config->alertResponseTime = MAX40080_GET_FIELD(MAX40080_CONFIG_ALERT_FIELD, configRegVal);
	config->packetErrorChecking = MAX40080_GET_FIELD(MAX40080_CONFIG_PEC_FIELD, configRegVal);
	config->inputRange = MAX40080_GET_FIELD(MAX40080_CONFIG_INPUT_RANGE_FIELD, configRegVal);
	config->stayHsMode = MAX40080_GET_FIELD(MAX40080_CONFIG_STAY_HS_MODE_FIELD, configRegVal);
	config->adcSampleRate = MAX40080_GET_FIELD(MAX40080_CONFIG_ADC_SAMPLE_RATE_FIELD, configRegVal);
	config->digitalFilter = MAX40080_GET_FIELD(MAX40080_CONFIG_DIGITAL_FILTER_FIELD, configRegVal);

	return MAX40080_Status_Ok;
}

MAX40080_Status MAX40080_SetConfiguration(MAX40080_Configuration* config) {
	MAX40080_Status status;

	uint16_t configRegVal =
		MAX40080_SET_FIELD(MAX40080_CONFIG_MODE_FIELD, config->operatingMode) |
		MAX40080_SET_FIELD(MAX40080_CONFIG_I2C_TIMEOUT_FIELD, config->i2cTimeoutSettings) |
		MAX40080_SET_FIELD(MAX40080_CONFIG_ALERT_FIELD, config->alertResponseTime) |
		MAX40080_SET_FIELD(MAX40080_CONFIG_PEC_FIELD, config->packetErrorChecking) |
		MAX40080_SET_FIELD(MAX40080_CONFIG_INPUT_RANGE_FIELD, config->inputRange) |
		MAX40080_SET_FIELD(MAX40080_CONFIG_STAY_HS_MODE_FIELD, config->stayHsMode) |
		MAX40080_SET_FIELD(MAX40080_CONFIG_ADC_SAMPLE_RATE_FIELD, config->adcSampleRate) |
		MAX40080_SET_FIELD(MAX40080_CONFIG_DIGITAL_FILTER_FIELD, config->digitalFilter);

	status = MAX40080_WriteRegister16(MAX40080_REG_CONFIG, configRegVal);
	if (status) {
		return status;
	}

	MAX40080_CurrentConfig.operatingMode = config->operatingMode;
	MAX40080_CurrentConfig.i2cTimeoutSettings = config->i2cTimeoutSettings;
	MAX40080_CurrentConfig.alertResponseTime = config->alertResponseTime;
	MAX40080_CurrentConfig.packetErrorChecking = config->packetErrorChecking;
	MAX40080_CurrentConfig.inputRange = config->inputRange;
	MAX40080_CurrentConfig.stayHsMode = config->stayHsMode;
	MAX40080_CurrentConfig.adcSampleRate = config->adcSampleRate;
	MAX40080_CurrentConfig.digitalFilter = config->digitalFilter;

	return MAX40080_Status_Ok;
}

MAX40080_Status MAX40080_EnableInterrupts(MAX40080_Interrupt interrupts) {
	MAX40080_Status status;

	status = MAX40080_WriteRegister16(MAX40080_REG_INTERRUPT_ENABLE, (uint16_t)interrupts);
	if (status) {
		return status;
	}

	return MAX40080_Status_Ok;
}

MAX40080_Status MAX40080_GetPendingInterrupts(MAX40080_Interrupt* interrupts) {
	MAX40080_Status status;
	uint16_t statusRegVal;

	status = MAX40080_ReadRegister16(MAX40080_REG_STATUS, &statusRegVal);
	if (status) {
		return status;
	}

	*interrupts = statusRegVal & 0xFF;

	return MAX40080_Status_Ok;
}

MAX40080_Status MAX40080_ClearPendingInterrupts(MAX40080_Interrupt interrupts) {
	MAX40080_Status status;

	status = MAX40080_WriteRegister16(MAX40080_REG_STATUS, (uint16_t)interrupts);
	if (status) {
		return status;
	}

	return MAX40080_Status_Ok;
}

MAX40080_Status MAX40080_GetAvailableFifoDataCount(uint8_t* fifoDataCount) {
	MAX40080_Status status;
	uint16_t statusRegVal;

	status = MAX40080_ReadRegister16(MAX40080_REG_STATUS, &statusRegVal);
	if (status) {
		return status;
	}

	*fifoDataCount = MAX40080_GET_FIELD(MAX40080_STATUS_FIFO_DATA_COUNT_FIELD, statusRegVal);
	if (*fifoDataCount == 0 && (statusRegVal & MAX40080_Interrupt_FifoOverflown)) {
		*fifoDataCount = MAX40080_FIFO_CAPACITY;
	}

	return MAX40080_Status_Ok;

}

MAX40080_Status MAX40080_TriggerSingleShootConversion() {
	return MAX40080_PlatformSpecific_QuickCommand();
}

MAX40080_Status MAX40080_ReadRawCurrent(int16_t* current) {
	MAX40080_Status status;
	uint16_t currentRegVal;

	if (MAX40080_CurrentFifoConfig.storingMode != MAX40080_FifoStoringMode_CurrentOnly) {
		// You used wrong function to retrieve data from FIFO. Following list contains 
		// valid function used for reading in specific FIFO storing configuration:
		// 
		// MAX40080_FifoStoringMode_CurrentOnly => MAX40080_ReadRawCurrent
		// MAX40080_FifoStoringMode_VoltageOnly => MAX40080_ReadRawVoltage
		// MAX40080_FifoStoringMode_CurrentAndVoltage => MAX40080_ReadRawCurrentAndVoltage

		return MAX40080_Status_InvalidOperation;
	}

	status = MAX40080_ReadRegister16(MAX40080_REG_CURRENT_MEASUREMENT, &currentRegVal);
	if (status) {
		return status;
	}

	if ((currentRegVal & 0x8000) == 0) {
		return MAX40080_Status_FifoIsEmpty;
	}

	// value is shifted left to replace Data Valid bit.
	*current = (currentRegVal & 0x7FFF) << 1;

	// sign extended shift back
	*current >>= 1;

	return MAX40080_Status_Ok;
}

MAX40080_Status MAX40080_ReadRawVoltage(int16_t* voltage) {
	MAX40080_Status status;
	uint16_t voltageRegVal;

	if (MAX40080_CurrentFifoConfig.storingMode != MAX40080_FifoStoringMode_VoltageOnly) {
		// You used wrong function to retrieve data from FIFO. Following list contains 
		// valid function used for reading in specific FIFO storing configuration:
		// 
		// MAX40080_FifoStoringMode_CurrentOnly => MAX40080_ReadRawCurrent
		// MAX40080_FifoStoringMode_VoltageOnly => MAX40080_ReadRawVoltage
		// MAX40080_FifoStoringMode_CurrentAndVoltage => MAX40080_ReadRawCurrentAndVoltage

		return MAX40080_Status_InvalidOperation;
	}

	status = MAX40080_ReadRegister16(MAX40080_REG_VOLTAGE_MEASUREMENT, &voltageRegVal);
	if (status) {
		return status;
	}

	if ((voltageRegVal & 0x8000) == 0) {
		return MAX40080_Status_FifoIsEmpty;
	}

	// value is shifted left to replace Data Valid bit.
	*voltage = (voltageRegVal & 0x7FFF) << 1;

	// sign extended shift back
	*voltage >>= 1;

	return MAX40080_Status_Ok;
}

MAX40080_Status MAX40080_ReadRawCurrentAndVoltage(int16_t* current, int16_t* voltage) {
	MAX40080_Status status;
	uint32_t currentVoltageRegVal;

	if (MAX40080_CurrentFifoConfig.storingMode != MAX40080_FifoStoringMode_CurrentAndVoltage) {
		// You used wrong function to retrieve data from FIFO. Following list contains 
		// valid function used for reading in specific FIFO storing configuration:
		// 
		// MAX40080_FifoStoringMode_CurrentOnly => MAX40080_ReadRawCurrent
		// MAX40080_FifoStoringMode_VoltageOnly => MAX40080_ReadRawVoltage
		// MAX40080_FifoStoringMode_CurrentAndVoltage => MAX40080_ReadRawCurrentAndVoltage

		return MAX40080_Status_InvalidOperation;
	}

	status = MAX40080_ReadRegister32(MAX40080_REG_CURRENT_VOLTAGE_MEASUREMENT, &currentVoltageRegVal);
	if (status) {
		return status;
	}

	if ((currentVoltageRegVal & 0x80000000) == 0) {
		return MAX40080_Status_FifoIsEmpty;
	}

	// value is shifted left to replace Data Valid bit. Then it is shifted right and converted to 
	// signed int16_t
	*voltage = (int16_t)((int32_t)((currentVoltageRegVal & 0x7FFF0000) << 1) >> 17);

	// value is shifted left to replace reserved bit. The it is converted to signed and shifted back 
	// to extend the sign.
	*current = ((int16_t)((uint16_t)(currentVoltageRegVal & 0x7FFF) << 1)) >> 1;

	return MAX40080_Status_Ok;
}

static float MAX40080_ConvertCurrent(int16_t rawCurrent) {
	float range;
	if (MAX40080_CurrentConfig.inputRange == MAX40080_InputRange_50mV) {
		range = 0.050;
	} else {
		range = 0.010;
	}

	return range * rawCurrent / (float)(1 << 12) / (float)(MAX40080_SHUNT_RESISOTR_VALUE);
}

static float MAX40080_ConvertVoltage(int16_t rawVoltage) {
	// 37.5 is 1.25 (ADC reference; Vref in datasheet) * 30 (Voltage buffer gain; G in Datasheet)
	return rawVoltage * 37.5 / (1 << 12);
}

MAX40080_Status MAX40080_ReadCurrent(float* current) {
	MAX40080_Status status;
	int16_t rawCurrent;

	status = MAX40080_ReadRawCurrent(&rawCurrent);
	if (status) {
		return status;
	}

	*current = MAX40080_ConvertCurrent(rawCurrent);

	return MAX40080_Status_Ok;
}

MAX40080_Status MAX40080_ReadVoltage(float* voltage) {
	MAX40080_Status status;
	int16_t rawVoltage;

	status = MAX40080_ReadRawVoltage(&rawVoltage);
	if (status) {
		return status;
	}

	*voltage = MAX40080_ConvertVoltage(rawVoltage);

	return MAX40080_Status_Ok;
}

MAX40080_Status MAX40080_ReadCurrentAndVoltage(float* current, float* voltage) {
	MAX40080_Status status;
	int16_t rawCurrent;
	int16_t rawVoltage;

	status = MAX40080_ReadRawCurrentAndVoltage(&rawCurrent, &rawVoltage);
	if (status) {
		return status;
	}

	*current = MAX40080_ConvertCurrent(rawCurrent);
	*voltage = MAX40080_ConvertVoltage(rawVoltage);

	return MAX40080_Status_Ok;
}

MAX40080_Status MAX40080_GetRawOverCurrentTreshold(uint8_t* treshold) {
	return MAX40080_ReadRegister8(MAX40080_REG_TRESHOLD_OVER_CURRENT, treshold);
}

MAX40080_Status MAX40080_SetRawOverCurrentTreshold(uint8_t treshold) {
	return MAX40080_WriteRegister8(MAX40080_REG_TRESHOLD_OVER_CURRENT, treshold);
}

MAX40080_Status MAX40080_GetRawOverVoltageTreshold(uint8_t* treshold) {
	return MAX40080_ReadRegister8(MAX40080_REG_TRESHOLD_OVER_VOLTAGE, treshold);
}

MAX40080_Status MAX40080_SetRawOverVoltageTreshold(uint8_t treshold) {
	return MAX40080_WriteRegister8(MAX40080_REG_TRESHOLD_OVER_VOLTAGE, treshold);
}

MAX40080_Status MAX40080_GetRawUnderVoltageTreshold(uint8_t* treshold) {
	return MAX40080_ReadRegister8(MAX40080_REG_TRESHOLD_UNDER_VOLTAGE, treshold);
}

MAX40080_Status MAX40080_SetRawUnderVoltageTreshold(uint8_t treshold) {
	return MAX40080_WriteRegister8(MAX40080_REG_TRESHOLD_UNDER_VOLTAGE, treshold);
}

MAX40080_Status MAX40080_GetRawWakeUpCurrent(uint8_t* wakeUpCurrent) {
	return MAX40080_ReadRegister8(MAX40080_REG_WAKE_UP_CURRENT, wakeUpCurrent);
}

MAX40080_Status MAX40080_SetRawWakeUpCurrent(uint8_t wakeUpCurrent) {
	return MAX40080_WriteRegister8(MAX40080_REG_WAKE_UP_CURRENT, wakeUpCurrent);
}

MAX40080_Status MAX40080_GetRawMaxPeakCurrent(uint16_t* maxPeakCurrent) {
	return MAX40080_ReadRegister16(MAX40080_REG_MAX_PEAK_CURRENT, maxPeakCurrent);
}

MAX40080_Status MAX40080_GetFifoDefaultConfiguration(MAX40080_FifoConfiguration* fifoConfig) {
	fifoConfig->overflowWarningLevel = 0x34;
	fifoConfig->rollOverMode = MAX40080_FifoRollOverMode_Ignore;
	fifoConfig->storingMode = MAX40080_FifoStoringMode_CurrentOnly;

	return MAX40080_Status_Ok;
}

MAX40080_Status MAX40080_GetFifoConfiguration(MAX40080_FifoConfiguration* fifoConfig) {
	MAX40080_Status status;
	uint16_t fifoConfigRegVal;

	status = MAX40080_ReadRegister16(MAX40080_REG_FIFO_CONFIG, &fifoConfigRegVal);
	if (status) {
		return status;
	}

	fifoConfig->storingMode = MAX40080_GET_FIELD(MAX40080_FIFO_CONFIG_STORING_MODE_FIELD, fifoConfigRegVal);
	fifoConfig->overflowWarningLevel = MAX40080_GET_FIELD(MAX40080_FIFO_CONFIG_WARNING_LEVEL_FIELD, fifoConfigRegVal);
	fifoConfig->rollOverMode = MAX40080_GET_FIELD(MAX40080_FIFO_CONFIG_ROLL_OVER_FIELD, fifoConfigRegVal);

	return MAX40080_Status_Ok;
}

MAX40080_Status MAX40080_SetFifoConfiguration(MAX40080_FifoConfiguration* fifoConfig) {
	MAX40080_Status status;

	uint16_t fifoConfigRegVal =
		MAX40080_SET_FIELD(MAX40080_FIFO_CONFIG_STORING_MODE_FIELD, fifoConfig->storingMode) |
		MAX40080_SET_FIELD(MAX40080_FIFO_CONFIG_WARNING_LEVEL_FIELD, fifoConfig->overflowWarningLevel) |
		MAX40080_SET_FIELD(MAX40080_FIFO_CONFIG_ROLL_OVER_FIELD, fifoConfig->rollOverMode);

	status = MAX40080_WriteRegister16(MAX40080_REG_FIFO_CONFIG, fifoConfigRegVal);
	if (status) {
		return status;
	}

	MAX40080_CurrentFifoConfig.storingMode = fifoConfig->storingMode;
	MAX40080_CurrentFifoConfig.overflowWarningLevel = fifoConfig->overflowWarningLevel;
	MAX40080_CurrentFifoConfig.rollOverMode = fifoConfig->rollOverMode;

	return MAX40080_Status_Ok;
}

MAX40080_Status MAX40080_FlushFifo() {
	MAX40080_Status status;
	uint16_t fifoConfigRegVal;
	uint16_t flushFifoRegVal;

	status = MAX40080_ReadRegister16(MAX40080_REG_FIFO_CONFIG, &fifoConfigRegVal);
	if (status) {
		return status;
	}

	flushFifoRegVal = fifoConfigRegVal | MAX40080_SET_FIELD(MAX40080_FIFO_CONFIG_FLUSH_FIELD, 1);

	status = MAX40080_WriteRegister16(MAX40080_REG_FIFO_CONFIG, flushFifoRegVal);
	if (status) {
		return status;
	}

	status = MAX40080_WriteRegister16(MAX40080_REG_FIFO_CONFIG, fifoConfigRegVal);
	if (status) {
		return status;
	}

	return MAX40080_Status_Ok;
}

MAX40080_Status MAX40080_GetChipRevisionId(uint8_t* revisionId) {
	return MAX40080_ReadRegister8(MAX40080_REG_REVISION_ID, revisionId);
}
