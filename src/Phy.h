#ifndef PHY_H
#define PHY_H

#include <Arduino.h>

// IEEE 802.3 clause 22 register definitions shared by the switch's internal PHYs (reached
// over SPI at 0xN100 + 2 * register) and the external VSC8531s (reached over MDIO).
namespace Phy {

constexpr uint8_t REG_CONTROL = 0x00;
constexpr uint8_t REG_ADVERTISE = 0x04;		// 10/100 autonegotiation advertisement
constexpr uint8_t REG_1000T_CONTROL = 0x09; // 1000BASE-T advertisement

constexpr uint16_t CONTROL_POWER_DOWN = 0x0800;
constexpr uint16_t CONTROL_RESTART_ANEG = 0x0200;

// Register 4 bits 8:5: 100 full, 100 half, 10 full, 10 half; bit 10: symmetric pause.
// Register 9 bit 9: 1000 full.
constexpr uint16_t ADVERTISE_PAUSE = 0x0400;
constexpr uint16_t ADVERTISE_10_100_MASK = 0x01E0 | ADVERTISE_PAUSE;
constexpr uint16_t ADVERTISE_1000_MASK = 0x0300;

// Port speed setting: what the port is allowed to negotiate.
enum Speed : uint8_t { SPEED_AUTO = 0, SPEED_10 = 1, SPEED_100 = 2, SPEED_1000 = 3 };

// Pause is always advertised: the switch MACs run with flow control on, and a link
// partner only honours pause frames it negotiated.
inline uint16_t advertise10_100(uint8_t speed) {
	switch (speed) {
	case SPEED_10:
		return 0x0060 | ADVERTISE_PAUSE;
	case SPEED_100:
		return 0x0180 | ADVERTISE_PAUSE;
	case SPEED_1000:
		return ADVERTISE_PAUSE;
	default:
		return ADVERTISE_10_100_MASK;
	}
}

inline uint16_t advertise1000(uint8_t speed) {
	return (speed == SPEED_AUTO || speed == SPEED_1000) ? 0x0200 : 0x0000;
}

} // namespace Phy

#endif // PHY_H
