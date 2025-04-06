#ifndef TERMINAL_H
#define TERMINAL_H

#include <Arduino.h>
#include "SpiController.h"

// Maximum length of command input
const int MAX_COMMAND_LENGTH = 32;

class Terminal {
public:
    Terminal(SpiController& spi);
    
    // Initialize terminal
    void begin();
    
    // Process incoming serial data
    void processInput();
    
    // Command handlers
    void handleReadCommand(const char* args);
    void handleWriteCommand(const char* args);
    void handleHelpCommand();

private:
    SpiController& spiController;
    char inputBuffer[MAX_COMMAND_LENGTH];
    int inputIndex;
    
    // Helper functions
    void processCommand();
    void printPrompt();
    void printError(const char* message);
    uint16_t parseHexAddress(const char* str, bool& success);
    uint8_t parseHexByte(const char* str, bool& success);
    int parseDecimal(const char* str);
};

#endif // TERMINAL_H 