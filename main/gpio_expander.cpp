#include "gpio_expander.h"
#include <Arduino.h>



// Define the MCP23S17 instance
MCP23S17 mcp(CS_PIN);

void initializeMCP23S17() {
    // Initialize SPI communication
    SPI.begin();

    // Initialize the MCP23S17
    if (mcp.begin()) {
        Serial.println("MCP23S17 Initialized successfully.");
    } else {
        Serial.println("Failed to initialize MCP23S17.");
    }

    // Configure the GPIO expander pins
    mcp.pinMode1(0, OUTPUT);  // GPB0
    mcp.pinMode1(1, OUTPUT);  // GPB1
    mcp.pinMode1(2, OUTPUT);  // GPB2
    mcp.pinMode1(3, OUTPUT);  // GPB3
    mcp.pinMode1(4, OUTPUT);  // GPB4
    mcp.pinMode1(5, OUTPUT);  // GPB5
    mcp.pinMode1(6, OUTPUT);  // GPB6
    mcp.pinMode1(7, OUTPUT);  // GPB7
    mcp.pinMode1(21, OUTPUT); // GPA0, SPI CS 1
    mcp.pinMode1(22, OUTPUT); // GPA1, SPI CS 2
    mcp.pinMode1(23, OUTPUT); // GPA2, SPI CS 3
    mcp.pinMode1(24, OUTPUT); // GPA3, SPI CS 4
    mcp.pinMode1(25, OUTPUT); // GPA4, SPI CS 5
    mcp.pinMode1(26, OUTPUT); // GPA5, SPI CS 6
    mcp.pinMode1(27, OUTPUT); // GPA6
    mcp.pinMode1(28, OUTPUT); // GPA7

    // Set default states as needed
    mcp.write1(0, LOW);
    mcp.write1(1, LOW);
    mcp.write1(2, LOW);
    mcp.write1(3, LOW);
    mcp.write1(4, LOW);
    mcp.write1(5, LOW);
    mcp.write1(6, LOW);
    mcp.write1(7, LOW);
    mcp.write1(21, HIGH); // CS pins typically HIGH when not in use
    mcp.write1(22, HIGH);
    mcp.write1(23, HIGH);
    mcp.write1(24, HIGH);
    mcp.write1(25, HIGH);
    mcp.write1(26, HIGH);
    mcp.write1(27, LOW);
    mcp.write1(28, LOW);
}

void setMCP23S17PinMode(uint8_t pin, uint8_t mode) {
    mcp.pinMode1(pin, mode);
}

void writeMCP23S17Pin(uint8_t pin, uint8_t value) {
    mcp.write1(pin, value);
}

int readMCP23S17Pin(uint8_t pin) {
    return mcp.read1(pin);
}
