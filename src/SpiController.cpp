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
    writeRegister(6, 3, 1, 0x00);
    writeRegister(7, 3, 1, 0x00);
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