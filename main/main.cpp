#include <Arduino.h>
#include "i2c_manager.h"
#include "spi_manager.h"
#include "i2s_manager.h"
#include "golioth_handler.h"

// Function prototypes
void Init_I2C();
void Init_SPI();
void Init_I2S();
void Nexus_device_manager(const char* protocol, uint8_t address, const char* instruction);
void I2C_manager();
void SPI_manager();
void I2S_manager();

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        ; // Wait for serial port to connect
    }

    // Initialize communication protocols
    Serial.println("Initializing I2C...");
    i2c_manager.init_i2c();
    i2c_manager.i2c_scan_bus();

    Serial.println("Initializing SPI...");
    Init_SPI();

    Serial.println("Initializing I2S...");
    Init_I2S();

    // Initialize Golioth
    Serial.println("Initializing Golioth...");
    golioth_init();

    Serial.println("Setup complete");
}

void loop() {
    // Check if Golioth input interrupt is triggered
    if (is_golioth_interrupt_triggered()) {
        // Handle Golioth input
        const char* protocol;
        uint8_t address;
        const char* instruction;
        // Retrieve the protocol, address, and instruction from Golioth input
        get_golioth_input(&protocol, &address, &instruction);
        Nexus_device_manager(protocol, address, instruction);
    }

    // Manage devices (non-interrupt driven)
    I2C_manager.i2c_manage_devices();
    SPI_manager();
    I2S_manager();

    delay(1000);  // Adjust delay as needed
}


void Init_SPI() {
    setup_spi();  // Setup SPI devices
}

void Init_I2S() {
    i2s_master_init();
}

void Nexus_device_manager(const char* protocol, uint8_t address, const char* instruction) {
    if (strcmp(protocol, "i2c") == 0) {
        I2C_manager(address, instruction);
    } else if (strcmp(protocol, "spi") == 0) {
        spi_handle_command(instruction, address);
    } else if (strcmp(protocol, "i2s") == 0) {
        I2S_manager(address, instruction);
    }
}

void I2C_manager(uint8_t address = 0, const char* instruction = NULL) {
    // Implement I2C device management logic here
    // Use address and instruction as needed
}

void SPI_manager(uint8_t address = 0, const char* instruction = NULL) {
    // Implement SPI device management logic here
    // Use address and instruction as needed
}

void I2S_manager(uint8_t address = 0, const char* instruction = NULL) {
    // Implement I2S device management logic here
    // Use address and instruction as needed
}
