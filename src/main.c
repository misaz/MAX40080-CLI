#include "CommandLineArguments.h"
#include "MAX40080.h"
#include "MAX40080_PlatformSpecific.h"

#include <stdio.h>

CommandLineArguments cliArgs;

float shuntReistorVariable = 0.010;

static const char* StatusToString(MAX40080_Status status) {
	switch (status) {
		case MAX40080_Status_Ok: return "OK";
		case MAX40080_Status_I2CNack: return "I2C Nack Received";
		case MAX40080_Status_I2CError: return "Other I2C Error";
		case MAX40080_Status_I2CTimeout: return "I2C Operation Timed Out";
		case MAX40080_Status_PacketErrorCheckFailed: return "Packet Error Check Failed";
		case MAX40080_Status_NotImplemented: return "Specified operation is not implemented";
		case MAX40080_Status_BadArg: return "Bad Argument";
		case MAX40080_Status_InvalidOperation: return "Invalid Operation";
		case MAX40080_Status_FifoIsEmpty: return "FIFO is empty";
		case MAX40080_Status_NotSupported: return "Specified operation is not supported";
		default: return "Unknown error";
	}
}

static MAX40080_Status collectAndPrintRawCurrent() {
	MAX40080_Status mStatus;
	int16_t current;

	mStatus = MAX40080_ReadRawCurrent(&current);
	if (mStatus == MAX40080_Status_Ok) {
		printf("%d\n", (int)current);
	}
	
	return mStatus;
}

static MAX40080_Status collectAndPrintCurrent() {
	MAX40080_Status mStatus;
	float current;

	mStatus = MAX40080_ReadCurrent(&current);
	if (mStatus == MAX40080_Status_Ok) {
		printf("%.6fA\n", current);
	}

	return mStatus;
}

static MAX40080_Status collectAndPrintRawVoltage() {
	MAX40080_Status mStatus;
	int16_t voltage;

	mStatus = MAX40080_ReadRawVoltage(&voltage);
	if (mStatus == MAX40080_Status_Ok) {
		printf("%d\n", (int)voltage);
	}

	return mStatus;
}

static MAX40080_Status collectAndPrintVoltage() {
	MAX40080_Status mStatus;
	float voltage;

	mStatus = MAX40080_ReadVoltage(&voltage);
	if (mStatus == MAX40080_Status_Ok) {
		printf("%.3fV\n", voltage);
	}

	return mStatus;
}

static MAX40080_Status collectAndPrintRawCurrentAndVoltage() {
	MAX40080_Status mStatus;
	int16_t current;
	int16_t voltage;

	mStatus = MAX40080_ReadRawCurrentAndVoltage(&current, &voltage);
	if (mStatus == MAX40080_Status_Ok) {
		printf("%d; %d\n", (int)current, (int)voltage);
	}

	return mStatus;
}

static MAX40080_Status collectAndPrintCurrentAndVoltage() {
	MAX40080_Status mStatus;
	float current;
	float voltage;

	mStatus = MAX40080_ReadCurrentAndVoltage(&current, &voltage);
	if (mStatus == MAX40080_Status_Ok) {
		printf("%.6fA; %.3fV\n", current, voltage);
	}

	return mStatus;
}

static void collectSample(MAX40080_Status (*sampleProvider) ()) {
	MAX40080_Status mStatus;
	int emptyFifoRetries = 1000;

	while (emptyFifoRetries--) {
		mStatus = sampleProvider();

		if (mStatus == MAX40080_Status_Ok) {
			return;
		}

		if (mStatus == MAX40080_Status_FifoIsEmpty) {
			// we wont print this error because it is allowed in some configurations.
			continue;
		}

		const char* retryAgainSentense = "Trying again. ";
		if (emptyFifoRetries == 0) {
			retryAgainSentense = "";
		}

		fprintf(stderr, "WARNING: Collecting sample failed with error. %sError details: %s\n", retryAgainSentense, StatusToString(mStatus));
	}

	fprintf(stderr, "Collecting sample failed. Giving up.\n");
}

int main(int argc, char** argv) {
	int iStatus;
	MAX40080_Status mStatus;

	iStatus = CommandLineArguments_Parse(&cliArgs, argc, argv);
	if (iStatus) {
		return 1;
	}

	if (cliArgs.isI2CAddressSet || cliArgs.isBoardSet) {
		// following call always returns ok. Error checking is not needed.
		MAX40080_PlatformSpecific_SetI2CAddress(cliArgs.i2CAddress);
	}

	if (cliArgs.isShuntResistorSet || cliArgs.isBoardSet) {
		shuntReistorVariable = cliArgs.shuntResistor;
	}

	if (cliArgs.isI2CControllerSet) {
		// following call always returns ok. Error checking is not needed.
		MAX40080_PlatformSpecific_SetI2CControllerNo(cliArgs.i2CController);
	}

	mStatus = MAX40080_Init();
	if (mStatus) {
		fprintf(stderr, "Sensor initialization failed. Details: %s\n", StatusToString(mStatus));
		return 1;
	}

	MAX40080_Configuration config;
	MAX40080_GetDefaultConfiguration(&config);
	config.operatingMode = MAX40080_OperationMode_Active;

	if (cliArgs.isSampleRateSet) {
		config.adcSampleRate = (MAX40080_AdcSampleRate)cliArgs.sampleRate;
	} else if (cliArgs.isVariableSet && cliArgs.variable == 'B') {
		config.adcSampleRate = MAX40080_AdcSampleRate_Both_at_0_5_ksps;
	}

	if (cliArgs.isAveragingSet) {
		config.digitalFilter = (MAX40080_DigitalFilter)cliArgs.averaging;
	}

	mStatus = MAX40080_SetConfiguration(&config);
	if (mStatus) {
		fprintf(stderr, "Setting sensor configuration failed. Details: %s\n", StatusToString(mStatus));
		return 1;
	}

	MAX40080_FifoConfiguration fifoConfig;
	MAX40080_GetFifoDefaultConfiguration(&fifoConfig);
	
	// rollover is better in this use case but there are some bugs in some configurations
	//fifoConfig.rollOverMode = MAX40080_FifoRollOverMode_OverwriteFirst;

	if (cliArgs.isVariableSet) {
		switch (cliArgs.variable) {
			case 'I':
				fifoConfig.storingMode = MAX40080_FifoStoringMode_CurrentOnly;
				break;
			case 'V':
				fifoConfig.storingMode = MAX40080_FifoStoringMode_VoltageOnly;
				break;
			case 'B':
				fifoConfig.storingMode = MAX40080_FifoStoringMode_CurrentAndVoltage;
				break;
		}
	}

	mStatus = MAX40080_SetFifoConfiguration(&fifoConfig);
	if (mStatus) {
		fprintf(stderr, "Setting sensor FIFO configuration failed. Details: %s\n", StatusToString(mStatus));
		return 1;
	}

	mStatus = MAX40080_FlushFifo();
	if (mStatus) {
		fprintf(stderr, "Flushing sensor FIFO failed. Details: %s\n", StatusToString(mStatus));
		return 1;
	}

	// 0xFF means all interrupt flags. It is OR over all MAX40080_Interrupt possible values
	mStatus = MAX40080_ClearPendingInterrupts(0xFF);
	if (mStatus) {
		fprintf(stderr, "Error while . Details: %s\n", StatusToString(mStatus));
		return 1;
	}

	int remainingSamples = cliArgs.count;
	while (remainingSamples > 0 || remainingSamples == -1) {
		if (!cliArgs.isVariableSet || cliArgs.variable == 'I') {
			if (cliArgs.isRawOutputSet) {
				collectSample(collectAndPrintRawCurrent);
			} else {
				collectSample(collectAndPrintCurrent);
			}
		} else if (cliArgs.variable == 'V') {
			if (cliArgs.isRawOutputSet) {
				collectSample(collectAndPrintRawVoltage);
			} else {
				collectSample(collectAndPrintVoltage);
			}
		} else if (cliArgs.variable == 'B') {
			if (cliArgs.isRawOutputSet) {
				collectSample(collectAndPrintRawCurrentAndVoltage);
			} else {
				collectSample(collectAndPrintCurrentAndVoltage);
			}
		} else {
			fprintf(stderr, "Internal error at %s:%d. Please report this bug.\n", __FILE__, __LINE__);
			return 1;
		}

		if (remainingSamples != -1) {
			remainingSamples--;
		}
	}

	config.operatingMode = MAX40080_OperationMode_Standby;
	MAX40080_SetConfiguration(&config);

	MAX40080_Interrupt pendingInterrupts;
	mStatus = MAX40080_GetPendingInterrupts(&pendingInterrupts);
	if (mStatus) {
		fprintf(stderr, "Error while reading sensor pending interrupts. Details: %s\n", StatusToString(mStatus));
		return 1;
	}

	if (pendingInterrupts & MAX40080_Interrupt_FifoOverflown) {
		fprintf(stderr, "WARNING: FIFO overflown while reading samples. Some samples were missing. Try reduce sample rate and/or increase I2C clock frequency and/or increase averaging value.\n");
	}

	return 0;
}