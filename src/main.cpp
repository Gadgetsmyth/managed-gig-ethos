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
const int resetPin = A1;

void setup()
{
    digitalWrite(resetPin, LOW);
    pinMode(resetPin, OUTPUT);
    // Initialize serial communication at 9600 baud
    Serial.begin(57600);

    // delay for the clock to be stable
    delay(250);

    digitalWrite(resetPin, HIGH);
    // delay according to dual phy reset data sheet requirements
    delay(250);
    // Initialize MDC/MDIO
    mdcController.begin();

    // Initialize terminal
    terminal.begin();

    mdcController.initialize_dual_phy();

    delay(250);

    // Initialize SPI
    spiController.begin();
}

void loop()
{
    // Process any incoming terminal commands
    terminal.processInput();
}