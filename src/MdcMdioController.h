#ifndef MDCMDIOCONTROLLER_H
#define MDCMDIOCONTROLLER_H

#include <Arduino.h>

// Bit-banged MDC/MDIO (SMI) master for accessing PHY registers.
class MdcMdioController {
public:
	// Expected VSC8531 identifier (IEEE registers 2 and 3) with the revision nibble masked off.
	static constexpr uint32_t VSC8531_PHY_ID = 0x00070570;
	static constexpr uint32_t PHY_ID_REVISION_MASK = 0x0000000F;

	MdcMdioController(int mdcPin, int mdioPin);

	// Configure the MDC/MDIO pins.
	void begin();

	// PHY register access over the MDC/MDIO bus.
	uint16_t readRegister(uint8_t phyAddr, uint8_t regAddr);
	void writeRegister(uint8_t phyAddr, uint8_t regAddr, uint16_t data);
	void writeRegisterMasked(uint8_t phyAddr, uint8_t regAddr, uint16_t data, uint16_t mask);

	// Read IEEE registers 2 and 3 as one 32-bit identifier, register 2 in the high half.
	uint32_t readPhyId(uint8_t phyAddr);

	// Apply the dual-PHY bring-up register sequence to the PHY at phyAddr.
	void initializeDualPhy(uint8_t phyAddr = 0x00);

private:
	const int mdcPin;
	const int mdioPin;

	// Drive one MDC clock cycle, presenting mdioValue on MDIO.
	void clockBit(bool mdioValue);
};

#endif // MDCMDIOCONTROLLER_H
