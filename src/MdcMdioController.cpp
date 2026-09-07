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

void MdcMdioController::initializeDualPhy(uint8_t phyAddr, uint16_t rgmiiDelay) {
	// set standard page section
	writeRegister(phyAddr, 0x1f, 0x0000);
	// hardware bringup
	// set broadcast mode. ( so we don't have to write both phys)
	writeRegister(phyAddr, 0x16, 0x3201);

	// switch to the test page
	writeRegister(phyAddr, 0x1f, 0x2A30);

	// Analog trims from Microchip's VSC8531 init sequence (as in the Linux mscc driver).
	writeRegisterMasked(phyAddr, 0x18, 0x0400, 0x0400); // 1000BASE-T bias
	writeRegisterMasked(phyAddr, 0x05, 0x0a00, 0x0e00); // 100BASE-TX pre-emphasis
	writeRegisterMasked(phyAddr, 0x08, 0x8000, 0x8000); // token ring clock enable

	// change to token ring page
	writeRegister(phyAddr, 0x1f, 0x52B5);

	// Token-ring writes: 32-bit value in registers 18/17 (0x12 MSB, 0x11 LSB), then the
	// target address with the write bit (0x8000) in register 16 (0x10). Values are the
	// VSC8531 sequence from the Linux mscc driver (vsc8531_pre_init_seq_set and
	// vsc85xx_eee_init_seq_set); they are characterization results, not documented bits.
	writeRegister(phyAddr, 0x12, 0x0068);
	writeRegister(phyAddr, 0x11, 0x8980);
	writeRegister(phyAddr, 0x10, 0x8f90);

	writeRegister(phyAddr, 0x12, 0x0000);
	writeRegister(phyAddr, 0x11, 0x0003);
	writeRegister(phyAddr, 0x10, 0x8696);

	writeRegister(phyAddr, 0x12, 0x0050);
	writeRegister(phyAddr, 0x11, 0x100f);
	writeRegister(phyAddr, 0x10, 0x87fa);

	writeRegister(phyAddr, 0x12, 0x0012);
	writeRegister(phyAddr, 0x11, 0xb00a);
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

	writeRegister(phyAddr, 0x12, 0x0000);
	writeRegister(phyAddr, 0x11, 0x0af4);
	writeRegister(phyAddr, 0x10, 0x8f80);

	writeRegister(phyAddr, 0x12, 0x0090);
	writeRegister(phyAddr, 0x11, 0x1809);
	writeRegister(phyAddr, 0x10, 0x8fec);

	writeRegister(phyAddr, 0x12, 0x0000);
	writeRegister(phyAddr, 0x11, 0xa6a1);
	writeRegister(phyAddr, 0x10, 0x8fee);

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

	writeRegisterMasked(phyAddr, 0x08, 0x0000, 0x8000); // token ring clock back off

	// set standard page section
	writeRegister(phyAddr, 0x1f, 0x0000);
	// configure the dual phy
	// set broadcast mode. ( so we don't have to write both phys)
	writeRegister(phyAddr, 0x16, 0x3201);
	// do a soft reset
	writeRegister(phyAddr, 0x00, 0x9040);
	// Select the RGMII MAC interface: register 23 bits 12:11 = 10. Takes effect at the
	// following soft reset. Other bits are left alone; bit 13 (RX_CLK from REFCLK) in
	// particular stays at its default of 0.
	writeRegisterMasked(phyAddr, 0x17, 0x1000, 0x1800);
	// soft reset again (set bit 15, default 0x1040)
	writeRegister(phyAddr, 0x00, 0x9040);

	// RGMII clock delays live in extended page 2 (register 31 = 0x0002 remaps 16-30).
	writeRegister(phyAddr, 0x1f, 0x0002);
	// Register 20E2: bits 6:4 delay the RX_CLK the PHY drives toward the switch, bits 2:0
	// delay the TX_CLK it receives. Codes 0-7 give 0.2, 0.8, 1.1, 1.7, 2.0, 2.3, 2.6 and
	// 3.4 ns. Bits 15:8 are reserved. The switch adds no delay of its own on ports 6-7
	// (0xN301 bits 4:3 are cleared in SpiController::begin), so these are the whole
	// RGMII skew budget. Default 0x0042 is 2.0 ns RX / 1.1 ns TX.
	// TODO: tune this value further - links up and passes traffic, but needs more testing.
	writeRegister(phyAddr, 0x14, rgmiiDelay);
	// reset the extended field
	writeRegister(phyAddr, 0x1f, 0x0000);

	writeRegister(phyAddr, 0x00, 0x9040);
	// turn smi duplication back off
	writeRegister(phyAddr, 0x16, 0x3200);
}

uint32_t MdcMdioController::readPhyId(uint8_t phyAddr) {
	uint32_t phyId = readRegister(phyAddr, 0x02);
	return (phyId << 16) | readRegister(phyAddr, 0x03);
}

bool MdcMdioController::readLinkStatus(uint8_t phyAddr) {
	readRegister(phyAddr, 0x01);
	return readRegister(phyAddr, 0x01) & _BV(2);
}

uint16_t MdcMdioController::readAuxStatus(uint8_t phyAddr) {
	return readRegister(phyAddr, 0x1C);
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