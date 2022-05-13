#ifndef __MAX40080_H
#define __MAX40080_H

#include <stdint.h>

// default 7-bit I2C address 0x21 is valid when A0 pin is connected using 100k resistor to ground. 
// If you use different resistor, edit address here or use -D compiler parameter.
#ifndef MAX40080_I2C_7BIT_ADDRESS
#define MAX40080_I2C_7BIT_ADDRESS					0x21
#endif

extern float shuntReistorVariable;
#define MAX40080_SHUNT_RESISOTR_VALUE				shuntReistorVariable

#define MAX40080_I2C_READ_ADDRESS					((MAX40080_I2C_7BIT_ADDRESS << 1) | 0x01)
#define MAX40080_I2C_WRITE_ADDRESS					((MAX40080_I2C_7BIT_ADDRESS << 1) | 0x00)

#define MAX40080_REG_CONFIG							0x00
#define MAX40080_REG_STATUS							0x02
#define MAX40080_REG_TRESHOLD_OVER_CURRENT			0x04
#define MAX40080_REG_TRESHOLD_OVER_VOLTAGE			0x05
#define MAX40080_REG_TRESHOLD_UNDER_VOLTAGE			0x06
#define MAX40080_REG_WAKE_UP_CURRENT				0x07
#define MAX40080_REG_MAX_PEAK_CURRENT				0x08
#define MAX40080_REG_FIFO_CONFIG					0x0A
#define MAX40080_REG_CURRENT_MEASUREMENT			0x0C
#define MAX40080_REG_VOLTAGE_MEASUREMENT			0x0E
#define MAX40080_REG_CURRENT_VOLTAGE_MEASUREMENT	0x10
#define MAX40080_REG_INTERRUPT_ENABLE				0x14
#define MAX40080_REG_REVISION_ID					0x15

#define MAX40080_EXPAND_MACRO(content)				content
#define MAX40080_FIELD(NAME)						NAME##_MASK, NAME##_OFFSET
#define MAX40080_GET_FIELD_IMPL(FIELD_MASK,FIELD_OFFSET,value)	(((value) >> FIELD_OFFSET) & FIELD_MASK)
#define MAX40080_SET_FIELD_IMPL(FIELD_MASK,FIELD_OFFSET,value)	(((value) & FIELD_MASK) << FIELD_OFFSET)
#define MAX40080_GET_FIELD(...)						MAX40080_EXPAND_MACRO(MAX40080_GET_FIELD_IMPL(__VA_ARGS__))
#define MAX40080_SET_FIELD(...)						MAX40080_EXPAND_MACRO(MAX40080_SET_FIELD_IMPL(__VA_ARGS__))

#define MAX40080_CONFIG_MODE_MASK					0x7
#define MAX40080_CONFIG_MODE_OFFSET					0
#define MAX40080_CONFIG_MODE_FIELD					MAX40080_FIELD(MAX40080_CONFIG_MODE)
#define MAX40080_CONFIG_I2C_TIMEOUT_MASK			0x1
#define MAX40080_CONFIG_I2C_TIMEOUT_OFFSET			3
#define MAX40080_CONFIG_I2C_TIMEOUT_FIELD			MAX40080_FIELD(MAX40080_CONFIG_I2C_TIMEOUT)
#define MAX40080_CONFIG_ALERT_MASK					0x1
#define MAX40080_CONFIG_ALERT_OFFSET				4
#define MAX40080_CONFIG_ALERT_FIELD					MAX40080_FIELD(MAX40080_CONFIG_ALERT)
#define MAX40080_CONFIG_PEC_MASK					0x1
#define MAX40080_CONFIG_PEC_OFFSET					5
#define MAX40080_CONFIG_PEC_FIELD					MAX40080_FIELD(MAX40080_CONFIG_PEC)
#define MAX40080_CONFIG_INPUT_RANGE_MASK			0x1
#define MAX40080_CONFIG_INPUT_RANGE_OFFSET			6
#define MAX40080_CONFIG_INPUT_RANGE_FIELD			MAX40080_FIELD(MAX40080_CONFIG_INPUT_RANGE)
#define MAX40080_CONFIG_STAY_HS_MODE_MASK			0x1
#define MAX40080_CONFIG_STAY_HS_MODE_OFFSET			7
#define MAX40080_CONFIG_STAY_HS_MODE_FIELD			MAX40080_FIELD(MAX40080_CONFIG_STAY_HS_MODE)
#define MAX40080_CONFIG_ADC_SAMPLE_RATE_MASK		0xF
#define MAX40080_CONFIG_ADC_SAMPLE_RATE_OFFSET		8
#define MAX40080_CONFIG_ADC_SAMPLE_RATE_FIELD		MAX40080_FIELD(MAX40080_CONFIG_ADC_SAMPLE_RATE)
#define MAX40080_CONFIG_DIGITAL_FILTER_MASK			0x7
#define MAX40080_CONFIG_DIGITAL_FILTER_OFFSET		12
#define MAX40080_CONFIG_DIGITAL_FILTER_FIELD		MAX40080_FIELD(MAX40080_CONFIG_DIGITAL_FILTER)

#define MAX40080_STATUS_FIFO_DATA_COUNT_MASK		0x3F
#define MAX40080_STATUS_FIFO_DATA_COUNT_OFFSET		8
#define MAX40080_STATUS_FIFO_DATA_COUNT_FIELD		MAX40080_FIELD(MAX40080_STATUS_FIFO_DATA_COUNT)

#define MAX40080_FIFO_CONFIG_STORING_MODE_MASK		0x3
#define MAX40080_FIFO_CONFIG_STORING_MODE_OFFSET	0
#define MAX40080_FIFO_CONFIG_STORING_MODE_FIELD		MAX40080_FIELD(MAX40080_FIFO_CONFIG_STORING_MODE) 
#define MAX40080_FIFO_CONFIG_WARNING_LEVEL_MASK		0x3F
#define MAX40080_FIFO_CONFIG_WARNING_LEVEL_OFFSET	8
#define MAX40080_FIFO_CONFIG_WARNING_LEVEL_FIELD	MAX40080_FIELD(MAX40080_FIFO_CONFIG_WARNING_LEVEL) 
#define MAX40080_FIFO_CONFIG_ROLL_OVER_MASK			0x1
#define MAX40080_FIFO_CONFIG_ROLL_OVER_OFFSET		14
#define MAX40080_FIFO_CONFIG_ROLL_OVER_FIELD		MAX40080_FIELD(MAX40080_FIFO_CONFIG_ROLL_OVER) 
#define MAX40080_FIFO_CONFIG_FLUSH_MASK				0x1
#define MAX40080_FIFO_CONFIG_FLUSH_OFFSET			15
#define MAX40080_FIFO_CONFIG_FLUSH_FIELD			MAX40080_FIELD(MAX40080_FIFO_CONFIG_FLUSH) 

#define MAX40080_FIFO_CAPACITY						64

typedef enum {
	MAX40080_Status_Ok = 0,
	MAX40080_Status_I2CNack = -10,
	MAX40080_Status_I2CError = -11,
	MAX40080_Status_I2CTimeout = -12,
	MAX40080_Status_PacketErrorCheckFailed = -13,
	MAX40080_Status_NotImplemented = -20,
	MAX40080_Status_BadArg = -21,
	MAX40080_Status_InvalidOperation = -22,
	MAX40080_Status_FifoIsEmpty = -23,
	MAX40080_Status_NotSupported = -24,
} MAX40080_Status;

typedef enum {
	MAX40080_OperationMode_Standby = 0,
	MAX40080_OperationMode_LowPower = 1,
	MAX40080_OperationMode_SingleConversion = 2,
	MAX40080_OperationMode_Active = 3,
	MAX40080_OperationMode_SelectedActive_4_SPS = 4,
	MAX40080_OperationMode_SelectedActive_1_SPS = 5,
	MAX40080_OperationMode_SelectedActive_0_25_SPS = 6,
	MAX40080_OperationMode_SelectedActive_0_0625_SPS = 7
} MAX40080_OperationMode;

typedef enum {
	MAX40080_I2CTimeoutSettings_Enabled = 0,
	MAX40080_I2CTimeoutSettings_Disabled = 1,
} MAX40080_I2CTimeoutSettings;

typedef enum {
	MAX40080_AlertResponseTime_Unfiltered = 0,
	MAX40080_AlertResponseTime_FourConsecutive = 1,
} MAX40080_AlertResponseTime;

typedef enum {
	MAX40080_PacketErrorChecking_Disabled = 0,
	MAX40080_PacketErrorChecking_Enabled = 1,
} MAX40080_PacketErrorChecking;

typedef enum {
	MAX40080_InputRange_50mV = 0,
	MAX40080_InputRange_10mV = 1,
} MAX40080_InputRange;

typedef enum {
	MAX40080_StayHsMode_ExitAtStop = 0,
	MAX40080_StayHsMode_Stay = 1,
} MAX40080_StayHsMode;

typedef enum {
	MAX40080_AdcSampleRate_Either_at_15_ksps = 0,
	MAX40080_AdcSampleRate_Either_at_18_75_ksps = 1,
	MAX40080_AdcSampleRate_Either_at_23_45_ksps = 2,
	MAX40080_AdcSampleRate_Either_at_30_ksps = 3,
	MAX40080_AdcSampleRate_Either_at_37_5_ksps = 4,
	MAX40080_AdcSampleRate_Either_at_47_1_ksps = 5,
	MAX40080_AdcSampleRate_Either_at_60_ksps = 6,
	MAX40080_AdcSampleRate_Either_at_93_5_ksps = 7,
	MAX40080_AdcSampleRate_Either_at_120_ksps = 8,
	MAX40080_AdcSampleRate_Either_at_150_ksps = 9,
	MAX40080_AdcSampleRate_Either_at_234_5_ksps = 10,
	MAX40080_AdcSampleRate_Either_at_375_ksps = 11,
	MAX40080_AdcSampleRate_Either_at_468_5_ksps = 12,
	MAX40080_AdcSampleRate_Either_at_750_ksps = 13,
	MAX40080_AdcSampleRate_Either_at_1000_ksps = 14,
	MAX40080_AdcSampleRate_Both_at_0_5_ksps= 15,
} MAX40080_AdcSampleRate;

typedef enum {
	MAX40080_DigitalFilter_NoAverage = 0,
	MAX40080_DigitalFilter_8_samples = 1,
	MAX40080_DigitalFilter_16_samples = 2,
	MAX40080_DigitalFilter_32_samples = 3,
	MAX40080_DigitalFilter_64_samples = 4,
	MAX40080_DigitalFilter_128_samples = 5
} MAX40080_DigitalFilter;

typedef enum {
	MAX40080_FifoStoringMode_CurrentOnly = 0,
	MAX40080_FifoStoringMode_VoltageOnly = 1,
	MAX40080_FifoStoringMode_CurrentAndVoltage = 2
} MAX40080_FifoStoringMode;

typedef enum {
	MAX40080_FifoRollOverMode_Ignore = 0,
	MAX40080_FifoRollOverMode_OverwriteFirst = 1
} MAX40080_FifoRollOverMode;

typedef enum {
	MAX40080_Interrupt_WakeUp = 1,
	MAX40080_Interrupt_ConversionReady = 2,
	MAX40080_Interrupt_CurrentOverflow = 4,
	MAX40080_Interrupt_VoltageOverflow = 8,
	MAX40080_Interrupt_VoltageUnderflow = 16,
	MAX40080_Interrupt_I2CInactive = 32,
	MAX40080_Interrupt_FifoOverflowWarning = 64,
	MAX40080_Interrupt_FifoOverflown = 128
} MAX40080_Interrupt;

typedef enum {
	MAX40080_I2CSpeed_100_kHz = 100,
	MAX40080_I2CSpeed_400_kHz = 400,
	MAX40080_I2CSpeed_1_MHz = 1000,
	MAX40080_I2CSpeed_3_4_MHz = 3400,
} MAX40080_I2CSpeed;

typedef struct {
	MAX40080_OperationMode operatingMode;
	MAX40080_I2CTimeoutSettings i2cTimeoutSettings;
	MAX40080_AlertResponseTime alertResponseTime;
	MAX40080_PacketErrorChecking packetErrorChecking;
	MAX40080_InputRange inputRange;
	MAX40080_StayHsMode stayHsMode;
	MAX40080_AdcSampleRate adcSampleRate;
	MAX40080_DigitalFilter digitalFilter;
} MAX40080_Configuration;

typedef struct {
	MAX40080_FifoStoringMode storingMode;
	MAX40080_FifoRollOverMode rollOverMode;
	uint8_t overflowWarningLevel;
} MAX40080_FifoConfiguration;

MAX40080_Status MAX40080_Init();
MAX40080_Status MAX40080_Deinit();

MAX40080_Status MAX40080_GetDefaultConfiguration(MAX40080_Configuration* config);
MAX40080_Status MAX40080_GetConfiguration(MAX40080_Configuration* config);
MAX40080_Status MAX40080_SetConfiguration(MAX40080_Configuration* config);

MAX40080_Status MAX40080_EnableInterrupts(MAX40080_Interrupt interrupts);
MAX40080_Status MAX40080_GetPendingInterrupts(MAX40080_Interrupt* interrupts);
MAX40080_Status MAX40080_ClearPendingInterrupts(MAX40080_Interrupt interrupts);

MAX40080_Status MAX40080_GetRawOverCurrentTreshold(uint8_t* treshold);
MAX40080_Status MAX40080_SetRawOverCurrentTreshold(uint8_t treshold);
MAX40080_Status MAX40080_GetRawOverVoltageTreshold(uint8_t* treshold);
MAX40080_Status MAX40080_SetRawOverVoltageTreshold(uint8_t treshold);
MAX40080_Status MAX40080_GetRawUnderVoltageTreshold(uint8_t* treshold);
MAX40080_Status MAX40080_SetRawUnderVoltageTreshold(uint8_t treshold);
MAX40080_Status MAX40080_GetRawWakeUpCurrent(uint8_t* wakeUpCurrent);
MAX40080_Status MAX40080_SetRawWakeUpCurrent(uint8_t wakeUpCurrent);
MAX40080_Status MAX40080_GetRawMaxPeakCurrent(uint16_t* maxPeakCurrent);

MAX40080_Status MAX40080_GetFifoDefaultConfiguration(MAX40080_FifoConfiguration* fifoConfig);
MAX40080_Status MAX40080_GetFifoConfiguration(MAX40080_FifoConfiguration* fifoConfig);
MAX40080_Status MAX40080_SetFifoConfiguration(MAX40080_FifoConfiguration* fifoConfig);
MAX40080_Status MAX40080_GetAvailableFifoDataCount(uint8_t* fifoDataCount);
MAX40080_Status MAX40080_FlushFifo();

MAX40080_Status MAX40080_TriggerSingleShootConversion();

MAX40080_Status MAX40080_ReadRawCurrent(int16_t* current);
MAX40080_Status MAX40080_ReadRawVoltage(int16_t* voltage);
MAX40080_Status MAX40080_ReadRawCurrentAndVoltage(int16_t* current, int16_t* voltage);

MAX40080_Status MAX40080_ReadCurrent(float* current);
MAX40080_Status MAX40080_ReadVoltage(float* voltage);
MAX40080_Status MAX40080_ReadCurrentAndVoltage(float* current, float* voltage);

MAX40080_Status MAX40080_GetChipRevisionId(uint8_t* revisionId);

// Use MAX40080_SetConfiguration and MAX40080_GetConfiguration for access following features:
//MAX40080_Status MAX40080_GetOperationMode(MAX40080_OperationMode* opMode);
//MAX40080_Status MAX40080_SetOperationMode(MAX40080_OperationMode opMode);
//MAX40080_Status MAX40080_GetI2CTimeoutSettings(MAX40080_I2CTimeoutSettings* timeoutSettings);
//MAX40080_Status MAX40080_SetI2CTimeoutSettings(MAX40080_I2CTimeoutSettings timeoutSettings);
//MAX40080_Status MAX40080_GetAlertResponseTime(MAX40080_AlertResponseTime* alertResponseTime);
//MAX40080_Status MAX40080_SetAlertResponseTime(MAX40080_AlertResponseTime alertResponseTime);
//MAX40080_Status MAX40080_GetPacketErrorChecking(MAX40080_PacketErrorChecking* pec);
//MAX40080_Status MAX40080_SetPacketErrorChecking(MAX40080_PacketErrorChecking* pec);
//MAX40080_Status MAX40080_GetInputRange(MAX40080_InputRange* inputRange);
//MAX40080_Status MAX40080_SetInputRange(MAX40080_InputRange inputRange);
//MAX40080_Status MAX40080_GetStayHsMode(MAX40080_StayHsMode* stayHsMode);
//MAX40080_Status MAX40080_SetStayHsMode(MAX40080_StayHsMode stayHsMode);
//MAX40080_Status MAX40080_GetAdcSampleRate(MAX40080_AdcSampleRate* sampleRate);
//MAX40080_Status MAX40080_SetAdcSampleRate(MAX40080_AdcSampleRate sampleRate);
//MAX40080_Status MAX40080_GetDigitalFilter(MAX40080_DigitalFilter* digitalFilter);
//MAX40080_Status MAX40080_SetDigitalFilter(MAX40080_DigitalFilter digitalFilter);

// Use MAX40080_GetPendingInterrupts for access following features:
//MAX40080_Status MAX40080_IsWakeUpCurrentReached(int* isWakeUpCurrentReached);
//MAX40080_Status MAX40080_IsConversionReady(int* isConversionReady);
//MAX40080_Status MAX40080_IsOverCurrent(int* isOverCurrent);
//MAX40080_Status MAX40080_IsOverVoltage(int* isOverVoltage);
//MAX40080_Status MAX40080_IsUnderVoltage(int* isUnderVoltage);
//MAX40080_Status MAX40080_IsI2CInactiveForMinute(int* isI2CInactive);
//MAX40080_Status MAX40080_IsFifoOverflowWarningActive(int* isFifoTresholdReached);
//MAX40080_Status MAX40080_IsFifoOverflown(int* isFifoOverflown);

// Use MAX40080_SetFifoConfiguration and MAX40080_GetFifoConfiguration for access following features:
//MAX40080_Status MAX40080_GetFifoStoringMode(MAX40080_FifoStoringMode* storingMode);
//MAX40080_Status MAX40080_SetFifoStoringMode(MAX40080_FifoStoringMode storingMode);
//MAX40080_Status MAX40080_GetFifoOverflowWarningLevel(size_t* fifoOverflowWarningLevel);
//MAX40080_Status MAX40080_SetFifoOverflowWarningLevel(size_t fifoOverflowWarningLevel);
//MAX40080_Status MAX40080_GetFifoRollOverMode(MAX40080_FifoRollOverMode* fifoOverflowWarningLevel);
//MAX40080_Status MAX40080_SetFifoRollOverMode(MAX40080_FifoRollOverMode fifoOverflowWarningLevel);


#endif