#include "LinkSync.h"

LinkSync::LinkSync(MdcMdioController& mdc, SpiController& spi)
    : mdcController(mdc), spiController(spi)
{
    for (uint8_t i = 0; i < 8; i++) {
        last_port_301[i] = 0;
        last_port_300[i] = 0;
        haveCache[i] = false;
    }
}

bool LinkSync::mapXmii(uint16_t aux, uint8_t& port_301, uint8_t& port_300)
{
    const uint8_t speed = (uint8_t)((aux >> 3) & 0x03);
    const bool fullDuplex = (aux & (1u << 5)) != 0;

    if (speed == 0x03)
        return false;

    if (speed == 0x02) {
        port_301 = 0x00;
        port_300 = fullDuplex ? 0x78 : 0x38;
    } else if (speed == 0x01) {
        port_301 = 0x40;
        port_300 = fullDuplex ? 0x78 : 0x38;
    } else {
        port_301 = 0x40;
        port_300 = fullDuplex ? 0x68 : 0x28;
    }
    return true;
}

void LinkSync::pollAndSyncPort(uint8_t phyAddr, uint8_t switchPort)
{
    if (switchPort > 7)
        return;

    const uint16_t bmsr = mdcController.readMdc(phyAddr, REG_BMSR);
    if ((bmsr & (BMSR_LINK | BMSR_ANEG_COMPLETE)) != (BMSR_LINK | BMSR_ANEG_COMPLETE))
        return;

    const uint16_t aux = mdcController.readMdc(phyAddr, REG_AUX);
    uint8_t port_301 = 0;
    uint8_t port_300 = 0;
    if (!mapXmii(aux, port_301, port_300))
        return;

    if (haveCache[switchPort] && last_port_301[switchPort] == port_301 && last_port_300[switchPort] == port_300)
        return;

    spiController.writeRegister(switchPort, 3, 0x01, port_301);
    spiController.writeRegister(switchPort, 3, 0x00, port_300);
    last_port_301[switchPort] = port_301;
    last_port_300[switchPort] = port_300;
    haveCache[switchPort] = true;
}

void LinkSync::pollAndSyncBoth()
{
    pollAndSyncPort(0x00, 6);
    pollAndSyncPort(0x10, 7);
}
