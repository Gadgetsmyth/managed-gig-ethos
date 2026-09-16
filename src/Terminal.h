#ifndef TERMINAL_H
#define TERMINAL_H

#include <Arduino.h>
#include "SpiController.h"
#include "MdcMdioController.h"
#include "Settings.h"

// Serial command interface. Owns the console formatting and applies the running
// configuration to the switch and PHYs through the two controllers.
class Terminal {
public:
	// One console command: its name and the handler that parses its arguments.
	struct Command {
		char name[9];
		void (Terminal::*handler)(const char* args);
	};

	Terminal(SpiController& spi, MdcMdioController& mdc, Settings& settings);

	// Print the firmware name, version, and build time.
	void printBanner();

	// Read the switch and PHY chip IDs and print one OK/FAIL line per chip.
	// Returns true only if every chip answered with its expected ID.
	bool printChipCheck();

	// Push the running configuration to the hardware at boot. Ports that are enabled are
	// left alone so they are not forced through a second autonegotiation.
	void applySettingsAtBoot();

	// Print a link change on its own line without losing a partly typed command.
	void printLinkEvent(uint8_t port, bool linkUp, uint8_t speedCode, bool fullDuplex);

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
	void handleStatusCommand(const char* args);
	void handlePortCommand(const char* args);
	void handleSpeedCommand(const char* args);
	void handleIsolateCommand(const char* args);
	void handleMirrorCommand(const char* args);
	void handleCountersCommand(const char* args);
	void handleLogCommand(const char* args);
	void handleRgmiiCommand(const char* args);
	void handleShowCommand(const char* args);
	void handleSaveCommand(const char* args);
	void handleDefaultsCommand(const char* args);
	void handleSelfTestCommand(const char* args);
	void handleRebootCommand(const char* args);
	void handleHangCommand(const char* args);
	void handleVersionCommand(const char* args);
	void handleHelpCommand(const char* args);

private:
	// Maximum length of a command line, including the null terminator.
	static constexpr uint8_t MAX_COMMAND_LENGTH = 32;

	// How many times to re-read a chip ID before declaring the chip missing.
	static constexpr uint8_t ID_CHECK_ATTEMPTS = 3;

	SpiController& spiController;
	MdcMdioController& mdcController;
	Settings& settings;
	char inputBuffer[MAX_COMMAND_LENGTH];
	int inputIndex;
	bool lastCharWasCr;

	// Helper functions
	void processCommand();
	void printPrompt();
	void printError(const __FlashStringHelper* message);
	void printHexByte(uint8_t value);
	void printHexWord(uint16_t value);
	void printLinkState(bool linkUp, uint8_t speedCode, bool fullDuplex, bool columns);
	void printPortList(uint8_t mask);
	void printSpeedSetting(uint8_t speed);
	void printMirrorSetting();
	bool verifySwitch();
	bool verifyPhy(uint8_t phyAddr);

	// Hardware side of the configuration commands.
	void setPortEnabled(uint8_t port, bool enabled);
	void setPortSpeed(uint8_t port, uint8_t speed);
	void applyMirror();

	uint16_t parseHexAddress(const char* str, bool& success);
	uint8_t parseHexByte(const char* str, bool& success);
	int parseDecimal(const char* str, bool& success);
	bool parsePort(const char* str, uint8_t& port);
	bool parsePortList(const char* str, uint8_t& mask);
	static const char* nextArg(const char* args);
	static bool matchWord(const char* args, PGM_P word);
};

#endif // TERMINAL_H
