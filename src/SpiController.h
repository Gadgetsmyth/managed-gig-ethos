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

	// Burst read/write of a big-endian 16-bit register pair starting at registerAddr.
	uint16_t readRegister16(uint8_t port, uint8_t function, uint8_t registerAddr);
	void writeRegister16(uint8_t port, uint8_t function, uint8_t registerAddr, uint16_t data);

	// Chip identification from the global registers.
	uint16_t readChipId();
	uint8_t readRevision();

	// Port N status register (0xN030): bits 4:3 speed, bit 2 full duplex. For PHY ports
	// this is the negotiated link; for the RGMII ports it mirrors the configured MAC speed.
	uint8_t readPortStatus(uint8_t port);
	static constexpr uint8_t PORT_STATUS_SPEED_SHIFT = 3;
	static constexpr uint8_t PORT_STATUS_SPEED_MASK = 0x03;
	static constexpr uint8_t PORT_STATUS_FULL_DUPLEX = 0x04;

	// Link state of an internal PHY port (1-5), from the latched-low IEEE status bit.
	bool readInternalPhyLink(uint8_t port);

private:
	// Helper function to construct address
	uint16_t constructAddress(uint8_t port, uint8_t function, uint8_t registerAddr);

	// Assert chip select and send the 32-bit command/address phase; endTransfer() releases.
	void startTransfer(uint8_t command, uint16_t address);
	void endTransfer();

	// Indirect (Clause 45 MMD) write to an internal PHY on ports 1-5.
	void writePhyMmd(uint8_t port, uint8_t mmd, uint16_t mmdReg, uint16_t data);

	// KSZ9897R errata workarounds (DS80000758): per internal PHY, then global.
	void applyPhyErrata(uint8_t port);
	void applySwitchErrata();

	// SPI configuration
	const int csPin;
	const uint32_t clockSpeed;
};

#endif // SPICONTROLLER_H