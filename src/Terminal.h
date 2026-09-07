#ifndef TERMINAL_H
#define TERMINAL_H

#include <Arduino.h>
#include "SpiController.h"
#include "MdcMdioController.h"

// Serial command interface that delegates to the SPI and MDC/MDIO controllers.
class Terminal {
public:
	Terminal(SpiController& spi, MdcMdioController& mdc);

	// Print the firmware name, version, and build time.
	void printBanner();

	// Read the switch and PHY chip IDs and print one OK/FAIL line per chip.
	// Returns true only if every chip answered with its expected ID.
	bool printChipCheck();

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
	void handleStatusCommand();
	void handleSelfTestCommand();
	void handleRebootCommand();
	void handleHangCommand();
	void handleVersionCommand();
	void handleHelpCommand();

private:
	// Maximum length of a command line, including the null terminator.
	static constexpr uint8_t MAX_COMMAND_LENGTH = 32;

	// How many times to re-read a chip ID before declaring the chip missing.
	static constexpr uint8_t ID_CHECK_ATTEMPTS = 3;

	SpiController& spiController;
	MdcMdioController& mdcController;
	char inputBuffer[MAX_COMMAND_LENGTH];
	int inputIndex;

	// Helper functions
	void processCommand();
	void printPrompt();
	void printError(const __FlashStringHelper* message);
	void printHexByte(uint8_t value);
	void printHexWord(uint16_t value);
	void printLinkState(bool linkUp, uint8_t speedCode, bool fullDuplex);
	bool verifySwitch();
	bool verifyPhy(uint8_t phyAddr);
	uint16_t parseHexAddress(const char* str, bool& success);
	uint8_t parseHexByte(const char* str, bool& success);
	int parseDecimal(const char* str, bool& success);
};

#endif // TERMINAL_H
