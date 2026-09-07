#ifndef SPICONTROLLER_H
#define SPICONTROLLER_H

#include <Arduino.h>
#include <SPI.h>

class SpiController {
public:
	// Expected KSZ9897R chip ID: global register 0x0001 (MSB) and 0x0002 (LSB).
	static constexpr uint16_t KSZ9897_CHIP_ID = 0x9897;

	// Constructor
	SpiController(int csPin = 10, uint32_t clockSpeed = 1000000);

	// Initialize SPI
	void begin();

	// Read/write functions
	uint8_t readRegister(uint8_t port, uint8_t function, uint8_t registerAddr);
	void writeRegister(uint8_t port, uint8_t function, uint8_t registerAddr, uint8_t data);

	// Chip identification from the global registers.
	uint16_t readChipId();
	uint8_t readRevision();

private:
	// Helper function to construct address
	uint16_t constructAddress(uint8_t port, uint8_t function, uint8_t registerAddr);

	// SPI configuration
	const int csPin;
	const uint32_t clockSpeed;
};

#endif // SPICONTROLLER_H