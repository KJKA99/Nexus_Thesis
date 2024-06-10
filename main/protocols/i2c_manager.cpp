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

#define I2C_MASTER_SDA_IO 9           // Sets the GPIO number for the serial data bus (SDA)
#define I2C_MASTER_SCL_IO 10           // Sets the GPIO number for the serial clock bus (SCL)
#define I2C_MASTER_FREQ_HZ 100000      // Sets the I2C frequency
#define I2C_MASTER_TX_BUF_DISABLE 0    // Disables the buffer for the transmission
#define I2C_MASTER_RX_BUF_DISABLE 0    // Disables the buffer for the reception
#define I2C_MASTER_BUS_SELECT I2C_NUM_0 // Selects the I2C bus (port number)
#define I2C_MASTER_GLITCH_FILTER 7     // Sets the glitch filter for the I2C bus, everything shorter than 7 cycles is considered a glitch and is filtered out
#define MAX_DEVICES 6                  // Sets the maximum number of I2C devices that can be connected to Nexus board


static uint8_t devices[MAX_DEVICES]; // Creates an array to store the addresses of the devices found on the I2C bus
static int device_count = 0;         // Initializes a variable to store the number of devices found


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
void IRAM_ATTR golioth_instruction_received() {
    // Trigger the Golioth action handling function
    golioth_perform_action("Received instruction from Golioth");
}

// Initialize the I2C master
void i2c_init() {
    i2c_config_t bus_config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,       // Sets the GPIO number for the serial data bus (SDA)
        .sda_pullup_en = GPIO_PULLUP_DISABLE,  // Disables the internal pull-up resistor
        .scl_io_num = I2C_MASTER_SCL_IO,       // Sets the GPIO number for the serial clock bus (SCL)
        .scl_pullup_en = GPIO_PULLUP_DISABLE,  // Disables the internal pull-up resistor
        .master.clk_speed = I2C_MASTER_FREQ_HZ // Sets the clock frequency for the I2C bus
    };

    i2c_param_config(I2C_MASTER_BUS_SELECT, &bus_config); //
    i2c_driver_install(I2C_MASTER_BUS_SELECT, bus_config.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

// Write data to the I2C bus
void i2c_write_data(uint8_t device_address, uint8_t* data, size_t data_len) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (device_address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, data, data_len, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_BUS_SELECT, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    char log_message[128];
    if (ret == ESP_OK) {
        snprintf(log_message, sizeof(log_message), "Data transmitted to device 0x%02x successfully", device_address);
    } else {
        snprintf(log_message, sizeof(log_message), "Data transmission to device 0x%02x failed", device_address);
    }
    log_to_golioth_and_sd(log_message);
}

// Read data from the I2C bus
void i2c_read_data(uint8_t device_address, uint8_t* read_buf, size_t read_len) {
    esp_err_t ret = i2c_master_read_from_device(I2C_MASTER_BUS_SELECT, device_address, read_buf, read_len, 1000 / portTICK_RATE_MS);

    char log_message[128];
    if (ret == ESP_OK) {
        snprintf(log_message, sizeof(log_message), "Data received from device 0x%02x: %x %x", device_address, read_buf[0], read_buf[1]);
    } else {
        snprintf(log_message, sizeof(log_message), "Data reception from device 0x%02x failed", device_address);
    }
    log_to_golioth_and_sd(log_message);
}

void I2c_scan_bus() {
    for (int i = 1; i < 127; i++) {
        if (i2c_master_probe(i)) {
            if (i2c_addresses.size() < MAX_DEVICES) {
                i2c_addresses.push_back(i);
            }
        }
    }
}

bool I2C_establish_connection(uint8_t address) { // Establishes a connection with a device at a given address
    if (std::find(i2c_addresses.begin(), i2c_addresses.end(), address) == i2c_addresses.end()) { // If the address is not in the list of devices
        I2c_scan_bus();
    }
    if (std::find(i2c_addresses.begin(), i2c_addresses.end(), address) != i2c_addresses.end()) { // If the address is in the list of devices
        return i2c_master_verify_connection(address);
    }
    return false;
}
// Function to manage the devices connected to the I2C bus
void I2C_manage_devices() {
    for (auto it = i2c_addresses.begin(); it != i2c_addresses.end(); ) { // Iterates through the list of devices
        if (!i2c_master_verify_connection(*it)) { // If the device is not connected
            it = i2c_addresses.erase(it); // Removes the device from the list
        } else {
            ++it; // Moves to the next device
        }
    }
    if (i2c_addresses.size() < MAX_DEVICES) { // If the number of devices is less than the maximum
        I2c_scan_bus(); 
    }
}

// Function to probe the I2C bus and check if a device is present
bool i2c_master_probe(uint8_t address) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret == ESP_OK;
}

// Function to verify if a device is connected at a given address
bool i2c_master_verify_connection(uint8_t address) {
    return i2c_master_probe(address);
}




// Main I2C function
void i2c_main() {
    i2c_init();
    i2c_scan_bus();
}

#endif // I2C_MANAGER_H
