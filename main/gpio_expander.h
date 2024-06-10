#ifndef GPIO_EXPANDER_H
#define GPIO_EXPANDER_H

#include <protocols/spi_manager.h>
#include <libraries/MCP23S17/MCP23S17.h>

// Define the chip select pin
#define CS_PIN 48

// Declare the MCP23S17 instance
extern MCP23S17 mcp;

// Function to initialize the MCP23S17
void initializeMCP23S17();

// Function to set a pin mode on the MCP23S17
void setMCP23S17PinMode(uint8_t pin, uint8_t mode);

// Function to write to a pin on the MCP23S17
void writeMCP23S17Pin(uint8_t pin, uint8_t value);

// Function to read from a pin on the MCP23S17
int readMCP23S17Pin(uint8_t pin);

#endif // GPIO_EXPANDER_H
