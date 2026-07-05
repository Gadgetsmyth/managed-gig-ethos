#include <Arduino.h>
#include "SpiController.h"
#include "MdcMdioController.h"
#include "Terminal.h"

SpiController spiController;
// MDC on PC5 (A5), MDIO on PC4 (A4)
MdcMdioController mdcController(A5, A4);
Terminal terminal(spiController, mdcController);
const int resetPin = A1;

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

	// Initialize terminal
	terminal.begin();

	// Bring up both PHYs of the dual-PHY device (base addresses 0x00 and 0x10)
	mdcController.initializeDualPhy(0x00);
	mdcController.initializeDualPhy(0x10);

	delay(250);

	// Initialize SPI
	spiController.begin();
}

void loop() {
	// Process any incoming terminal commands
	terminal.processInput();
}