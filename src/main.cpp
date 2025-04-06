#include <Arduino.h>
#include <SPI.h>

// SPI configuration
const int SPI_CS_PIN = 10;  // PB2 on ATmega328P (Arduino pin 10)
const uint32_t SPI_CLOCK = 1000000;  // 1 MHz clock

// Function to construct 16-bit address from components
uint16_t constructAddress(uint8_t port, uint8_t function, uint8_t registerAddr) {
    // Validate inputs
    if (port > 7) port = 0;  // Force to global if invalid port
    if (function > 0x0F) function = 0;  // Limit to 4 bits
    if (registerAddr > 0xFF) registerAddr = 0;  // Limit to 8 bits
    
    // Construct address: 0b0PPP FFFF RRRRRRRR
    return ((port & 0x07) << 12) | ((function & 0x0F) << 8) | (registerAddr & 0xFF);
}

// Function to write data to a register
void writeRegister(uint8_t port, uint8_t function, uint8_t registerAddr, uint8_t data) {
    uint16_t address = constructAddress(port, function, registerAddr);
    
    // Begin SPI transaction
    SPI.beginTransaction(SPISettings(SPI_CLOCK, MSBFIRST, SPI_MODE0));
    digitalWrite(SPI_CS_PIN, LOW);
    
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
    digitalWrite(SPI_CS_PIN, HIGH);
    SPI.endTransaction();
}

// Function to read data from a register
uint8_t readRegister(uint8_t port, uint8_t function, uint8_t registerAddr) {
    uint16_t address = constructAddress(port, function, registerAddr);
    
    // Begin SPI transaction
    SPI.beginTransaction(SPISettings(SPI_CLOCK, MSBFIRST, SPI_MODE0));
    digitalWrite(SPI_CS_PIN, LOW);
    
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
    digitalWrite(SPI_CS_PIN, HIGH);
    SPI.endTransaction();
    
    return data;
}

void setup()
{
    // Initialize serial communication at 9600 baud
    Serial.begin(9600);
    
    // Initialize SPI
    pinMode(SPI_CS_PIN, OUTPUT);
    digitalWrite(SPI_CS_PIN, HIGH);  // Deselect device
    SPI.begin();
}

void loop()
{
    // Example usage of read/write functions
    // Write value 0x42 to port 1, function 2, register 3
    // writeRegister(1, 2, 3, 0x42);
    
    // Read from global port 0, function 0, register 0 and increase the register address each time
    uint8_t readValue = readRegister(0, 0, 0);
    // Print the read value
    Serial.print("Read register 0 value: 0x");
    Serial.println(readValue, HEX);

    readValue = readRegister(0, 0, 1);
    Serial.print("Read register 1 value: 0x");
    Serial.println(readValue, HEX);

    readValue = readRegister(0, 0, 2);
    Serial.print("Read register 2 value: 0x");
    Serial.println(readValue, HEX);
    
    // Wait for one second
    delay(1000);
}