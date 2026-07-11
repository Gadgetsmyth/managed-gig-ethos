#include "MdcMdioController.h"

// Half-period of the bit-banged MDC clock, in microseconds.
static constexpr uint8_t DELAY_US = 10;

MdcMdioController::MdcMdioController(int mdcPin, int mdioPin) : mdcPin(mdcPin), mdioPin(mdioPin) {}

void MdcMdioController::begin() {
	// Idle both lines high, then drive them as outputs.
	digitalWrite(mdcPin, HIGH);
	digitalWrite(mdioPin, HIGH);
	pinMode(mdcPin, OUTPUT);
	pinMode(mdioPin, OUTPUT);
}

void MdcMdioController::initializeDualPhy(uint8_t phyAddr) {
	// set standard page section
	writeRegister(phyAddr, 0x1f, 0x0000);
	// hardware bringup
	// set broadcast mode. ( so we don't have to write both phys)
	writeRegister(phyAddr, 0x16, 0x3201);

	// switch to the test page
	writeRegister(phyAddr, 0x1f, 0x2A30);

	writeRegisterMasked(phyAddr, 0x18, 0x0000, 0x0400); // clear bias bit for 1000BT distortion
	writeRegisterMasked(phyAddr, 0x05, 0x0c00, 0x0e00); // optimize pre-emphasis for 100basetx
	writeRegisterMasked(phyAddr, 0x08, 0x8000, 0x8000); // token ring enable

	// change to token ring page
	writeRegister(phyAddr, 0x1f, 0x52B5);

	// Token-ring patch: each group programs registers 18/17/16 (0x12/0x11/0x10).
	writeRegister(phyAddr, 0x12, 0x0068);
	writeRegister(phyAddr, 0x11, 0x8980);
	writeRegister(phyAddr, 0x10, 0x8f90);

	writeRegister(phyAddr, 0x12, 0x0000);
	writeRegister(phyAddr, 0x11, 0x0003);
	writeRegister(phyAddr, 0x10, 0x8796);

	writeRegister(phyAddr, 0x12, 0x0050);
	writeRegister(phyAddr, 0x11, 0x100f);
	writeRegister(phyAddr, 0x10, 0x87fa);

	writeRegister(phyAddr, 0x12, 0x0012);
	writeRegister(phyAddr, 0x11, 0xb002);
	writeRegister(phyAddr, 0x10, 0x8f82);

	writeRegister(phyAddr, 0x12, 0x0000);
	writeRegister(phyAddr, 0x11, 0x0004);
	writeRegister(phyAddr, 0x10, 0x9686);

	writeRegister(phyAddr, 0x12, 0x00d2);
	writeRegister(phyAddr, 0x11, 0xc46f);
	writeRegister(phyAddr, 0x10, 0x968c);

	writeRegister(phyAddr, 0x12, 0x0000);
	writeRegister(phyAddr, 0x11, 0x0620);
	writeRegister(phyAddr, 0x10, 0x97a2);

	writeRegister(phyAddr, 0x12, 0x00ee);
	writeRegister(phyAddr, 0x11, 0xffdd);
	writeRegister(phyAddr, 0x10, 0x96a0);

	writeRegister(phyAddr, 0x12, 0x0007);
	writeRegister(phyAddr, 0x11, 0x1448);
	writeRegister(phyAddr, 0x10, 0x96a6);

	writeRegister(phyAddr, 0x12, 0x0013);
	writeRegister(phyAddr, 0x11, 0x132f);
	writeRegister(phyAddr, 0x10, 0x96a4);

	writeRegister(phyAddr, 0x12, 0x0000);
	writeRegister(phyAddr, 0x11, 0x0000);
	writeRegister(phyAddr, 0x10, 0x96a8);

	writeRegister(phyAddr, 0x12, 0x00c0);
	writeRegister(phyAddr, 0x11, 0xa028);
	writeRegister(phyAddr, 0x10, 0x8ffc);

	writeRegister(phyAddr, 0x12, 0x0091);
	writeRegister(phyAddr, 0x11, 0xb06c);
	writeRegister(phyAddr, 0x10, 0x8fe8);

	writeRegister(phyAddr, 0x12, 0x0004);
	writeRegister(phyAddr, 0x11, 0x1600);
	writeRegister(phyAddr, 0x10, 0x8fea);

	writeRegister(phyAddr, 0x12, 0x00ff);
	writeRegister(phyAddr, 0x11, 0xfaff);
	writeRegister(phyAddr, 0x10, 0x8f80);

	writeRegister(phyAddr, 0x12, 0x0090);
	writeRegister(phyAddr, 0x11, 0x1809);
	writeRegister(phyAddr, 0x10, 0x8fec);

	writeRegister(phyAddr, 0x12, 0x00b0);
	writeRegister(phyAddr, 0x11, 0x1007);
	writeRegister(phyAddr, 0x10, 0x8ffe);

	writeRegister(phyAddr, 0x12, 0x00ee);
	writeRegister(phyAddr, 0x11, 0xff00);
	writeRegister(phyAddr, 0x10, 0x96b0);

	writeRegister(phyAddr, 0x12, 0x0000);
	writeRegister(phyAddr, 0x11, 0x7000);
	writeRegister(phyAddr, 0x10, 0x96b2);

	writeRegister(phyAddr, 0x12, 0x0000);
	writeRegister(phyAddr, 0x11, 0x0814);
	writeRegister(phyAddr, 0x10, 0x96b4);

	// switch to the test page
	writeRegister(phyAddr, 0x1f, 0x2A30);

	writeRegisterMasked(phyAddr, 0x08, 0x0000, 0x8000); // token ring enable

	// set standard page section
	writeRegister(phyAddr, 0x1f, 0x0000);
	// configure the dual phy
	// set broadcast mode. ( so we don't have to write both phys)
	writeRegister(phyAddr, 0x16, 0x3201);
	// do a soft reset
	writeRegister(phyAddr, 0x00, 0x9040);
	// set RGMII mode (bit 12 of register 0x17, default 0x2000)
	writeRegister(phyAddr, 0x17, 0x3000);
	// soft reset again (set bit 15, default 0x1040)
	writeRegister(phyAddr, 0x00, 0x9040);

	// TODO: APPLY PRODUCTION PATCH HERE!

	// Adjust the clock control. Set register 0x1F to 2 to map the E2 address space,
	// remapping registers 16-30 (0x10-0x1E) from the main space.
	writeRegister(phyAddr, 0x1f, 0x0002);
	// Register 0x14 in space 2 sets the clock delay: bits 2:0 are the GTX clock delay
	// and bits 6:4 the RX delay; each step is +0.3ns (min 0.2ns). High nibble is the RX
	// pair, low nibble the TX pair. The switch already compensates TX, so RX needs more
	// delay. Bit 11 (default field 0x0800) is also cleared here.
	// TODO: tune this value further - links up and passes traffic, but needs more testing.
	writeRegister(phyAddr, 0x14, 0x0042);
	// reset the extended field
	writeRegister(phyAddr, 0x1f, 0x0000);

	writeRegister(phyAddr, 0x00, 0x9040);
	// turn smi duplication back off
	writeRegister(phyAddr, 0x16, 0x3200);
}

void MdcMdioController::clockBit(bool mdioValue) {
	digitalWrite(mdcPin, LOW);
	digitalWrite(mdioPin, mdioValue);
	delayMicroseconds(DELAY_US);
	digitalWrite(mdcPin, HIGH);
	delayMicroseconds(DELAY_US);
}

void MdcMdioController::writeRegisterMasked(
	uint8_t phyAddr, uint8_t regAddr, uint16_t data, uint16_t mask) {
	uint16_t currentVal = readRegister(phyAddr, regAddr);
	uint16_t valToWrite = (currentVal & ~mask) | (data & mask);
	writeRegister(phyAddr, regAddr, valToWrite);
}

uint16_t MdcMdioController::readRegister(uint8_t phyAddr, uint8_t regAddr) {
	pinMode(mdioPin, OUTPUT);

	// Preamble - 32 bits of HIGH
	for (uint8_t preambleBit = 0; preambleBit < 32; preambleBit++)
		clockBit(HIGH);

	// Start code (01b)
	clockBit(LOW);
	clockBit(HIGH);

	// Read OP code (10b)
	clockBit(HIGH);
	clockBit(LOW);

	// PHY address - 5 bits, MSB first
	for (uint8_t addressBit = 0x10; addressBit != 0; addressBit = addressBit >> 1)
		clockBit(addressBit & phyAddr ? HIGH : LOW);

	// Register address - 5 bits, MSB first
	for (uint8_t registerBit = 0x10; registerBit != 0; registerBit = registerBit >> 1)
		clockBit(registerBit & regAddr ? HIGH : LOW);

	// Turnaround bits - MDIO becomes an input while the PHY drives the bus
	pinMode(mdioPin, INPUT);

	// TA bit 1 - high Z (controller releases, pullup holds high)
	digitalWrite(mdcPin, LOW);
	delayMicroseconds(DELAY_US);
	digitalWrite(mdcPin, HIGH);
	delayMicroseconds(DELAY_US);

	// TA bit 2 - PHY drives LOW to acknowledge (TODO: check the acknowledgment and error if absent)
	digitalWrite(mdcPin, LOW);
	delayMicroseconds(DELAY_US);
	digitalWrite(mdcPin, HIGH);
	delayMicroseconds(DELAY_US);

	// Data - 16 bits, MSB first
	uint16_t data = 0;
	for (uint16_t dataBit = 0x8000; dataBit != 0; dataBit = dataBit >> 1) {
		digitalWrite(mdcPin, LOW);
		delayMicroseconds(DELAY_US / 2);

		if (digitalRead(mdioPin))
			data |= dataBit;

		delayMicroseconds(DELAY_US / 2);
		digitalWrite(mdcPin, HIGH);
		delayMicroseconds(DELAY_US);
	}

	// Return MDIO to output and idle high
	pinMode(mdioPin, OUTPUT);
	digitalWrite(mdioPin, HIGH);

	return data;
}

void MdcMdioController::writeRegister(uint8_t phyAddr, uint8_t regAddr, uint16_t data) {
	pinMode(mdioPin, OUTPUT);

	// Preamble - 32 bits of HIGH
	for (uint8_t preambleBit = 0; preambleBit < 32; preambleBit++)
		clockBit(HIGH);

	// Start code (01b)
	clockBit(LOW);
	clockBit(HIGH);

	// Write OP code (01b)
	clockBit(LOW);
	clockBit(HIGH);

	// PHY address - 5 bits, MSB first
	for (uint8_t addressBit = 0x10; addressBit != 0; addressBit = addressBit >> 1)
		clockBit(addressBit & phyAddr ? HIGH : LOW);

	// Register address - 5 bits, MSB first
	for (uint8_t registerBit = 0x10; registerBit != 0; registerBit = registerBit >> 1)
		clockBit(registerBit & regAddr ? HIGH : LOW);

	// Turnaround bits (10b)
	clockBit(HIGH);
	clockBit(LOW);

	// Data - 16 bits, MSB first
	for (uint16_t dataBit = 0x8000; dataBit != 0; dataBit = dataBit >> 1)
		clockBit(dataBit & data ? HIGH : LOW);

	// Return MDIO to idle high
	digitalWrite(mdioPin, HIGH);
}