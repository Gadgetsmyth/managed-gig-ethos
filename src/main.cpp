#include <Arduino.h>
#include "SpiController.h"
#include "MdcMdioController.h"
#include "Terminal.h"

// Create SPI controller and Terminal instances
SpiController spiController;
// MDC on PC4 (A4), MDIO on PC5 (A5)
MdcMdioController mdcController(A5, A4);
Terminal terminal(spiController, mdcController);
const int resetPin = A1;

void setup()
{
    pinMode(resetPin, OUTPUT);
    digitalWrite(resetPin, LOW);
    // Initialize serial communication at 9600 baud
    Serial.begin(57600);

    // Initialize SPI
    spiController.begin();

    // delay for the clock to be stable
    delay(10);


    digitalWrite(resetPin, HIGH);
    // delay according to dual phy reset data sheet requirements
    delay(110);
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