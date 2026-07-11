#ifndef TERMINAL_H
#define TERMINAL_H

#include <Arduino.h>
#include "SpiController.h"
#include "MdcMdioController.h"

// Serial command interface that delegates to the SPI and MDC/MDIO controllers.
class Terminal {
public:
	Terminal(SpiController& spi, MdcMdioController& mdc);

	// Initialize terminal
	void begin();

	// Process incoming serial data
	void processInput();

	// Command handlers
	void handleReadCommand(const char* args);
	void handleWriteCommand(const char* args);
	void handleReadMdcCommand(const char* args);
	void handleWriteMdcCommand(const char* args);
	void handleScanMdcCommand(const char* args);
	void handleHelpCommand();

private:
	// Maximum length of a command line, including the null terminator.
	static constexpr uint8_t MAX_COMMAND_LENGTH = 32;

	SpiController& spiController;
	MdcMdioController& mdcController;
	char inputBuffer[MAX_COMMAND_LENGTH];
	int inputIndex;

	// Helper functions
	void processCommand();
	void printPrompt();
	void printError(const char* message);
	void printHexByte(uint8_t value);
	void printHexWord(uint16_t value);
	uint16_t parseHexAddress(const char* str, bool& success);
	uint8_t parseHexByte(const char* str, bool& success);
	int parseDecimal(const char* str, bool& success);
};

#endif // TERMINAL_H
