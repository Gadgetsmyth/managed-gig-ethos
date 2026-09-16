#include "Terminal.h"
#include "Board.h"
#include "Watchdog.h"

// Set from `git describe` by scripts/git_version.py; fallback for other build setups.
#ifndef FW_VERSION
#define FW_VERSION "dev"
#endif

constexpr uint8_t Terminal::MAX_COMMAND_LENGTH;

// Command table in flash. Names are compared against the first word of the input line.
static const Terminal::Command COMMANDS[] PROGMEM = {
	{"status", &Terminal::handleStatusCommand},
	{"port", &Terminal::handlePortCommand},
	{"isolate", &Terminal::handleIsolateCommand},
	{"mirror", &Terminal::handleMirrorCommand},
	{"counters", &Terminal::handleCountersCommand},
	{"log", &Terminal::handleLogCommand},
	{"rgmii", &Terminal::handleRgmiiCommand},
	{"show", &Terminal::handleShowCommand},
	{"save", &Terminal::handleSaveCommand},
	{"defaults", &Terminal::handleDefaultsCommand},
	{"read", &Terminal::handleReadCommand},
	{"write", &Terminal::handleWriteCommand},
	{"readmdc", &Terminal::handleReadMdcCommand},
	{"writemdc", &Terminal::handleWriteMdcCommand},
	{"scanmdc", &Terminal::handleScanMdcCommand},
	{"selftest", &Terminal::handleSelfTestCommand},
	{"reboot", &Terminal::handleRebootCommand},
	{"hang", &Terminal::handleHangCommand},
	{"version", &Terminal::handleVersionCommand},
	{"help", &Terminal::handleHelpCommand},
};
static constexpr uint8_t COMMAND_COUNT = sizeof(COMMANDS) / sizeof(COMMANDS[0]);

// The MIB counters shown by `counters`, in display order. Indexes are from the KSZ9897R
// datasheet table 5-6; 0x80 and 0x81 are the 36-bit byte counters.
struct MibEntry {
	uint8_t index;
	char name[13];
};
static const MibEntry MIB_TABLE[] PROGMEM = {
	{0x80, "RxBytes"},
	{0x0C, "RxUnicast"},
	{0x0B, "RxMulticast"},
	{0x0A, "RxBroadcast"},
	{0x09, "RxPause"},
	{0x06, "RxCrcErr"},
	{0x07, "RxAlignErr"},
	{0x05, "RxSymbolErr"},
	{0x02, "RxFragments"},
	{0x01, "RxUndersize"},
	{0x03, "RxOversize"},
	{0x04, "RxJabbers"},
	{0x82, "RxDropped"},
	{0x81, "TxBytes"},
	{0x1A, "TxUnicast"},
	{0x19, "TxMulticast"},
	{0x18, "TxBroadcast"},
	{0x17, "TxPause"},
	{0x1C, "TxCollisions"},
	{0x16, "TxLateColl"},
	{0x1D, "TxExcessColl"},
	{0x83, "TxDropped"},
};
static constexpr uint8_t MIB_COUNT = sizeof(MIB_TABLE) / sizeof(MIB_TABLE[0]);

Terminal::Terminal(SpiController& spi, MdcMdioController& mdc, Settings& settings)
	: spiController(spi), mdcController(mdc), settings(settings), inputIndex(0),
	  lastCharWasCr(false) {
	memset(inputBuffer, 0, MAX_COMMAND_LENGTH);
}

void Terminal::printBanner() {
	Serial.println(F("managed-gig-ethos " FW_VERSION " built " __DATE__ " " __TIME__));
}

bool Terminal::printChipCheck() {
	bool allOk = verifySwitch();
	for (uint8_t phyAddr : Board::PHY_ADDRESSES)
		allOk &= verifyPhy(phyAddr);
	return allOk;
}

void Terminal::applySettingsAtBoot() {
	for (uint8_t port = 1; port <= Board::PORT_COUNT; port++) {
		if (!(settings.data.portEnabled & Board::portBit(port)))
			setPortEnabled(port, false);
		spiController.setPortMembership(port, settings.data.membership[port - 1]);
	}
	applyMirror();
}

// A disabled port is blocked in the switch fabric and its PHY is powered down, so the
// link partner sees the cable as unplugged. Enabling reverses both.
void Terminal::setPortEnabled(uint8_t port, bool enabled) {
	spiController.setPortForwarding(port, enabled);
	if (Board::isExternalPort(port))
		mdcController.setPowerDown(Board::phyAddressForPort(port), !enabled);
	else
		spiController.setInternalPhyPowerDown(port, !enabled);
}

void Terminal::applyMirror() {
	const Settings::Data& data = settings.data;
	bool active = data.mirrorSource != 0;
	for (uint8_t port = 1; port <= Board::PORT_COUNT; port++) {
		bool source = active && port == data.mirrorSource;
		spiController.setPortMirroring(port, active && port == data.mirrorDest,
			source && (data.mirrorMode & Settings::MIRROR_RX),
			source && (data.mirrorMode & Settings::MIRROR_TX));
	}
}

void Terminal::printLinkEvent(uint8_t port, bool linkUp, uint8_t speedCode, bool fullDuplex) {
	// Blank out the prompt and any typed text with spaces, then return to column 0 so
	// the event replaces the prompt line. Plain characters only: the PlatformIO monitor
	// prints ANSI escape sequences literally. The prompt and typed text are redrawn after.
	Serial.print('\r');
	for (uint8_t i = 0; i < inputIndex + 2; i++)
		Serial.print(' ');
	Serial.print('\r');
	Serial.print(F("link: port "));
	Serial.print(port);
	Serial.print(' ');
	printLinkState(linkUp, speedCode, fullDuplex, false);
	Serial.println();
	printPrompt();
	Serial.print(inputBuffer);
}

void Terminal::begin() {
	Serial.println(F("Type 'help' for available commands."));
	printPrompt();
}

void Terminal::processInput() {
	// Only process if there's data available
	if (Serial.available() > 0) {
		char c = Serial.read();

		// Handle backspace
		if (c == '\b' || c == 127) {
			if (inputIndex > 0) {
				inputIndex--;
				inputBuffer[inputIndex] = '\0';
				Serial.print(F("\b \b"));
			}
			return;
		}

		// A line ends at CR or LF. A LF straight after a CR is the second half of a CR+LF
		// pair and is ignored, so terminals that send either convention get one prompt.
		if (c == '\n' || c == '\r') {
			bool secondHalfOfCrLf = c == '\n' && lastCharWasCr;
			lastCharWasCr = c == '\r';
			if (secondHalfOfCrLf)
				return;
			Serial.println();
			if (inputIndex > 0) {
				processCommand();
			}
			printPrompt();
			return;
		}
		lastCharWasCr = false;

		// Add character to buffer if there's space
		if (inputIndex < MAX_COMMAND_LENGTH - 1) {
			inputBuffer[inputIndex++] = c;
			Serial.print(c);
		}
	}
}

void Terminal::processCommand() {
	// Null-terminate the input
	inputBuffer[inputIndex] = '\0';

	// Find the first space to separate command from arguments
	char* space = strchr(inputBuffer, ' ');
	char* args = nullptr;

	if (space) {
		*space = '\0';
		args = space + 1;
	}

	bool found = false;
	for (uint8_t i = 0; i < COMMAND_COUNT && !found; i++) {
		Command command;
		memcpy_P(&command, &COMMANDS[i], sizeof(command));
		if (strcmp(inputBuffer, command.name) == 0) {
			(this->*command.handler)(args);
			found = true;
		}
	}
	if (!found)
		printError(F("Unknown command. Type 'help' for available commands."));

	// Reset input buffer
	inputIndex = 0;
	memset(inputBuffer, 0, MAX_COMMAND_LENGTH);
}

void Terminal::handleReadCommand(const char* args) {
	if (!args) {
		printError(F("Missing arguments. Usage: read <address> <count>"));
		return;
	}

	// Parse address
	bool parseSuccess;
	uint16_t address = parseHexAddress(args, parseSuccess);
	if (!parseSuccess) {
		printError(F("Invalid address format. Use hex (e.g., 0x01FF)"));
		return;
	}

	// Find count argument
	const char* countStr = nextArg(args);
	if (!countStr) {
		printError(F("Missing count argument. Usage: read <address> <count>"));
		return;
	}

	int count = parseDecimal(countStr, parseSuccess);
	if (count <= 0 || count > 16) {
		printError(F("Invalid count. Must be between 1 and 16"));
		return;
	}

	// Extract port and function from address
	uint8_t port = (address >> 12) & 0x07;
	uint8_t function = (address >> 8) & 0x0F;
	uint8_t registerAddr = address & 0xFF;

	// Read and print the values
	Serial.print(F("Reading "));
	Serial.print(count);
	Serial.print(F(" bytes from address 0x"));
	Serial.println(address, HEX);

	for (int i = 0; i < count; i++) {
		uint8_t value = spiController.readRegister(port, function, registerAddr + i);
		printHexByte(value);
		Serial.print(' ');
	}
	Serial.println();
}

void Terminal::handleWriteCommand(const char* args) {
	if (!args) {
		printError(F("Missing arguments. Usage: write <address> <value>"));
		return;
	}

	// Parse address
	bool parseSuccess;
	uint16_t address = parseHexAddress(args, parseSuccess);
	if (!parseSuccess) {
		printError(F("Invalid address format. Use hex (e.g., 0x01FF)"));
		return;
	}

	// Find value argument
	const char* valueStr = nextArg(args);
	if (!valueStr) {
		printError(F("Missing value argument. Usage: write <address> <value>"));
		return;
	}

	uint8_t value = parseHexByte(valueStr, parseSuccess);
	if (!parseSuccess) {
		printError(F("Invalid value format. Use hex (e.g., 0xC0)"));
		return;
	}

	// Extract port and function from address
	uint8_t port = (address >> 12) & 0x07;
	uint8_t function = (address >> 8) & 0x0F;
	uint8_t registerAddr = address & 0xFF;

	// Write the value
	spiController.writeRegister(port, function, registerAddr, value);
	Serial.print(F("Wrote "));
	printHexByte(value);
	Serial.print(F(" to address 0x"));
	Serial.println(address, HEX);
}

void Terminal::handleReadMdcCommand(const char* args) {
	if (!args) {
		printError(F("Missing arguments. Usage: readmdc <phy_addr> <reg_addr>"));
		return;
	}

	// Parse PHY address
	bool parseSuccess;
	uint8_t phyAddr = parseHexByte(args, parseSuccess);
	if (!parseSuccess) {
		printError(F("Invalid PHY address format. Use hex (e.g., 0x01)"));
		return;
	}

	// Find register address argument
	const char* regStr = nextArg(args);
	if (!regStr) {
		printError(F("Missing register address argument. Usage: readmdc <phy_addr> <reg_addr>"));
		return;
	}

	uint8_t regAddr = parseHexByte(regStr, parseSuccess);
	if (!parseSuccess) {
		printError(F("Invalid register address format. Use hex (e.g., 0x01)"));
		return;
	}

	// Read and print the value
	uint16_t value = mdcController.readRegister(phyAddr, regAddr);
	Serial.print(F("Read from PHY "));
	printHexByte(phyAddr);
	Serial.print(F(" Register "));
	printHexByte(regAddr);
	Serial.print(F(" = "));
	printHexWord(value);
	Serial.println();
}

void Terminal::handleWriteMdcCommand(const char* args) {
	if (!args) {
		printError(F("Missing arguments. Usage: writemdc <phy_addr> <reg_addr> <value>"));
		return;
	}

	// Parse PHY address
	bool parseSuccess;
	uint8_t phyAddr = parseHexByte(args, parseSuccess);
	if (!parseSuccess) {
		printError(F("Invalid PHY address format. Use hex (e.g., 0x01)"));
		return;
	}

	// Find register address argument
	const char* regStr = nextArg(args);
	if (!regStr) {
		printError(
			F("Missing register address argument. Usage: writemdc <phy_addr> <reg_addr> <value>"));
		return;
	}

	uint8_t regAddr = parseHexByte(regStr, parseSuccess);
	if (!parseSuccess) {
		printError(F("Invalid register address format. Use hex (e.g., 0x01)"));
		return;
	}

	// Find value argument
	const char* valueStr = nextArg(regStr);
	if (!valueStr) {
		printError(F("Missing value argument. Usage: writemdc <phy_addr> <reg_addr> <value>"));
		return;
	}

	uint16_t value = parseHexAddress(valueStr, parseSuccess);
	if (!parseSuccess) {
		printError(F("Invalid value format. Use hex (e.g., 0x1234)"));
		return;
	}

	// Write the value
	mdcController.writeRegister(phyAddr, regAddr, value);
	Serial.print(F("Wrote "));
	printHexWord(value);
	Serial.print(F(" to PHY "));
	printHexByte(phyAddr);
	Serial.print(F(" Register "));
	printHexByte(regAddr);
	Serial.println();
}

void Terminal::handleScanMdcCommand(const char* args) {
	(void)args;
	bool found = false;

	// Scan all possible PHY addresses (0-31); stop at the first that responds.
	for (uint8_t phyAddr = 0; phyAddr < 32; phyAddr++) {
		uint16_t value = mdcController.readRegister(phyAddr, 0x00);

		if (value != 0x0000) {
			found = true;
			if (phyAddr < 0x10)
				Serial.print('0');
			Serial.println(phyAddr, HEX);
			break;
		}

		// Small delay to prevent overwhelming the serial output
		delay(100);
	}

	if (!found) {
		Serial.println(F("No PHY devices found."));
	}

	// Ensure all output is sent
	Serial.flush();
}

// One row per switch port. Ports 1-5 come from the switch's internal PHYs; ports 6-7
// come from the external PHYs over MDIO, alongside the switch's fixed RGMII MAC setting
// so a mismatch between the two is visible. A port disabled with `port N off` shows
// "off" in place of its link state.
void Terminal::handleStatusCommand(const char* args) {
	(void)args;
	Serial.println(F("Port  Link  Speed  Duplex"));
	for (uint8_t port = 1; port <= Board::PORT_COUNT; port++) {
		Serial.print(port);
		Serial.print(F("     "));

		if (!(settings.data.portEnabled & Board::portBit(port))) {
			Serial.println(F("off"));
			continue;
		}

		uint8_t portStatus = spiController.readPortStatus(port);
		uint8_t macSpeed = (portStatus >> SpiController::PORT_STATUS_SPEED_SHIFT) &
			SpiController::PORT_STATUS_SPEED_MASK;
		bool macFullDuplex = portStatus & SpiController::PORT_STATUS_FULL_DUPLEX;

		if (Board::isExternalPort(port)) {
			uint8_t phyAddr = Board::phyAddressForPort(port);
			bool linkUp = mdcController.readLinkStatus(phyAddr);
			uint16_t auxStatus = mdcController.readAuxStatus(phyAddr);
			uint8_t phySpeed = (auxStatus >> MdcMdioController::AUX_STATUS_SPEED_SHIFT) &
				MdcMdioController::AUX_STATUS_SPEED_MASK;
			printLinkState(
				linkUp, phySpeed, auxStatus & MdcMdioController::AUX_STATUS_FULL_DUPLEX, true);
			Serial.print(F("  (PHY "));
			printHexByte(phyAddr);
			Serial.print(F(", MAC "));
			printLinkState(true, macSpeed, macFullDuplex, true);
			Serial.print(')');
		} else {
			printLinkState(spiController.readInternalPhyLink(port), macSpeed, macFullDuplex, true);
		}
		Serial.println();
	}
}

void Terminal::handlePortCommand(const char* args) {
	uint8_t port;
	if (!parsePort(args, port))
		return;

	const char* state = nextArg(args);
	bool enable = matchWord(state, PSTR("on"));
	if (!enable && !matchWord(state, PSTR("off"))) {
		printError(F("Usage: port <1-7> on|off"));
		return;
	}

	if (enable)
		settings.data.portEnabled |= Board::portBit(port);
	else
		settings.data.portEnabled &= ~Board::portBit(port);
	setPortEnabled(port, enable);

	Serial.print(F("Port "));
	Serial.print(port);
	Serial.println(enable ? F(" on") : F(" off"));
}

// Set which ports frames arriving on a port may be forwarded to. The rule is one-way:
// `isolate 3 1` stops port 3 reaching anything but port 1, while port 1 can still reach
// port 3 unless its own list is narrowed too.
void Terminal::handleIsolateCommand(const char* args) {
	uint8_t port;
	if (!parsePort(args, port))
		return;

	const char* list = nextArg(args);
	uint8_t mask;
	if (matchWord(list, PSTR("all")))
		mask = Board::ALL_PORTS_MASK;
	else if (!parsePortList(list, mask)) {
		printError(F("Usage: isolate <1-7> all|<port,port,...>"));
		return;
	}

	settings.data.membership[port - 1] = mask;
	spiController.setPortMembership(port, mask);

	Serial.print(F("Port "));
	Serial.print(port);
	Serial.print(F(" forwards to "));
	printPortList(mask);
	Serial.println();
}

// mirror <source> <dest> [rx|tx|both]  copies the source port's traffic to dest.
// mirror off                            stops mirroring.
void Terminal::handleMirrorCommand(const char* args) {
	Settings::Data& data = settings.data;

	if (matchWord(args, PSTR("off"))) {
		data.mirrorSource = 0;
		data.mirrorDest = 0;
	} else {
		uint8_t source;
		uint8_t dest;
		const char* destStr = nextArg(args);
		if (!parsePort(args, source) || !parsePort(destStr, dest))
			return;
		if (source == dest) {
			printError(F("Source and destination must differ"));
			return;
		}

		const char* mode = nextArg(destStr);
		if (!mode || matchWord(mode, PSTR("both")))
			data.mirrorMode = Settings::MIRROR_RX | Settings::MIRROR_TX;
		else if (matchWord(mode, PSTR("rx")))
			data.mirrorMode = Settings::MIRROR_RX;
		else if (matchWord(mode, PSTR("tx")))
			data.mirrorMode = Settings::MIRROR_TX;
		else {
			printError(F("Usage: mirror <src> <dst> [rx|tx|both] or mirror off"));
			return;
		}
		data.mirrorSource = source;
		data.mirrorDest = dest;
	}

	applyMirror();
	printMirrorSetting();
}

// counters <port>  prints the port's MIB counters. The switch clears each counter as
// it is read, so every listing covers the time since the previous one.
// counters clear   zeroes the counters on every port.
void Terminal::handleCountersCommand(const char* args) {
	if (matchWord(args, PSTR("clear"))) {
		spiController.clearMibCounters();
		Serial.println(F("Counters cleared"));
		return;
	}

	uint8_t port;
	if (!parsePort(args, port))
		return;

	Serial.print(F("Port "));
	Serial.print(port);
	Serial.println(F(" counters since last read ('+' = beyond 32 bits):"));
	for (uint8_t i = 0; i < MIB_COUNT; i++) {
		MibEntry entry;
		memcpy_P(&entry, &MIB_TABLE[i], sizeof(entry));
		uint8_t high;
		uint32_t value = spiController.readMibCounter(port, entry.index, high);

		Serial.print(F("  "));
		Serial.print(entry.name);
		for (uint8_t pad = strlen(entry.name); pad < sizeof(entry.name); pad++)
			Serial.print(' ');
		Serial.print(value);
		if (high)
			Serial.print('+');
		Serial.println();
	}
}

void Terminal::handleLogCommand(const char* args) {
	bool enable = matchWord(args, PSTR("on"));
	if (!enable && !matchWord(args, PSTR("off"))) {
		printError(F("Usage: log on|off"));
		return;
	}
	settings.data.linkLog = enable;
	Serial.println(enable ? F("Link log on") : F("Link log off"));
}

// Change the VSC8531 RGMII clock delay on both external PHYs. Their links drop and
// come back with the new timing, so this can be tuned live with iperf running.
void Terminal::handleRgmiiCommand(const char* args) {
	bool parseSuccess;
	uint16_t delay = parseHexAddress(args, parseSuccess);
	if (!parseSuccess || (delay & 0xFF88)) {
		printError(F("Usage: rgmii 0x00XY (X = RX_CLK code 0-7, Y = TX_CLK code 0-7)"));
		return;
	}

	settings.data.rgmiiDelay = delay;
	for (uint8_t phyAddr : Board::PHY_ADDRESSES)
		mdcController.setRgmiiDelay(phyAddr, delay);

	Serial.print(F("RGMII delay "));
	printHexWord(delay);
	Serial.println(F(" applied to both PHYs"));
}

void Terminal::handleShowCommand(const char* args) {
	(void)args;
	const Settings::Data& data = settings.data;

	Serial.println(F("Port  Admin  Forwards to"));
	for (uint8_t port = 1; port <= Board::PORT_COUNT; port++) {
		Serial.print(port);
		Serial.print(
			(data.portEnabled & Board::portBit(port)) ? F("     on     ") : F("     off    "));
		printPortList(data.membership[port - 1]);
		Serial.println();
	}
	printMirrorSetting();
	Serial.println(data.linkLog ? F("Link log: on") : F("Link log: off"));
	Serial.print(F("RGMII delay: "));
	printHexWord(data.rgmiiDelay);
	Serial.println();
	Serial.println(settings.isSaved() ? F("Config: saved") : F("Config: unsaved (type 'save')"));
}

void Terminal::handleSaveCommand(const char* args) {
	(void)args;
	settings.save();
	Serial.println(F("Saved to EEPROM"));
}

// Return the running config to factory values and apply the differences. Ports that
// were off come back on; ports already on are not disturbed. Nothing is written to
// EEPROM until `save`.
void Terminal::handleDefaultsCommand(const char* args) {
	(void)args;
	Settings::Data previous = settings.data;
	settings.setDefaults();

	for (uint8_t port = 1; port <= Board::PORT_COUNT; port++) {
		uint8_t bit = Board::portBit(port);
		if ((previous.portEnabled ^ settings.data.portEnabled) & bit)
			setPortEnabled(port, settings.data.portEnabled & bit);
		spiController.setPortMembership(port, settings.data.membership[port - 1]);
	}
	applyMirror();
	if (previous.rgmiiDelay != settings.data.rgmiiDelay)
		for (uint8_t phyAddr : Board::PHY_ADDRESSES)
			mdcController.setRgmiiDelay(phyAddr, settings.data.rgmiiDelay);

	Serial.println(F("Defaults applied (not saved)"));
}

void Terminal::handleSelfTestCommand(const char* args) {
	(void)args;
	printChipCheck();
}

void Terminal::handleRebootCommand(const char* args) {
	(void)args;
	Serial.println(F("Rebooting..."));
	Serial.flush();
	Watchdog::reboot();
}

// Stop servicing the watchdog so it fires naturally. Proves the hang protection works.
void Terminal::handleHangCommand(const char* args) {
	(void)args;
	Serial.println(F("Hanging until the watchdog fires..."));
	Serial.flush();
	for (;;) {
	}
}

void Terminal::handleVersionCommand(const char* args) {
	(void)args;
	printBanner();
}

void Terminal::handleHelpCommand(const char* args) {
	(void)args;
	Serial.println(F("Switch:"));
	Serial.println(F("  status                     - Link, speed and duplex for all 7 ports"));
	Serial.println(F("  port <n> on|off            - Enable or disable a port"));
	Serial.println(F("  isolate <n> all|<p,p,..>   - Limit which ports <n> may forward to"));
	Serial.println(F("  mirror <src> <dst> [rx|tx|both] / mirror off"));
	Serial.println(F("  counters <n>|clear         - MIB counters (cleared on read)"));
	Serial.println(F("  log on|off                 - Print link changes as they happen"));
	Serial.println(F("  rgmii <0x00XY>             - RGMII delay for the external PHYs"));
	Serial.println(F("Config:"));
	Serial.println(F("  show / save / defaults     - Running config, store it, factory reset"));
	Serial.println(F("Registers:"));
	Serial.println(F("  read <address> <count>     - e.g. read 0x01FF 3"));
	Serial.println(F("  write <address> <value>    - e.g. write 0x01FF 0xC0"));
	Serial.println(F("  readmdc <phy> <reg>        - e.g. readmdc 0x10 0x00"));
	Serial.println(F("  writemdc <phy> <reg> <val> - e.g. writemdc 0x01 0x00 0x1234"));
	Serial.println(F("  scanmdc                    - Scan for PHY devices"));
	Serial.println(F("System:"));
	Serial.println(F("  selftest                   - Re-check switch and PHY chip IDs"));
	Serial.println(F("  version                    - Firmware version and build time"));
	Serial.println(F("  reboot                     - Restart the controller"));
	Serial.println(F("  hang                       - Stop kicking the watchdog (test)"));
	Serial.println(F("  help                       - Show this help message"));
}

void Terminal::printPrompt() {
	Serial.print(F("> "));
}

void Terminal::printError(const __FlashStringHelper* message) {
	Serial.print(F("Error: "));
	Serial.println(message);
}

// Print "up 1000 full" or "down". speedCode is the 2-bit encoding shared by the KSZ9897
// port status and VSC8531 auxiliary status registers: 0 = 10, 1 = 100, 2 = 1000. With
// `columns` the fields are padded to line up under the status table header.
void Terminal::printLinkState(bool linkUp, uint8_t speedCode, bool fullDuplex, bool columns) {
	if (!linkUp) {
		Serial.print(F("down"));
		return;
	}
	Serial.print(columns ? F("up    ") : F("up "));
	switch (speedCode) {
	case 0:
		Serial.print(columns ? F("10     ") : F("10 "));
		break;
	case 1:
		Serial.print(columns ? F("100    ") : F("100 "));
		break;
	case 2:
		Serial.print(columns ? F("1000   ") : F("1000 "));
		break;
	default:
		Serial.print(columns ? F("?      ") : F("? "));
		break;
	}
	Serial.print(fullDuplex ? F("full") : F("half"));
}

// Print a port mask as "all", "none" or a comma-separated list such as "1,6".
void Terminal::printPortList(uint8_t mask) {
	mask &= Board::ALL_PORTS_MASK;
	if (mask == Board::ALL_PORTS_MASK) {
		Serial.print(F("all"));
		return;
	}
	if (mask == 0) {
		Serial.print(F("none"));
		return;
	}
	bool first = true;
	for (uint8_t port = 1; port <= Board::PORT_COUNT; port++) {
		if (!(mask & Board::portBit(port)))
			continue;
		if (!first)
			Serial.print(',');
		Serial.print(port);
		first = false;
	}
}

void Terminal::printMirrorSetting() {
	const Settings::Data& data = settings.data;
	Serial.print(F("Mirror: "));
	if (data.mirrorSource == 0) {
		Serial.println(F("off"));
		return;
	}
	Serial.print(F("port "));
	Serial.print(data.mirrorSource);
	if (data.mirrorMode == Settings::MIRROR_RX)
		Serial.print(F(" rx"));
	else if (data.mirrorMode == Settings::MIRROR_TX)
		Serial.print(F(" tx"));
	else
		Serial.print(F(" rx+tx"));
	Serial.print(F(" -> port "));
	Serial.println(data.mirrorDest);
}

// Confirm the switch answers with the KSZ9897 chip ID. Prints one status line.
bool Terminal::verifySwitch() {
	uint16_t chipId = 0;
	for (uint8_t attempt = 0; attempt < ID_CHECK_ATTEMPTS; attempt++) {
		chipId = spiController.readChipId();
		if (chipId == SpiController::KSZ9897_CHIP_ID) {
			Serial.print(F("Switch KSZ9897 rev "));
			Serial.print(spiController.readRevision());
			Serial.println(F(": OK"));
			return true;
		}
		delay(10);
	}
	Serial.print(F("Switch: FAIL (id "));
	printHexWord(chipId);
	Serial.println(')');
	return false;
}

// Confirm the PHY at phyAddr answers with the VSC8531 identifier. Prints one status line.
bool Terminal::verifyPhy(uint8_t phyAddr) {
	uint32_t phyId = 0;
	for (uint8_t attempt = 0; attempt < ID_CHECK_ATTEMPTS; attempt++) {
		phyId = mdcController.readPhyId(phyAddr);
		if ((phyId & ~MdcMdioController::PHY_ID_REVISION_MASK) ==
			MdcMdioController::VSC8531_PHY_ID) {
			Serial.print(F("PHY "));
			printHexByte(phyAddr);
			Serial.print(F(" VSC8531 rev "));
			Serial.print(static_cast<uint8_t>(phyId & MdcMdioController::PHY_ID_REVISION_MASK));
			Serial.println(F(": OK"));
			return true;
		}
		delay(10);
	}
	Serial.print(F("PHY "));
	printHexByte(phyAddr);
	Serial.print(F(": FAIL (id 0x"));
	Serial.print(phyId, HEX);
	Serial.println(')');
	return false;
}

// Print a byte as "0x" followed by two zero-padded hex digits.
void Terminal::printHexByte(uint8_t value) {
	Serial.print(F("0x"));
	if (value < 0x10)
		Serial.print('0');
	Serial.print(value, HEX);
}

// Print a 16-bit word as "0x" followed by four zero-padded hex digits.
void Terminal::printHexWord(uint16_t value) {
	Serial.print(F("0x"));
	if (value < 0x1000)
		Serial.print('0');
	if (value < 0x100)
		Serial.print('0');
	if (value < 0x10)
		Serial.print('0');
	Serial.print(value, HEX);
}

uint16_t Terminal::parseHexAddress(const char* str, bool& success) {
	success = false;

	if (!str) {
		return 0;
	}

	// Skip any leading whitespace
	while (*str == ' ')
		str++;

	if (strncmp(str, "0x", 2) != 0) {
		return 0;
	}

	char* endptr;
	unsigned long value = strtoul(str, &endptr, 16);

	// Check if parsing was successful and value is within 16-bit range
	if (*endptr == '\0' || *endptr == ' ') {
		if (value <= 0xFFFF) {
			success = true;
			return static_cast<uint16_t>(value);
		}
	}

	return 0;
}

uint8_t Terminal::parseHexByte(const char* str, bool& success) {
	unsigned long value = parseHexAddress(str, success);
	if (value > 0xFF)
		success = false;
	return success ? static_cast<uint8_t>(value) : 0;
}

int Terminal::parseDecimal(const char* str, bool& success) {
	success = (str != nullptr);
	if (!success) {
		return -1;
	}
	return atoi(str);
}

// Parse a port number 1-7 from the start of str. Prints the error itself.
bool Terminal::parsePort(const char* str, uint8_t& port) {
	if (str) {
		char* end;
		unsigned long value = strtoul(str, &end, 10);
		if (end != str && (*end == '\0' || *end == ' ') && value >= 1 &&
			value <= Board::PORT_COUNT) {
			port = value;
			return true;
		}
	}
	printError(F("Port must be 1-7"));
	return false;
}

// Parse "1,2,6" into a port mask. Returns false on any malformed or out-of-range entry.
bool Terminal::parsePortList(const char* str, uint8_t& mask) {
	if (!str)
		return false;
	mask = 0;
	for (;;) {
		char* end;
		unsigned long value = strtoul(str, &end, 10);
		if (end == str || value < 1 || value > Board::PORT_COUNT)
			return false;
		mask |= Board::portBit(value);
		if (*end == '\0' || *end == ' ')
			return true;
		if (*end != ',')
			return false;
		str = end + 1;
	}
}

// The argument after the current one, or nullptr if there is none.
const char* Terminal::nextArg(const char* args) {
	if (!args)
		return nullptr;
	const char* space = strchr(args, ' ');
	if (!space)
		return nullptr;
	while (*space == ' ')
		space++;
	return *space ? space : nullptr;
}

// True if args starts with the flash string word, followed by a space or the end.
bool Terminal::matchWord(const char* args, PGM_P word) {
	if (!args)
		return false;
	size_t length = strlen_P(word);
	return strncmp_P(args, word, length) == 0 && (args[length] == '\0' || args[length] == ' ');
}
