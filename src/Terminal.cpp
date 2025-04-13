#include "Terminal.h"

Terminal::Terminal(SpiController& spi, MdcMdioController& mdc) 
    : spiController(spi), mdcController(mdc), inputIndex(0) {
    memset(inputBuffer, 0, MAX_COMMAND_LENGTH);
}

void Terminal::begin() {
    Serial.println("Terminal initialized. Type 'help' for available commands.");
    printPrompt();
}

void Terminal::processInput() {
    // Only process if there's data available
    if (Serial.available() > 0) {
        char c = Serial.read();
        
        // Handle backspace
        if (c == '\b' || c == 127) {
            if (inputIndex > 0) {
                inputIndex--;
                Serial.print("\b \b");
            }
            return;
        }
        
        // Handle newline/return
        if (c == '\n' || c == '\r') {
            // Only process if it's a complete line (not just a partial line)
            if (c == '\n' || (c == '\r' && Serial.peek() != '\n')) {
                Serial.println();
                if (inputIndex > 0) {
                    processCommand();
                }
                printPrompt();
            }
            return;
        }
        
        // Add character to buffer if there's space
        if (inputIndex < MAX_COMMAND_LENGTH - 1) {
            inputBuffer[inputIndex++] = c;
            Serial.print(c);
        }
    }
}

void Terminal::processCommand() {
    // Null-terminate the input
    inputBuffer[inputIndex] = '\0';
    
    // Find the first space to separate command from arguments
    char* space = strchr(inputBuffer, ' ');
    char* args = nullptr;
    
    if (space) {
        *space = '\0';
        args = space + 1;
    }
    
    // Process the command
    if (strcmp(inputBuffer, "read") == 0) {
        handleReadCommand(args);
    }
    else if (strcmp(inputBuffer, "write") == 0) {
        handleWriteCommand(args);
    }
    else if (strcmp(inputBuffer, "readmdc") == 0) {
        handleReadMdcCommand(args);
    }
    else if (strcmp(inputBuffer, "writemdc") == 0) {
        handleWriteMdcCommand(args);
    }
    else if (strcmp(inputBuffer, "scanmdc") == 0) {
        handleScanMdcCommand(args);
    }
    else if (strcmp(inputBuffer, "help") == 0) {
        handleHelpCommand();
    }
    else {
        printError("Unknown command. Type 'help' for available commands.");
    }
    
    // Reset input buffer
    inputIndex = 0;
    memset(inputBuffer, 0, MAX_COMMAND_LENGTH);
}

void Terminal::handleReadCommand(const char* args) {
    if (!args) {
        printError("Missing arguments. Usage: read <address> <count>");
        return;
    }
    
    // Parse address
    bool parseSuccess;
    uint16_t address = parseHexAddress(args, parseSuccess);
    if (!parseSuccess) {
        printError("Invalid address format. Use hex (e.g., 0x01FF)");
        return;
    }
    
    // Find count argument
    char* countStr = strchr(args, ' ');
    if (!countStr) {
        printError("Missing count argument. Usage: read <address> <count>");
        return;
    }
    countStr++; // Skip the space
    
    int count = parseDecimal(countStr);
    if (count <= 0 || count > 16) {
        printError("Invalid count. Must be between 1 and 16");
        return;
    }
    
    // Extract port and function from address
    uint8_t port = (address >> 12) & 0x07;
    uint8_t function = (address >> 8) & 0x0F;
    uint8_t registerAddr = address & 0xFF;
    
    // Read and print the values
    Serial.print("Reading ");
    Serial.print(count);
    Serial.print(" bytes from address 0x");
    Serial.println(address, HEX);
    
    for (int i = 0; i < count; i++) {
        uint8_t value = spiController.readRegister(port, function, registerAddr + i);
        Serial.print("0x");
        if (value < 0x10) Serial.print("0");
        Serial.print(value, HEX);
        Serial.print(" ");
    }
    Serial.println();
}

void Terminal::handleWriteCommand(const char* args) {
    if (!args) {
        printError("Missing arguments. Usage: write <address> <value>");
        return;
    }
    
    // Parse address
    bool parseSuccess;
    uint16_t address = parseHexAddress(args, parseSuccess);
    if (!parseSuccess) {
        printError("Invalid address format. Use hex (e.g., 0x01FF)");
        return;
    }
    
    // Find value argument
    char* valueStr = strchr(args, ' ');
    if (!valueStr) {
        printError("Missing value argument. Usage: write <address> <value>");
        return;
    }
    valueStr++; // Skip the space
    
    uint8_t value = parseHexByte(valueStr, parseSuccess);
    if (!parseSuccess) {
        printError("Invalid value format. Use hex (e.g., 0xC0)");
        return;
    }
    
    // Extract port and function from address
    uint8_t port = (address >> 12) & 0x07;
    uint8_t function = (address >> 8) & 0x0F;
    uint8_t registerAddr = address & 0xFF;
    
    // Write the value
    spiController.writeRegister(port, function, registerAddr, value);
    Serial.print("Wrote 0x");
    if (value < 0x10) Serial.print("0");
    Serial.print(value, HEX);
    Serial.print(" to address 0x");
    Serial.println(address, HEX);
}

void Terminal::handleReadMdcCommand(const char* args) {
    if (!args) {
        printError("Missing arguments. Usage: readmdc <phy_addr> <reg_addr>");
        return;
    }
    
    // Parse PHY address
    bool parseSuccess;
    uint8_t phyAddr = parseHexByte(args, parseSuccess);
    if (!parseSuccess) {
        printError("Invalid PHY address format. Use hex (e.g., 0x01)");
        return;
    }
    
    // Find register address argument
    char* regStr = strchr(args, ' ');
    if (!regStr) {
        printError("Missing register address argument. Usage: readmdc <phy_addr> <reg_addr>");
        return;
    }
    regStr++; // Skip the space
    
    uint8_t regAddr = parseHexByte(regStr, parseSuccess);
    if (!parseSuccess) {
        printError("Invalid register address format. Use hex (e.g., 0x01)");
        return;
    }
    
    // Read and print the value
    uint16_t value = mdcController.readMdc(phyAddr, regAddr);
    Serial.print("Read from PHY 0x");
    if (phyAddr < 0x10) Serial.print("0");
    Serial.print(phyAddr, HEX);
    Serial.print(" Register 0x");
    if (regAddr < 0x10) Serial.print("0");
    Serial.print(regAddr, HEX);
    Serial.print(" = 0x");
    if (value < 0x1000) Serial.print("0");
    if (value < 0x100) Serial.print("0");
    if (value < 0x10) Serial.print("0");
    Serial.println(value, HEX);
}

void Terminal::handleWriteMdcCommand(const char* args) {
    if (!args) {
        printError("Missing arguments. Usage: writemdc <phy_addr> <reg_addr> <value>");
        return;
    }
    
    // Parse PHY address
    bool parseSuccess;
    uint8_t phyAddr = parseHexByte(args, parseSuccess);
    if (!parseSuccess) {
        printError("Invalid PHY address format. Use hex (e.g., 0x01)");
        return;
    }
    
    // Find register address argument
    char* regStr = strchr(args, ' ');
    if (!regStr) {
        printError("Missing register address argument. Usage: writemdc <phy_addr> <reg_addr> <value>");
        return;
    }
    regStr++; // Skip the space
    
    uint8_t regAddr = parseHexByte(regStr, parseSuccess);
    if (!parseSuccess) {
        printError("Invalid register address format. Use hex (e.g., 0x01)");
        return;
    }
    
    // Find value argument
    char* valueStr = strchr(regStr, ' ');
    if (!valueStr) {
        printError("Missing value argument. Usage: writemdc <phy_addr> <reg_addr> <value>");
        return;
    }
    valueStr++; // Skip the space
    
    uint16_t value = parseHexAddress(valueStr, parseSuccess);
    if (!parseSuccess) {
        printError("Invalid value format. Use hex (e.g., 0x1234)");
        return;
    }
    
    // Write the value
    mdcController.writeMdc(phyAddr, regAddr, value);
    Serial.print("Wrote 0x");
    if (value < 0x1000) Serial.print("0");
    if (value < 0x100) Serial.print("0");
    if (value < 0x10) Serial.print("0");
    Serial.print(value, HEX);
    Serial.print(" to PHY 0x");
    if (phyAddr < 0x10) Serial.print("0");
    Serial.print(phyAddr, HEX);
    Serial.print(" Register 0x");
    if (regAddr < 0x10) Serial.print("0");
    Serial.println(regAddr, HEX);
}

void Terminal::handleScanMdcCommand(const char* args) {
    // Clear any pending serial data to prevent interference
    //while (Serial.available()) {
    //    Serial.read();
    //}
    
    //Serial.println("  Scanning for PHY devices...");
    //Serial.println("  PHY Addr | Register 0x01 Value");
    //Serial.println("  ---------|-------------------");
    
    bool found = false;
    
    // Scan all possible PHY addresses (0-31)
    for (uint8_t phyAddr = 0; phyAddr < 32; phyAddr++) {
        // Read register 0x01 from this PHY address
        uint16_t value = mdcController.readMdc(phyAddr, 0x00);
        
        // Print the result
        // Serial.print("0x");
        // if (phyAddr < 0x10) Serial.print("0");
        // Serial.print(phyAddr, HEX);
        // Serial.print("    | 0x");
        // if (value < 0x1000) Serial.print("0");
        // if (value < 0x100) Serial.print("0");
        // if (value < 0x10) Serial.print("0");
        // Serial.println(value, HEX);
        
        // phy setup commands , needs to be done on both halves. 
        // or set broadcast mode register and do it on both at the same time, that's better probably.
        // set rgmii mode set bit 12 register 23 (0x17, default is 0x2000)
        // writemdc 0x00 0x17 0x3000
        // soft reset // set bit 15, default is 0x1040
        // writemdc 0x00 0x00 0x9040
        // adjust the clock control
        // configure 20E2 and ( set 31 (0x1F) to 2 ( sets the address space to e2) ) 
        // this means registers 16 through 30 are remapped from the main space.
        // writemdc 0x00 0x1f 0x0002
        // write register 20 (0x14 )in space 2, to adjust the clock delay 
        // bits 2:0 are gtx clock delay and bits 6:4 are the same, 
        // pattern increases by .3ns per bit, with a minimum of .2ns
        // we set it to bit pattern 100 to get 2ns delay,
        // this means we write 0x0055 to the bit field
        // patch uses 0x0011
        // we also need to clear bit 11, the default field is 0x0800 we change it to 0x0044
        // writemdc 0x00 0x14 0x0055


        // Check if we found a valid PHY (value != 0xFFFF)
        if (value != 0x0000) {
            found = true;
            //Serial.print(phyAddr);
            if (phyAddr < 0x10) Serial.print("0");
            Serial.println(phyAddr, HEX);
            break;  // Stop after finding the first valid PHY
        }
        
        // Small delay to prevent overwhelming the serial output
        delay(100);
    }
    
    if (!found) {
        Serial.println("No PHY devices found.");
    }
    
    // Ensure all output is sent
    Serial.flush();
}

void Terminal::handleHelpCommand() {
    Serial.println("Available commands:");
    Serial.println("  read <address> <count>     - Read <count> bytes from <address>");
    Serial.println("                             Example: read 0x01FF 3");
    Serial.println("  write <address> <value>    - Write <value> to <address>");
    Serial.println("                             Example: write 0x01FF 0xC0");
    Serial.println("  readmdc <phy> <reg>       - Read from PHY register");
    Serial.println("                             Example: readmdc 0x01 0x00");
    Serial.println("  writemdc <phy> <reg> <val> - Write to PHY register");
    Serial.println("                             Example: writemdc 0x01 0x00 0x1234");
    Serial.println("  scanmdc                    - Scan for PHY devices");
    Serial.println("  help                       - Show this help message");
}

void Terminal::printPrompt() {
    Serial.print("> ");
}

void Terminal::printError(const char* message) {
    Serial.print("Error: ");
    Serial.println(message);
}

uint16_t Terminal::parseHexAddress(const char* str, bool& success) {
    success = false;
    
    if (!str) {
        return 0;
    }
    
    // Skip any leading whitespace
    while (*str == ' ') str++;
    
    if (strncmp(str, "0x", 2) != 0) {
        return 0;
    }
    
    char* endptr;
    unsigned long value = strtoul(str, &endptr, 16);
    
    // Check if parsing was successful and value is within 16-bit range
    if (*endptr == '\0' || *endptr == ' ') {
        if (value <= 0xFFFF) {
            success = true;
            return static_cast<uint16_t>(value);
        }
    }
    
    return 0;
}

uint8_t Terminal::parseHexByte(const char* str, bool& success) {
    success = false;
    
    if (!str) {
        return 0;
    }
    
    // Skip any leading whitespace
    while (*str == ' ') str++;
    
    if (strncmp(str, "0x", 2) != 0) {
        return 0;
    }
    
    char* endptr;
    unsigned long value = strtoul(str, &endptr, 16);
    
    // Check if parsing was successful and value is within 16-bit range
    if (*endptr == '\0' || *endptr == ' ') {
        if (value <= 0xFF) {
            success = true;
            return static_cast<uint8_t>(value);
        }
    }
    
    return 0;
}

int Terminal::parseDecimal(const char* str) {
    if (!str) {
        return -1;
    }
    return atoi(str);
} 