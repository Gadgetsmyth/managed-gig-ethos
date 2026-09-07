#ifndef LINKSYNC_H
#define LINKSYNC_H

#include <Arduino.h>
#include "SpiController.h"
#include "MdcMdioController.h"

/** Poll VSC8531 link/speed/duplex and program KSZ9897R XMII registers. */
class LinkSync {
public:
    LinkSync(MdcMdioController& mdc, SpiController& spi);

    /** Poll one PHY; write XMII only if link+aneg are solid and values changed. */
    void pollAndSyncPort(uint8_t phyAddr, uint8_t switchPort);

    /** Poll PHY 0x00 (port 6) and PHY 0x10 (port 7). */
    void pollAndSyncBoth();

private:
    static const uint8_t REG_BMSR = 0x01;
    static const uint8_t REG_AUX = 0x1c;
    static const uint16_t BMSR_LINK = (1u << 2);
    static const uint16_t BMSR_ANEG_COMPLETE = (1u << 5);

    MdcMdioController& mdcController;
    SpiController& spiController;
    uint8_t last_port_301[8];
    uint8_t last_port_300[8];
    bool haveCache[8];

    static bool mapXmii(uint16_t aux, uint8_t& port_301, uint8_t& port_300);
};

#endif // LINKSYNC_H
