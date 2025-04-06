#include "Terminal.h"

Terminal::Terminal(SpiController& spi) 
    : spiController(spi), inputIndex(0) {
    memset(inputBuffer, 0, MAX_COMMAND_LENGTH);
}

void Terminal::begin() {
    Serial.println("Terminal initialized. Type 'help' for available commands.");
    printPrompt();
}

void Terminal::processInput() {
    while (Serial.available()) {
        char c = Serial.read();
        
        // Handle backspace
        if (c == '\b' || c == 127) {
            if (inputIndex > 0) {
                inputIndex--;
                Serial.print("\b \b");
            }
            continue;
        }
        
        // Handle newline/return
        if (c == '\n' || c == '\r') {
            Serial.println();
            if (inputIndex > 0) {
                processCommand();
            }
            printPrompt();
            continue;
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

void Terminal::handleHelpCommand() {
    Serial.println("Available commands:");
    Serial.println("  read <address> <count>  - Read <count> bytes from <address>");
    Serial.println("                           Example: read 0x01FF 3");
    Serial.println("  write <address> <value> - Write <value> to <address>");
    Serial.println("                           Example: write 0x01FF 0xC0");
    Serial.println("  help                    - Show this help message");
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
    if (!str || strncmp(str, "0x", 2) != 0) {
        return 0;
    }
    
    char* endptr;
    unsigned long value = strtoul(str, &endptr, 16);
    
    // Check if parsing was successful and value is within byte range
    if (*endptr == '\0' && value <= 0xFF) {
        success = true;
        return static_cast<uint8_t>(value);
    }
    
    return 0;
}

int Terminal::parseDecimal(const char* str) {
    if (!str) {
        return -1;
    }
    return atoi(str);
} 