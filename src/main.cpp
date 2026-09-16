#include <Arduino.h>
#include "Board.h"
#include "Watchdog.h"
#include "Settings.h"
#include "SpiController.h"
#include "MdcMdioController.h"
#include "Terminal.h"
#include "LinkSync.h"

Settings settings;
SpiController spiController(Board::SWITCH_CS_PIN);
MdcMdioController mdcController(Board::MDC_PIN, Board::MDIO_PIN);
Terminal terminal(spiController, mdcController, settings);
LinkSync linkSync(mdcController, spiController, settings, terminal);

// How often every port is polled for link changes and, on the external ports, to keep
// the RGMII MAC speed in step with the PHY.
static constexpr unsigned long LINK_POLL_INTERVAL_MS = 100;
static unsigned long lastLinkPollMs = 0;

void setup() {
	// Guard bring-up as well as the main loop
	Watchdog::begin();

	digitalWrite(Board::PHY_RESET_PIN, LOW);
	pinMode(Board::PHY_RESET_PIN, OUTPUT);
	// Initialize serial communication at 57600 baud
	Serial.begin(57600);
	terminal.printBanner();
	Watchdog::printResetCause();

	if (!settings.load())
		Serial.println(F("Config: no saved settings, using defaults"));

	// delay for the clock to be stable
	delay(250);

	digitalWrite(Board::PHY_RESET_PIN, HIGH);
	// delay according to dual phy reset data sheet requirements
	delay(250);
	// Initialize MDC/MDIO
	mdcController.begin();

	// Bring up both PHYs
	for (uint8_t phyAddr : Board::PHY_ADDRESSES)
		mdcController.initializeDualPhy(phyAddr, settings.data.rgmiiDelay);

	delay(250);

	// Initialize SPI
	spiController.begin();

	// Report whether every chip answered with the expected ID
	terminal.printChipCheck();

	// Port enables, isolation and mirroring from the saved configuration
	terminal.applySettingsAtBoot();

	// Initialize terminal
	terminal.begin();
}

void loop() {
	Watchdog::kick();
	// Process any incoming terminal commands
	terminal.processInput();

	unsigned long now = millis();
	if (now - lastLinkPollMs >= LINK_POLL_INTERVAL_MS) {
		lastLinkPollMs = now;
		linkSync.poll();
	}
}
