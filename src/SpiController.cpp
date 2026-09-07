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

uint8_t SpiController::readPortStatus(uint8_t port) {
	return readRegister(port, 0, 0x30);
}

bool SpiController::readInternalPhyLink(uint8_t port) {
	// IEEE basic status is at 0xN102. Its link bit latches low on a link drop, so the
	// first read reports history and the second the current state.
	readRegister16(port, 1, 0x02);
	return readRegister16(port, 1, 0x02) & _BV(2);
}
