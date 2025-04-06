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
    uint8_t readRegister(uint8_t port, uint8_t function, uint8_t registerAddr);
    void writeRegister(uint8_t port, uint8_t function, uint8_t registerAddr, uint8_t data);

private:
    // Helper function to construct address
    uint16_t constructAddress(uint8_t port, uint8_t function, uint8_t registerAddr);
    
    // SPI configuration
    const int csPin;
    const uint32_t clockSpeed;
};

#endif // SPICONTROLLER_H 