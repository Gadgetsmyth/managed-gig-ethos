#include <Arduino.h>
#include "Board.h"
#include "Watchdog.h"
#include "SpiController.h"
#include "MdcMdioController.h"
#include "Terminal.h"
#include "LinkSync.h"

SpiController spiController(Board::SWITCH_CS_PIN);
MdcMdioController mdcController(Board::MDC_PIN, Board::MDIO_PIN);
Terminal terminal(spiController, mdcController);
LinkSync linkSync(mdcController, spiController);

// How often the external PHYs are polled to keep the RGMII MAC speed in step.
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

	// delay for the clock to be stable
	delay(250);

	digitalWrite(Board::PHY_RESET_PIN, HIGH);
	// delay according to dual phy reset data sheet requirements
	delay(250);
	// Initialize MDC/MDIO
	mdcController.begin();

	// Bring up both PHYs
	for (uint8_t phyAddr : Board::PHY_ADDRESSES)
		mdcController.initializeDualPhy(phyAddr);

	delay(250);

	// Initialize SPI
	spiController.begin();

	// Report whether every chip answered with the expected ID
	terminal.printChipCheck();

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
