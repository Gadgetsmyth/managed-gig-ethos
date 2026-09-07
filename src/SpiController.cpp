#include "SpiController.h"

SpiController::SpiController(int csPin, uint32_t clockSpeed)
    : csPin(csPin), clockSpeed(clockSpeed)
{
}

void SpiController::begin()
{
    // Initialize SPI
    pinMode(csPin, OUTPUT);
    digitalWrite(csPin, HIGH); // Deselect device
    SPI.begin();

    // Write to the 0x7301 and 0x6301 the value of 0x00
    // Enables RGMII mode
    writeRegister(6, 3, 1, 0x00);
    writeRegister(7, 3, 1, 0x00);
    switchErrataCorrections();
}


void SpiController::writeMmdPhy16(uint8_t port, uint8_t phyReg, uint16_t data)
{
    writeRegister(port, 1, (uint8_t)(phyReg * 2), (uint8_t)(data >> 8));
    writeRegister(port, 1, (uint8_t)(phyReg * 2 + 1), (uint8_t)(data & 0xFF));
}

void SpiController::writeMmd(uint8_t port, uint8_t mmd, uint16_t reg, uint16_t data)
{
    const uint16_t devad = (uint16_t)(mmd & 0x1F);
    writeMmdPhy16(port, 0x0D, devad);
    writeMmdPhy16(port, 0x0E, reg);
    writeMmdPhy16(port, 0x0D, (uint16_t)(0x4000 | devad));
    writeMmdPhy16(port, 0x0E, data);
}

void SpiController::writePortMmdErrata(uint8_t port)
{
    // Phy RX performance improvements
    writeMmd(port, 0x01, 0x6F, 0xDD0B);
    writeMmd(port, 0x01, 0x8F, 0x6032);
    writeMmd(port, 0x01, 0x9D, 0x248C);
    writeMmd(port, 0x01, 0x75, 0x0060);
    writeMmd(port, 0x01, 0xD3, 0x7777);
    writeMmd(port, 0x1C, 0x06, 0x3008);
    writeMmd(port, 0x1C, 0x08, 0x2000);

    // PYH TX waveform amplitude correction
    writeMmd(port, 0x1C, 0x04, 0x00D0);

    // DISABLE EEE
    writeMmd(port, 0x07, 0x3C, 0x0000);

    // Adjust Power Consumption Registers
    writeMmd(port, 0x1C, 0x13, 0x6EFF);
    writeMmd(port, 0x1C, 0x14, 0xE6FF);
    writeMmd(port, 0x1C, 0x15, 0x6EFF);
    writeMmd(port, 0x1C, 0x16, 0xE6FF);
    writeMmd(port, 0x1C, 0x17, 0x00FF);
    writeMmd(port, 0x1C, 0x18, 0x43FF);
    writeMmd(port, 0x1C, 0x19, 0xC3FF);
    writeMmd(port, 0x1C, 0x1A, 0x6FFF);
    writeMmd(port, 0x1C, 0x1B, 0x07FF);
    writeMmd(port, 0x1C, 0x1C, 0x0FFF);
    writeMmd(port, 0x1C, 0x1D, 0xE7FF);
    writeMmd(port, 0x1C, 0x1E, 0xEFFF);
    writeMmd(port, 0x1C, 0x20, 0xEEEE);
}

void SpiController::switchErrataCorrections()
{
    // Set the every port to 100mbit, with autonegotiation disabled to correct errata
    for (int port = 1; port < 6; port++) {
        // write 0x2100 to 0xn100-0xn101
        writeRegister(port, 1, 0, 0x21);
        writeRegister(port, 1, 1, 0x00);
    }

    for (int port = 1; port < 6; port++)
        writePortMmdErrata((uint8_t)port);

    // Assign Global resgisters to compensate for pause frames
    writeRegister(0, 3, 0xC0, 0x40);
    writeRegister(0, 3, 0xC1, 0x90);
    writeRegister(0, 3, 0xC2, 0x00);
    writeRegister(0, 3, 0xC3, 0x80);
    writeRegister(0, 3, 0xC4, 0x20);
    writeRegister(0, 3, 0xC5, 0x00);

    // Assign Global resgister to correct back pressure faults
    writeRegister(0, 3, 0x31, 0xD0);



    // Re-enable autonegotiation
    for (int port = 1; port < 6; port++) {
        writeRegister(port, 1, 0, 0x13);
        writeRegister(port, 1, 1, 0x40);
    }
}

uint16_t SpiController::constructAddress(uint8_t port, uint8_t function, uint8_t registerAddr)
{
    // Validate inputs
    if (port > 7)
        port = 0; // Force to global if invalid port
    if (function > 0x0F)
        function = 0; // Limit to 4 bits
    if (registerAddr > 0xFF)
        registerAddr = 0; // Limit to 8 bits

    // Construct address: 0b0PPP FFFF RRRRRRRR
    return ((port & 0x07) << 12) | ((function & 0x0F) << 8) | (registerAddr & 0xFF);
}

void SpiController::writeRegister(uint8_t port, uint8_t function, uint8_t registerAddr, uint8_t data)
{
    uint16_t address = constructAddress(port, function, registerAddr);

    // Begin SPI transaction
    SPI.beginTransaction(SPISettings(clockSpeed, MSBFIRST, SPI_MODE0));
    digitalWrite(csPin, LOW);

    // Send write command (010) and address
    // First byte: 010xxxxx (command + 5bits of dont care)
    SPI.transfer(0x40);

    // Second byte: (3 bits of dont care) + 5 bits of address (A15-A11)
    SPI.transfer((address >> 11) & 0x1F);

    // Third byte: 8 bits of middle of address (A10-A3)
    SPI.transfer(((address >> 3) & 0xFF));

    // Fourth byte: 3 bits of bottom of address (A2-A0) + 5 bits of dont care
    SPI.transfer((address << 5) & 0xE0);

    // Send data
    SPI.transfer(data);

    // End transaction
    digitalWrite(csPin, HIGH);
    SPI.endTransaction();
}

uint8_t SpiController::readRegister(uint8_t port, uint8_t function, uint8_t registerAddr)
{
    uint16_t address = constructAddress(port, function, registerAddr);

    // Begin SPI transaction
    SPI.beginTransaction(SPISettings(clockSpeed, MSBFIRST, SPI_MODE0));
    digitalWrite(csPin, LOW);

    // Send read command (011) and address
    // First byte: 011xxxxx (command + 5 bits of dont care)
    SPI.transfer(0x60);

    // Second byte: (3 bits of dont care) + 5 bits of address (A15-A11)
    SPI.transfer((address >> 11) & 0x1F);

    // Third byte: 8 bits of middle of address (A10-A3)
    SPI.transfer(((address >> 3) & 0xFF));

    // Fourth byte: 3 bits of bottom of address (A2-A0) + 5 bits of dont care
    SPI.transfer((address << 5) & 0xE0);

    // Read data
    uint8_t data = SPI.transfer(0x00);

    // End transaction
    digitalWrite(csPin, HIGH);
    SPI.endTransaction();

    return data;
}