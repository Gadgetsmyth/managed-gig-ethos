#ifndef MDCMDIOCONTROLLER_H
#define MDCMDIOCONTROLLER_H

#include <Arduino.h>

class MdcMdioController {
public:
    // Constructor
    MdcMdioController(int mdcPin, int mdioPin);
    
    // Initialize MDC/MDIO pins
    void begin();
    
    // MDC/MDIO read/write functions
    uint16_t readMdc(uint8_t phyAddr, uint8_t regAddr);
    void writeMdc(uint8_t phyAddr, uint8_t regAddr, uint16_t data);
    void initialize_dual_phy();   

private:
    // Pin configuration
    const int mdcPin;
    const int mdioPin;
    
    // Helper functions for MDC/MDIO protocol
    void pulse_mdc(void);    
    void startPreamble();
    void writeBit(bool bit);
    bool readBit();
    void turnaround(bool isRead);
    uint16_t readData();
    void writeData(uint16_t data);
};

#endif // MDCMDIOCONTROLLER_H 