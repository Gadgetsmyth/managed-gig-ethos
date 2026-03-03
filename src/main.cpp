// #include <Arduino.h>

// void setup()
// {
//     pinMode(0, OUTPUT);
//     pinMode(1, OUTPUT);
// }

// void loop()
// {
//     digitalWrite(0, HIGH);
//     digitalWrite(1, HIGH);
//     delay(100);
//     digitalWrite(0, LOW);
//     digitalWrite(1, LOW);
//     delay(100);
// }

#include <Arduino.h>
#include "SpiController.h"
#include "MdcMdioController.h"
#include "Terminal.h"

// Create SPI controller and Terminal instances
SpiController spiController;
// MDC on PC5 (A5), MDIO on PC4 (A4)
MdcMdioController mdcController(A5, A4);
Terminal terminal(spiController, mdcController);

void setup()
{
    // Initialize serial communication at 9600 baud
    Serial.begin(57600);

    // Initialize SPI
    spiController.begin();

    // Initialize MDC/MDIO
    mdcController.begin();

    // Initialize terminal
    terminal.begin();

    mdcController.initialize_dual_phy();
}

void loop()
{
    // Process any incoming terminal commands
    terminal.processInput();
}