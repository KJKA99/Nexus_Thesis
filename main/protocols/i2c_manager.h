#ifndef I2C_MANAGER_H
#define I2C_MANAGER_H

#include "driver/i2c.h"
#include "client.h" // Include the specific Golioth header
#include "log.h"    // Include Golioth logging header
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include "golioth_manager.h"
// #include "sd_card.h"        // Include SD card header

#define I2C_MASTER_SDA_IO 21           // Sets the GPIO number for the serial data bus (SDA)
#define I2C_MASTER_SCL_IO 22           // Sets the GPIO number for the serial clock bus (SCL)
#define I2C_MASTER_FREQ_HZ 100000      // Sets the I2C frequency
#define I2C_MASTER_TX_BUF_DISABLE 0    // Disables the buffer for the transmission
#define I2C_MASTER_RX_BUF_DISABLE 0    // Disables the buffer for the reception
#define I2C_MASTER_BUS_SELECT I2C_NUM_0 // Selects the I2C bus (port number)
#define I2C_MASTER_GLITCH_FILTER 7     // Sets the glitch filter for the I2C bus, everything shorter than 7 cycles is considered a glitch and is filtered out
#define MAX_DEVICES 6                  // Sets the maximum number of I2C devices that can be connected to Nexus board

// Function prototypes
void i2c_init();
void i2c_write_data(uint8_t device_address, uint8_t* data, size_t data_len);
void i2c_read_data(uint8_t device_address, uint8_t* read_buf, size_t read_len);
void i2c_scan_bus();
bool i2c_establish_connection(uint8_t device_address);
void i2c_manage_devices();
void golioth_perform_action(const char* instruction);
void log_to_golioth_and_sd(const char* log_message);

// Interrupt Service Routine to handle instructions from the Golioth client
void IRAM_ATTR golioth_instruction_received();

// Initialize the I2C master
void i2c_init();

// Write data to the I2C bus
void i2c_write_data(uint8_t device_address, uint8_t* data, size_t data_len);

// Read data from the I2C bus
void i2c_read_data(uint8_t device_address, uint8_t* read_buf, size_t read_len);

// Scan the I2C bus for devices
void i2c_scan_bus();

// Establish a connection with a device at a given address
bool i2c_establish_connection(uint8_t address);

// Manage the devices connected to the I2C bus
void i2c_manage_devices();

// Probe the I2C bus and check if a device is present
bool i2c_master_probe(uint8_t address);

// Verify if a device is connected at a given address
bool i2c_master_verify_connection(uint8_t address);

// Main I2C function
void i2c_main();

#endif // I2C_MANAGER_H
