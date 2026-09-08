#ifndef LINKSYNC_H
#define LINKSYNC_H

#include <Arduino.h>
#include "Board.h"
#include "MdcMdioController.h"
#include "SpiController.h"

// Keeps the KSZ9897R's fixed-speed RGMII MAC ports in step with what the external
// VSC8531 PHYs negotiate. The switch has no way to learn link speed on ports 6 and 7
// itself, so without this a 10 or 100 Mb/s partner on those ports passes no traffic.
class LinkSync {
public:
	LinkSync(MdcMdioController& mdc, SpiController& spi);

	// Poll every external PHY; reprogram its MAC port only when link and autonegotiation
	// are complete and the resolved speed or duplex changed since the last write.
	void poll();

private:
	static constexpr uint8_t EXTERNAL_PORT_COUNT =
		Board::PORT_COUNT - Board::FIRST_EXTERNAL_PORT + 1;

	// IEEE basic status (register 1) bits.
	static constexpr uint16_t BMSR_LINK_UP = 0x0004;
	static constexpr uint16_t BMSR_ANEG_COMPLETE = 0x0020;

	// XMII Port Control 0 (0xN300) and 1 (0xN301) bits.
	static constexpr uint8_t CTRL0_FULL_DUPLEX = 0x40;
	static constexpr uint8_t CTRL0_TX_FLOW_CONTROL = 0x20;
	static constexpr uint8_t CTRL0_SPEED_100 = 0x10;
	static constexpr uint8_t CTRL0_RX_FLOW_CONTROL = 0x08;
	static constexpr uint8_t CTRL1_SPEED_10_100 = 0x40;

	MdcMdioController& mdcController;
	SpiController& spiController;

	// Last values written per external port, so unchanged links cost no SPI traffic.
	uint8_t lastControl0[EXTERNAL_PORT_COUNT];
	uint8_t lastControl1[EXTERNAL_PORT_COUNT];
	bool haveLastValues[EXTERNAL_PORT_COUNT];

	void syncPort(uint8_t port);

	// Translate a VSC8531 auxiliary status word into the two XMII control bytes.
	// Returns false if the speed field is the reserved value.
	static bool mapToXmiiControl(uint16_t auxStatus, uint8_t& control0, uint8_t& control1);
};

#endif // LINKSYNC_H
