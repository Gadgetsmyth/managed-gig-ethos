#include "SpiController.h"

// SPI command bytes: the top 3 bits select the operation, the low 5 bits are don't-care.
static constexpr uint8_t SPI_WRITE_COMMAND = 0x40; // 010xxxxx
static constexpr uint8_t SPI_READ_COMMAND = 0x60;  // 011xxxxx

SpiController::SpiController(int csPin, uint32_t clockSpeed)
	: csPin(csPin), clockSpeed(clockSpeed) {}

void SpiController::begin() {
	// Initialize SPI
	pinMode(csPin, OUTPUT);
	digitalWrite(csPin, HIGH); // Deselect device
	SPI.begin();

	// Write to the 0x7301 and 0x6301 the value of 0x00
	writeRegister(6, 3, 1, 0x00);
	writeRegister(7, 3, 1, 0x00);

	applySwitchErrata();
}

uint16_t SpiController::readChipId() {
	return readRegister16(0, 0, 0x01);
}

// Silicon revision is the high nibble of global register 0x0003.
uint8_t SpiController::readRevision() {
	return readRegister(0, 0, 0x03) >> 4;
}

uint16_t SpiController::constructAddress(uint8_t port, uint8_t function, uint8_t registerAddr) {
	// Validate inputs
	if (port > 7)
		port = 0; // Force to global if invalid port
	if (function > 0x0F)
		function = 0; // Limit to 4 bits
	if (registerAddr > 0xFF)
		registerAddr = 0; // Limit to 8 bits

	// Construct address: 0b0PPP FFFF RRRRRRRR
	return ((port & 0x07) << 12) | ((function & 0x0F) << 8) | (registerAddr & 0xFF);
}

void SpiController::startTransfer(uint8_t command, uint16_t address) {
	SPI.beginTransaction(SPISettings(clockSpeed, MSBFIRST, SPI_MODE0));
	digitalWrite(csPin, LOW);

	// Command phase is 32 bits: 3-bit opcode, 24-bit address (A23-A16 are don't-care on
	// this part), then 5 turnaround bits. Data bytes follow while chip select stays low,
	// and the switch auto-increments the address for each one.
	SPI.transfer(command);
	SPI.transfer((address >> 11) & 0x1F); // A15-A11
	SPI.transfer((address >> 3) & 0xFF);  // A10-A3
	SPI.transfer((address << 5) & 0xE0);  // A2-A0 + turnaround
}

void SpiController::endTransfer() {
	digitalWrite(csPin, HIGH);
	SPI.endTransaction();
}

void SpiController::writeRegister(
	uint8_t port, uint8_t function, uint8_t registerAddr, uint8_t data) {
	startTransfer(SPI_WRITE_COMMAND, constructAddress(port, function, registerAddr));
	SPI.transfer(data);
	endTransfer();
}

uint8_t SpiController::readRegister(uint8_t port, uint8_t function, uint8_t registerAddr) {
	startTransfer(SPI_READ_COMMAND, constructAddress(port, function, registerAddr));
	uint8_t data = SPI.transfer(0x00);
	endTransfer();
	return data;
}

uint16_t SpiController::readRegister16(uint8_t port, uint8_t function, uint8_t registerAddr) {
	startTransfer(SPI_READ_COMMAND, constructAddress(port, function, registerAddr));
	uint16_t data = static_cast<uint16_t>(SPI.transfer(0x00)) << 8;
	data |= SPI.transfer(0x00);
	endTransfer();
	return data;
}

void SpiController::writeRegister16(
	uint8_t port, uint8_t function, uint8_t registerAddr, uint16_t data) {
	startTransfer(SPI_WRITE_COMMAND, constructAddress(port, function, registerAddr));
	SPI.transfer(data >> 8);
	SPI.transfer(data & 0xFF);
	endTransfer();
}

// The internal PHYs expose IEEE registers 13 (MMD setup) and 14 (MMD data) at
// 0xN11A and 0xN11C. Select the device with op 00, load the register address, then
// switch to data mode (op 01, no post-increment) and write the value.
void SpiController::writePhyMmd(uint8_t port, uint8_t mmd, uint16_t mmdReg, uint16_t data) {
	static constexpr uint8_t MMD_SETUP = 0x1A;
	static constexpr uint8_t MMD_DATA = 0x1C;
	static constexpr uint16_t MMD_OP_DATA_NO_INCREMENT = 0x4000;

	uint16_t device = mmd & 0x1F;
	writeRegister16(port, 1, MMD_SETUP, device);
	writeRegister16(port, 1, MMD_DATA, mmdReg);
	writeRegister16(port, 1, MMD_SETUP, MMD_OP_DATA_NO_INCREMENT | device);
	writeRegister16(port, 1, MMD_DATA, data);
}

void SpiController::applyPhyErrata(uint8_t port) {
	// Module 1: receive performance
	writePhyMmd(port, 0x01, 0x6F, 0xDD0B);
	writePhyMmd(port, 0x01, 0x8F, 0x6032);
	writePhyMmd(port, 0x01, 0x9D, 0x248C);
	writePhyMmd(port, 0x01, 0x75, 0x0060);
	writePhyMmd(port, 0x01, 0xD3, 0x7777);
	writePhyMmd(port, 0x1C, 0x06, 0x3008);
	writePhyMmd(port, 0x1C, 0x08, 0x2000);

	// Module 2: transmit waveform amplitude
	writePhyMmd(port, 0x1C, 0x04, 0x00D0);

	// Module 4: EEE is enabled by default but not functional; disable it
	writePhyMmd(port, 0x07, 0x3C, 0x0000);

	// Module 7: supply current in autonegotiation, 10BASE-T, and 100BASE-TX modes
	writePhyMmd(port, 0x1C, 0x13, 0x6EFF);
	writePhyMmd(port, 0x1C, 0x14, 0xE6FF);
	writePhyMmd(port, 0x1C, 0x15, 0x6EFF);
	writePhyMmd(port, 0x1C, 0x16, 0xE6FF);
	writePhyMmd(port, 0x1C, 0x17, 0x00FF);
	writePhyMmd(port, 0x1C, 0x18, 0x43FF);
	writePhyMmd(port, 0x1C, 0x19, 0xC3FF);
	writePhyMmd(port, 0x1C, 0x1A, 0x6FFF);
	writePhyMmd(port, 0x1C, 0x1B, 0x07FF);
	writePhyMmd(port, 0x1C, 0x1C, 0x0FFF);
	writePhyMmd(port, 0x1C, 0x1D, 0xE7FF);
	writePhyMmd(port, 0x1C, 0x1E, 0xEFFF);
	writePhyMmd(port, 0x1C, 0x20, 0xEEEE);
}

void SpiController::applySwitchErrata() {
	static constexpr uint8_t FIRST_PHY_PORT = 1;
	static constexpr uint8_t LAST_PHY_PORT = 5;
	static constexpr uint8_t PHY_CONTROL = 0x00; // IEEE register 0 at 0xN100
	static constexpr uint16_t FORCE_100_FULL_NO_ANEG = 0x2100;
	static constexpr uint16_t ANEG_ENABLE_RESTART = 0x1340;

	// The errata require the PHY forced to 100 Mb/s with autonegotiation off while its
	// MMD registers are written, then autonegotiation re-enabled and restarted.
	for (uint8_t port = FIRST_PHY_PORT; port <= LAST_PHY_PORT; port++)
		writeRegister16(port, 1, PHY_CONTROL, FORCE_100_FULL_NO_ANEG);

	for (uint8_t port = FIRST_PHY_PORT; port <= LAST_PHY_PORT; port++)
		applyPhyErrata(port);

	// Module 10: pause frames for ingress rate limiting on an EEE link (undocumented globals)
	writeRegister16(0, 3, 0xC0, 0x4090);
	writeRegister16(0, 3, 0xC2, 0x0080);
	writeRegister16(0, 3, 0xC4, 0x2000);

	// Module 11: collision-based rather than CRS-based back pressure
	writeRegister(0, 3, 0x31, 0xD0);

	for (uint8_t port = FIRST_PHY_PORT; port <= LAST_PHY_PORT; port++)
		writeRegister16(port, 1, PHY_CONTROL, ANEG_ENABLE_RESTART);
}

uint8_t SpiController::readPortStatus(uint8_t port) {
	return readRegister(port, 0, 0x30);
}

bool SpiController::readInternalPhyLink(uint8_t port) {
	// IEEE basic status is at 0xN102. Its link bit latches low on a link drop, so the
	// first read reports history and the second the current state.
	readRegister16(port, 1, 0x02);
	return readRegister16(port, 1, 0x02) & _BV(2);
}
