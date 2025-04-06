#include <Arduino.h>
#include "SpiController.h"
#include "Terminal.h"

// Create SPI controller and Terminal instances
SpiController spiController;
Terminal terminal(spiController);

void setup()
{
    // Initialize serial communication at 9600 baud
    Serial.begin(9600);
    
    // Initialize SPI
    spiController.begin();
    
    // Initialize terminal
    terminal.begin();
}

void loop()
{
    // Process any incoming terminal commands
    terminal.processInput();
}