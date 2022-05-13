#include "CommandLineArguments.h"

#include <argp.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

static error_t CommandLineArgumets_ParseOption(int key, char* arg, struct argp_state* state);

const char* argp_program_version = "MAX40080 Utility 1.0";
const char* argp_program_bug_address = "<max40080-util@misaz.cz>";
static char doc[] = "Utility for controllling MAX40080 sensor connected to the I2C bus.";
static char args_doc[] = "command";

#define CLI_BOARD_SELECTION_PARAM   	'b'
#define CLI_I2C_CONTROLLER_PARAM        'c'
#define CLI_I2C_ADDRESS_PARAM           'a'
#define CLI_SHUNT_PARAM                 'r'
#define CLI_VARIABLE_PARAM              'v'
#define CLI_SAMPLE_RATE_PARAM           's'
#define CLI_AVERAGING_PARAM             'f'
#define CLI_COUNT_PARAM                 'n'
#define CLI_OUTPUT_RAW_PARAM            'w'

static struct argp_option options[] = {
	{ "board", CLI_BOARD_SELECTION_PARAM, "BOARD", 0, "Specify board. Supported boards are 'mikroe-current-6-click' and 'MAX40080EVSYS'.", 1},
	{ "i2c-controler", CLI_I2C_CONTROLLER_PARAM, "N", 0, "Specify I2C controller number. Device /dev/i2c-N will be used where N is value of this parameter", 2},
	{ "i2c-address", CLI_I2C_ADDRESS_PARAM, "HEX", 0, "Specify I2C address of MAX40080 device. Enter 7-bit address as a 2 digit HEX number with no prefix.", 3},
	{ "shunt", CLI_SHUNT_PARAM, "FLOAT", 0, "Specify resistance of shunt resistor used for current sensing (float value in ohms).", 4},
	{ "variable", CLI_VARIABLE_PARAM, "current|voltage|both", 0, "Specify variable to meassure.", 5},
	{ "sample-rate", CLI_SAMPLE_RATE_PARAM, "FLOAT", 0, "Specify sample rate in kHz.", 6},
	{ "averaging", CLI_AVERAGING_PARAM, "N", 0, "Specify number of averaged samples.", 7},
	{ "count", CLI_COUNT_PARAM, "N", 0, "Specify number of continously collected samples.", 8},
	{ "raw", CLI_OUTPUT_RAW_PARAM, NULL, 0, "Output raw values received from sensor.", 9},
	{ 0 }
};

static struct argp argp = { options, CommandLineArgumets_ParseOption, args_doc, doc, 0, 0, 0 };

static error_t CommandLineArgumets_ParseOption(int key, char* arg, struct argp_state* state) {
	CommandLineArguments* cliArgs = (CommandLineArguments*)state->input;

	if (key == CLI_BOARD_SELECTION_PARAM) {
		if (cliArgs->isShuntResistorSet) {
			argp_error(state, "Specifying board is not allowed when shunt resistor value was manually specified.");
		}
		if (cliArgs->isI2CAddressSet) {
			argp_error(state, "Specifying board is not allowed when I2C address was manually specified.");
		}

		int isClickBoard = strcmp(arg, "mikroe-current-6-click") == 0;
		int isMaxEvsys = strcmp(arg, "MAX40080EVSYS") == 0;

		if (!isClickBoard && !isMaxEvsys) {
			argp_error(state, "Unknown board '%s'. Supported boards are 'mikroe-current-6-click' and 'MAX40080EVSYS'", arg);
		}

		if (isClickBoard) {
			cliArgs->i2CAddress = 0x21;
			cliArgs->shuntResistor = 0.010;
		} else if (isMaxEvsys) {
			cliArgs->i2CAddress = 0x21;
			cliArgs->shuntResistor = 0.050;
		}

		cliArgs->isBoardSet = 1;

		return 0;
	}

	if (key == CLI_I2C_CONTROLLER_PARAM) {
		int pos;
		if (sscanf(arg, " %d%n", &cliArgs->i2CController, &pos) != 1 || pos != strlen(arg)) {
			argp_error(state, "Invalid I2C Controller value.");
		}

		cliArgs->isI2CControllerSet = 1;

		return 0;
	}

	if (key == CLI_I2C_ADDRESS_PARAM) {
		char hexUpper;
		char hexLower;

		if (cliArgs->isBoardSet) {
			argp_error(state, "Specifying I2C address is not allowed when board was manually specified.");
		}

		int pos;
		if (sscanf(arg, " %c%c%n", &hexUpper, &hexLower, &pos) != 2 || pos != strlen(arg)) {
			argp_error(state, "Invalid I2C address value");
		}

		uint8_t addr = 0;
		if (hexUpper >= 'A' && hexUpper <= 'F') {
			addr = hexUpper - 'A' + 10;
		} else if (hexUpper >= 'a' && hexUpper <= 'f') {
			addr = hexUpper - 'a' + 10;
		} else if (hexUpper >= '0' && hexUpper <= '9') {
			addr = hexUpper - '0';
		} else {
			argp_error(state, "Invalid I2C address value. I2C address must be entered as 2 digit hex value without any prefix.");
		}
		addr <<= 4;

		if (hexLower >= 'A' && hexLower <= 'F') {
			addr |= hexLower - 'A' + 10;
		} else if (hexLower >= 'a' && hexLower <= 'f') {
			addr |= hexLower - 'a' + 10;
		} else if (hexLower >= '0' && hexLower <= '9') {
			addr |= hexLower - '0';
		} else {
			argp_error(state, "Invalid I2C address value. I2C address must be entered as 2 digit hex value without any prefix.");
		}

		cliArgs->i2CAddress = addr;
		cliArgs->isI2CAddressSet = 1;

		return 0;
	}

	if (key == CLI_SHUNT_PARAM) {
		int pos;

		if (sscanf(arg, " %f%n", &cliArgs->shuntResistor, &pos) != 1 || pos != strlen(arg)) {
			argp_error(state, "Invalid shunt resistor value.");
		}

		cliArgs->isShuntResistorSet = 1;

		return 0;
	}

	if (key == CLI_SHUNT_PARAM) {
		int pos;

		if (cliArgs->isBoardSet) {
			argp_error(state, "Specifying shunt resistor value is not allowed when board was manually specified.");
		}

		if (sscanf(arg, " %f%n", &cliArgs->shuntResistor, &pos) != 1 || pos != strlen(arg)) {
			argp_error(state, "Invalid shunt resistor value.");
		}

		cliArgs->isShuntResistorSet = 1;

		return 0;
	}

	if (key == CLI_VARIABLE_PARAM) {
		int isCurrent = strcmp(arg, "current") == 0;
		int isVoltage = strcmp(arg, "voltage") == 0;
		int isBoth = strcmp(arg, "both") == 0;

		if (isCurrent) {
			cliArgs->variable = 'I';
		} else if (isVoltage) {
			cliArgs->variable = 'V';
		} else if (isBoth) {
			if (cliArgs->isSampleRateSet && cliArgs->sampleRate != 15) {
				argp_error(state, "Sample rate must be 0.5 when measurement of both current and voltage is selected.");
			}

			cliArgs->variable = 'B';
		} else {
			argp_error(state, "Invalid variable value. Allowed values are current, voltage and both.");
		}

		cliArgs->isVariableSet = 1;

		return 0;
	}

	if (key == CLI_SAMPLE_RATE_PARAM) {
		float availableSampleRates[] = { 15, 18.75, 23.45, 30, 37.5, 47.1, 60, 93.5, 120, 150, 234.5, 375, 468.5, 750, 1000, 0.5 };
		size_t availableSampleRatesCount = sizeof(availableSampleRates) / sizeof(float);

		float sampleRate;
		int pos;

		if (sscanf(arg, " %f%n", &sampleRate, &pos) != 1 || pos != strlen(arg)) {
			argp_error(state, "Invalid sample rate value.");
		}

		for (size_t i = 0; i < availableSampleRatesCount; i++) {
			float diff = sampleRate - availableSampleRates[i];
			if (diff < 0.01 && diff > -0.01) {
				if (i != 15 && cliArgs->isVariableSet && cliArgs->variable == 'B') {
					argp_error(state, "You cant measure at specified sample rate when measurement of both current and voltage is selected. Allowed sample rate in this configuration is 0.5 kHz.");
				}

				cliArgs->sampleRate = i;
				cliArgs->isSampleRateSet = 1;

				return 0;
			}
		}

		argp_error(state, "Invalid sample rate. Allowed sample rates are 15, 18.75, 23.45, 30, 37.5, 47.1, 60, 93.5, 120, 150, 234.5, 375, 468.5, 750, 1000 and 0.5.");
	}

	if (key == CLI_AVERAGING_PARAM) {
		int availableAverangingModes[] = { 1, 8, 16, 32, 64, 128 };
		size_t availableAverangingModesCount = sizeof(availableAverangingModes) / sizeof(float);

		int averaging;
		int pos;

		if (sscanf(arg, " %d%n", &averaging, &pos) != 1 || pos != strlen(arg)) {
			argp_error(state, "Invalid averaging value.");
		}

		for (size_t i = 0; i < availableAverangingModesCount; i++) {
			if (averaging == availableAverangingModes[i]) {
				cliArgs->averaging = i;
				cliArgs->isAveragingSet = 1;

				return 0;
			}
		}

		argp_error(state, "Invalid averaging value. Allowed averaging modes are 1 (no averaging), 8, 16, 32, 64 and 128.");
	}

	if (key == CLI_COUNT_PARAM) {
		int pos;

		if (sscanf(arg, " %d%n", &cliArgs->count, &pos) != 1 || pos != strlen(arg)) {
			argp_error(state, "Invalid samples count value.");
		}

		// -1 means indefinitely
		if (cliArgs->count < -1 || cliArgs->count == 0) {
			argp_error(state, "Invalid samples count value.");
		}

		return 0;
	}

	if (key == CLI_OUTPUT_RAW_PARAM) {
		cliArgs->isRawOutputSet = 1;

		return 0;
	}

	// ARGP_KEY_ARG

	return ARGP_ERR_UNKNOWN;
}

int CommandLineArguments_Parse(CommandLineArguments* output, int argc, char** argv) {
	error_t status;

	output->isBoardSet = 0;
	output->isI2CAddressSet = 0;
	output->isI2CControllerSet = 0;
	output->isSampleRateSet = 0;
	output->isVariableSet = 0;
	output->isAveragingSet = 0;
	output->isCountSet = 0;
	output->isRawOutputSet = 0;
	output->count = 1;

	status = argp_parse(&argp, argc, argv, 0, 0, output);
	if (status) {

	}


	return 0;
}
