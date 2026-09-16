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

	// Read-modify-write of the bits in mask.
	void writeRegisterMasked(
		uint8_t port, uint8_t function, uint8_t registerAddr, uint8_t data, uint8_t mask);
	void writeRegister16Masked(
		uint8_t port, uint8_t function, uint8_t registerAddr, uint16_t data, uint16_t mask);

	// Burst read/write of a big-endian 32-bit register starting at registerAddr.
	uint32_t readRegister32(uint8_t port, uint8_t function, uint8_t registerAddr);
	void writeRegister32(uint8_t port, uint8_t function, uint8_t registerAddr, uint32_t data);

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

	// Power an internal PHY (ports 1-5) down or back up, which renegotiates the link.
	void setInternalPhyPowerDown(uint8_t port, bool down);

	// Limit what an internal PHY advertises to one Phy::Speed (or everything for
	// SPEED_AUTO) and restart autonegotiation.
	void setInternalPhySpeed(uint8_t port, uint8_t speed);

	// Port MSTP state (0xNB04): let the port forward traffic and learn addresses, or
	// block both so a disabled port is silent even if its link comes up.
	void setPortForwarding(uint8_t port, bool enabled);

	// Port VLAN membership (0xNA04-0xNA07): the set of ports frames received on this
	// port may be forwarded to, as a mask with bit N-1 for port N.
	void setPortMembership(uint8_t port, uint8_t mask);

	// Port mirroring control (0xN800): a port can be the sniffer that receives copies,
	// and/or have its received and transmitted frames copied to the sniffer.
	void setPortMirroring(uint8_t port, bool sniffer, bool mirrorRx, bool mirrorTx);

	// Egress queue split (0xN020 bits 1:0): one queue, or four selected by priority.
	void setPortFourQueues(uint8_t port, bool fourQueues);

	// 802.1p priority classification (0xN801 bit 2): trust the PCP field of tagged frames.
	void setPort8021pClassification(uint8_t port, bool enabled);

	// Default priority 0-7 (0xN802 bits 2:0) for frames no other classifier assigns.
	void setPortDefaultPriority(uint8_t port, uint8_t priority);

	// Port-based ingress/egress rate limits. `code` is a datasheet table 5-3 value:
	// 0 = line rate, 1-10 = that many Mb/s, 11-100 = code x 10 Mb/s on a gigabit link
	// (code x 1 Mb/s on a 100 Mb/s link).
	void setIngressRateLimit(uint8_t port, uint8_t code);
	void setEgressRateLimit(uint8_t port, uint8_t code);

	// Read one MIB counter through the port's indirect access registers. Counters clear
	// on read. Returns bits 31:0; bits 35:32 of the byte counters and the overflow flag
	// come back in `high` (0x10 marks overflow).
	uint32_t readMibCounter(uint8_t port, uint8_t index, uint8_t& high);

	// Zero every MIB counter on every port.
	void clearMibCounters();

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