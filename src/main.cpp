#include <Arduino.h>
#include "SpiController.h"
#include "MdcMdioController.h"
#include "Terminal.h"

SpiController spiController;
// MDC on PC5 (A5), MDIO on PC4 (A4)
MdcMdioController mdcController(A5, A4);
Terminal terminal(spiController, mdcController);
const int resetPin = A1;

// MDIO addresses of the two VSC8531 PHYs, strapped on the board.
static constexpr uint8_t PHY_ADDRESSES[] = {0x00, 0x10};

// How many times to re-read a chip ID before declaring the chip missing.
static constexpr uint8_t ID_CHECK_ATTEMPTS = 3;

// Confirm the switch answers with the KSZ9897 chip ID. Prints one status line.
static bool verifySwitch() {
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
	Serial.print(F("Switch: FAIL (id 0x"));
	Serial.print(chipId, HEX);
	Serial.println(')');
	return false;
}

// Confirm the PHY at phyAddr answers with the VSC8531 identifier. Prints one status line.
static bool verifyPhy(uint8_t phyAddr) {
	uint32_t phyId = 0;
	for (uint8_t attempt = 0; attempt < ID_CHECK_ATTEMPTS; attempt++) {
		phyId = mdcController.readPhyId(phyAddr);
		if ((phyId & ~MdcMdioController::PHY_ID_REVISION_MASK) ==
			MdcMdioController::VSC8531_PHY_ID) {
			Serial.print(F("PHY 0x"));
			Serial.print(phyAddr, HEX);
			Serial.print(F(" VSC8531 rev "));
			Serial.print(static_cast<uint8_t>(phyId & MdcMdioController::PHY_ID_REVISION_MASK));
			Serial.println(F(": OK"));
			return true;
		}
		delay(10);
	}
	Serial.print(F("PHY 0x"));
	Serial.print(phyAddr, HEX);
	Serial.print(F(": FAIL (id 0x"));
	Serial.print(phyId, HEX);
	Serial.println(')');
	return false;
}

void setup() {
	digitalWrite(resetPin, LOW);
	pinMode(resetPin, OUTPUT);
	// Initialize serial communication at 57600 baud
	Serial.begin(57600);

	// delay for the clock to be stable
	delay(250);

	digitalWrite(resetPin, HIGH);
	// delay according to dual phy reset data sheet requirements
	delay(250);
	// Initialize MDC/MDIO
	mdcController.begin();

	// Bring up both PHYs
	for (uint8_t phyAddr : PHY_ADDRESSES)
		mdcController.initializeDualPhy(phyAddr);

	delay(250);

	// Initialize SPI
	spiController.begin();

	// Report whether every chip answered with the expected ID
	verifySwitch();
	for (uint8_t phyAddr : PHY_ADDRESSES)
		verifyPhy(phyAddr);

	// Initialize terminal
	terminal.begin();
}

void loop() {
	// Process any incoming terminal commands
	terminal.processInput();
}
