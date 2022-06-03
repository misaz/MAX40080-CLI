#ifndef __COMMAND_LINE_ARGUMENTS_H
#define __COMMAND_LINE_ARGUMENTS_H

typedef struct {
    int isBoardSet;
    
    int i2CAddress;
    int isI2CAddressSet;

    int i2CController;
    int isI2CControllerSet;

    int sampleRate;
    int isSampleRateSet;

    char variable;
    int isVariableSet;

    int averaging;
    int isAveragingSet;

    int count;
    int isCountSet;

    int isRawOutputSet;

    float shuntResistor;
    int isShuntResistorSet;

    int inputRange;
    int isInputRangeSet;
} CommandLineArguments;

int CommandLineArguments_Parse(CommandLineArguments* output, int argc, char** argv);

#endif