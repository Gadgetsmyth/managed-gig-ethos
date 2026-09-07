#ifndef SPICONTROLLER_H
#define SPICONTROLLER_H

#include <Arduino.h>
#include <SPI.h>

class SpiController {
public:
    // Constructor
    SpiController(int csPin = 10, uint32_t clockSpeed = 1000000);
    
    // Initialize SPI
    void begin();
    
    // Read/write functions
    // port + function + register is 16 bits, and is the actual register address.
    uint8_t readRegister(uint8_t port, uint8_t function, uint8_t registerAddr);
    void writeRegister(uint8_t port, uint8_t function, uint8_t registerAddr, uint8_t data);

private:
    uint16_t constructAddress(uint8_t port, uint8_t function, uint8_t registerAddr);
    void writeMmdPhy16(uint8_t port, uint8_t phyReg, uint16_t data);
    void writeMmd(uint8_t port, uint8_t mmd, uint16_t reg, uint16_t data);
    void writePortMmdErrata(uint8_t port);
    void switchErrataCorrections();
    // SPI configuration
    const int csPin;
    const uint32_t clockSpeed;
};

#endif // SPICONTROLLER_H 