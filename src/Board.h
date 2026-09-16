#ifndef BOARD_H
#define BOARD_H

#include <Arduino.h>

// How the ATmega328P is wired to the switch and PHYs on this board. The ATmega runs from
// a 2.5 V rail at 8 MHz, which is inside its rated range with little margin; the brown-out
// threshold must stay at 1.8 V, since 2.7 V holds it in reset.
namespace Board {

// Active-low reset shared by both external PHYs.
constexpr uint8_t PHY_RESET_PIN = A1;

// Bit-banged MDIO bus to the external PHYs.
constexpr uint8_t MDC_PIN = A5;
constexpr uint8_t MDIO_PIN = A4;

// SPI chip select for the KSZ9897R.
constexpr uint8_t SWITCH_CS_PIN = 10;

// KSZ9897R ports 1-5 are its internal PHYs. Ports 6-7 are RGMII to the external VSC8531s.
constexpr uint8_t PORT_COUNT = 7;
constexpr uint8_t FIRST_EXTERNAL_PORT = 6;

// MDIO address of the PHY on each external port, indexed by port - FIRST_EXTERNAL_PORT.
constexpr uint8_t PHY_ADDRESSES[] = {0x00, 0x10};

// Port masks use bit N-1 for port N, the same layout as the switch's own port registers.
constexpr uint8_t ALL_PORTS_MASK = (1 << PORT_COUNT) - 1;

inline uint8_t portBit(uint8_t port) {
	return 1 << (port - 1);
}

inline bool isExternalPort(uint8_t port) {
	return port >= FIRST_EXTERNAL_PORT;
}

inline uint8_t phyAddressForPort(uint8_t port) {
	return PHY_ADDRESSES[port - FIRST_EXTERNAL_PORT];
}

} // namespace Board

#endif // BOARD_H
