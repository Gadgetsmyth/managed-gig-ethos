#include "LinkSync.h"

LinkSync::LinkSync(
	MdcMdioController& mdc, SpiController& spi, Settings& settings, Terminal& terminal)
	: mdcController(mdc), spiController(spi), settings(settings), terminal(terminal),
	  linkUpMask(0) {
	for (uint8_t i = 0; i < EXTERNAL_PORT_COUNT; i++) {
		lastControl0[i] = 0;
		lastControl1[i] = 0;
		haveLastValues[i] = false;
	}
}

void LinkSync::poll() {
	for (uint8_t port = 1; port <= Board::PORT_COUNT; port++) {
		if (Board::isExternalPort(port))
			pollExternalPort(port);
		else
			pollInternalPort(port);
	}
}

bool LinkSync::mapToXmiiControl(uint16_t auxStatus, uint8_t& control0, uint8_t& control1) {
	uint8_t speed = (auxStatus >> MdcMdioController::AUX_STATUS_SPEED_SHIFT) &
		MdcMdioController::AUX_STATUS_SPEED_MASK;
	bool fullDuplex = auxStatus & MdcMdioController::AUX_STATUS_FULL_DUPLEX;

	// 0 = 10 Mb/s, 1 = 100 Mb/s, 2 = 1000 Mb/s, 3 = reserved
	if (speed == 3)
		return false;

	// Control 1 bit 6 selects 10/100 vs 1000; control 0 bit 4 then picks 100 over 10.
	control1 = (speed == 2) ? 0x00 : CTRL1_SPEED_10_100;
	control0 = CTRL0_TX_FLOW_CONTROL | CTRL0_RX_FLOW_CONTROL;
	if (speed != 0)
		control0 |= CTRL0_SPEED_100;
	if (fullDuplex)
		control0 |= CTRL0_FULL_DUPLEX;
	return true;
}

void LinkSync::pollInternalPort(uint8_t port) {
	bool linkUp = spiController.readInternalPhyLink(port);
	uint8_t portStatus = linkUp ? spiController.readPortStatus(port) : 0;
	reportLink(port, linkUp,
		(portStatus >> SpiController::PORT_STATUS_SPEED_SHIFT) &
			SpiController::PORT_STATUS_SPEED_MASK,
		portStatus & SpiController::PORT_STATUS_FULL_DUPLEX);
}

void LinkSync::pollExternalPort(uint8_t port) {
	uint8_t phyAddr = Board::phyAddressForPort(port);
	uint8_t index = port - Board::FIRST_EXTERNAL_PORT;

	// The link bit latches low on a drop. A second read after a "down" reports the
	// current state, so a flap that already recovered is not logged as an outage.
	uint16_t basicStatus = mdcController.readRegister(phyAddr, 0x01);
	if (!(basicStatus & BMSR_LINK_UP))
		basicStatus = mdcController.readRegister(phyAddr, 0x01);

	if (!(basicStatus & BMSR_LINK_UP)) {
		reportLink(port, false, 0, false);
		return;
	}

	uint16_t auxStatus = mdcController.readAuxStatus(phyAddr);
	reportLink(port, true,
		(auxStatus >> MdcMdioController::AUX_STATUS_SPEED_SHIFT) &
			MdcMdioController::AUX_STATUS_SPEED_MASK,
		auxStatus & MdcMdioController::AUX_STATUS_FULL_DUPLEX);

	if (!(basicStatus & BMSR_ANEG_COMPLETE))
		return;

	uint8_t control0 = 0;
	uint8_t control1 = 0;
	if (!mapToXmiiControl(auxStatus, control0, control1))
		return;

	if (haveLastValues[index] && lastControl0[index] == control0 && lastControl1[index] == control1)
		return;

	spiController.writeRegister(port, 3, 0x01, control1);
	spiController.writeRegister(port, 3, 0x00, control0);
	lastControl0[index] = control0;
	lastControl1[index] = control1;
	haveLastValues[index] = true;
}

void LinkSync::reportLink(uint8_t port, bool linkUp, uint8_t speedCode, bool fullDuplex) {
	uint8_t bit = Board::portBit(port);
	if (static_cast<bool>(linkUpMask & bit) == linkUp)
		return;
	if (linkUp)
		linkUpMask |= bit;
	else
		linkUpMask &= ~bit;
	if (settings.data.linkLog)
		terminal.printLinkEvent(port, linkUp, speedCode, fullDuplex);
}
