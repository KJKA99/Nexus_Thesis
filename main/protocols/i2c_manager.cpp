#ifndef I2C_MANAGER_H
#define I2C_MANAGER_H

#include "driver/i2c.h"
#include "client.h" // Include the specific Golioth header
#include "log.h"    // Include Golioth logging header
// #include "sd_card.h"        // Include SD card header

#define I2C_MASTER_SDA_IO 21           // Sets the GPIO number for the serial data bus (SDA)
#define I2C_MASTER_SCL_IO 22           // Sets the GPIO number for the serial clock bus (SCL)
#define I2C_MASTER_FREQ_HZ 100000      // Sets the I2C frequency
#define I2C_MASTER_TX_BUF_DISABLE 0    // Disables the buffer for the transmission
#define I2C_MASTER_RX_BUF_DISABLE 0    // Disables the buffer for the reception
#define I2C_MASTER_BUS_SELECT I2C_NUM_0 // Selects the I2C bus (port number)
#define I2C_MASTER_GLITCH_FILTER 7     // Sets the glitch filter for the I2C bus, everything shorter than 7 cycles is considered a glitch and is filtered out
#define MAX_DEVICES 6                  // Sets the maximum number of I2C devices that can be connected to Nexus board


static uint8_t devices[MAX_DEVICES]; // Creates an array to store the addresses of the devices found on the I2C bus
static int device_count = 0;         // Initializes a variable to store the number of devices found

// Function prototypes
void i2c_init(uint8_t device_address, uint8_t* data, size_t data_len);
void i2c_RW_data(uint8_t device_address, uint8_t* data, size_t data_len, uint8_t* read_buf, size_t read_len);
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

    i2c_param_config(I2C_MASTER_BUS_SELECT, &bus_config);
    i2c_driver_install(I2C_MASTER_BUS_SELECT, bus_config.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

// Read and Write data on the I2C bus
void i2c_RW_data(uint8_t device_address, uint8_t* data, size_t data_len, uint8_t* read_buf, size_t read_len) {
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

    if (ret == ESP_OK && read_buf != NULL && read_len > 0) {
        cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (device_address << 1) | I2C_MASTER_READ, true);
        i2c_master_read(cmd, read_buf, read_len, I2C_MASTER_LAST_NACK);
        i2c_master_stop(cmd);
        ret = i2c_master_cmd_begin(I2C_MASTER_BUS_SELECT, cmd, 1000 / portTICK_RATE_MS);
        i2c_cmd_link_delete(cmd);

        if (ret == ESP_OK) {
            snprintf(log_message, sizeof(log_message), "Data received from device 0x%02x: %x %x", device_address, read_buf[0], read_buf[1]);
        } else {
            snprintf(log_message, sizeof(log_message), "Data reception from device 0x%02x failed", device_address);
        }
        log_to_golioth_and_sd(log_message);
    }
}

// Scan the I2C bus for devices
void i2c_scan_bus() {
    device_count = 0; // Reset device count

    for (int i = 0; i < 128 && device_count < MAX_DEVICES; i++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create(); // Creates an I2C command link
        i2c_master_start(cmd);                        // Sends a start condition on the I2C bus
        i2c_master_write_byte(cmd, (i << 1) | I2C_MASTER_WRITE, true); // Writes the address of the device to the I2C bus
        i2c_master_stop(cmd);                         // Sends a stop condition on the I2C bus
        esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_BUS_SELECT, cmd, 1000 / portTICK_RATE_MS); // Sends the I2C command
        i2c_cmd_link_delete(cmd);                     // Deletes the I2C command link

        if (ret == ESP_OK) {
            devices[device_count++] = i;              // Stores the address of the device in the array
            char log_message[64];
            snprintf(log_message, sizeof(log_message), "Found device at address 0x%02x", i); // Prints the address of the device
            log_to_golioth_and_sd(log_message);
        }
    }
}

// Establish connection with a device
bool i2c_establish_connection(uint8_t device_address) {
    for (int attempts = 0; attempts < 2; attempts++) {
        i2c_scan_bus();
        for (int i = 0; i < device_count; i++) {
            if (devices[i] == device_address) {
                char log_message[64];
                snprintf(log_message, sizeof(log_message), "Connection established with device at address 0x%02x", device_address);
                log_to_golioth_and_sd(log_message);
                return true;
            }
        }
    }

    char log_message[64];
    snprintf(log_message, sizeof(log_message), "Failed to establish connection with device at address 0x%02x", device_address);
    log_to_golioth_and_sd(log_message);
    return false;
}

// Manage I2C devices
void i2c_manage_devices() {
    while (1) {
        if (device_count == 0) {
            log_to_golioth_and_sd("No I2C modules found on the Nexus board");
        } else {
            for (int i = 0; i < device_count; i++) {
                if (devices[i] != 0) {
                    char log_message[64];
                    snprintf(log_message, sizeof(log_message), "Establishing connection with device at address 0x%02x", devices[i]);
                    log_to_golioth_and_sd(log_message);

                    if (i2c_establish_connection(devices[i])) {
                        snprintf(log_message, sizeof(log_message), "Connection established with device at address 0x%02x", devices[i]);
                        log_to_golioth_and_sd(log_message);
                        // Perform read/write operations
                        uint8_t data[2] = {0x01, 0x02}; // Example data
                        uint8_t read_buf[2];
                        i2c_RW_data(devices[i], data, sizeof(data), read_buf, sizeof(read_buf));
                    } else {
                        snprintf(log_message, sizeof(log_message), "Failed to establish connection with device at address 0x%02x", devices[i]);
                        log_to_golioth_and_sd(log_message);
                        devices[i] = 0; // Remove the address of the device from the array
                        device_count--;
                    }
                }
            }
        }
        i2c_scan_bus();
        vTaskDelay(1000 / portTICK_RATE_MS); // Delay to avoid busy looping
    }
}

// Perform action based on instruction from Golioth
void golioth_perform_action(const char* instruction) {
    char log_message[128];
    snprintf(log_message, sizeof(log_message), "Received instruction: %s", instruction);
    log_to_golioth_and_sd(log_message);

    // Example: parse instruction to get device address and data
    uint8_t device_address = 0x40; // Example address
    uint8_t data[2] = {0x01, 0x02}; // Example data

    if (i2c_establish_connection(device_address)) {
        i2c_init(device_address, data, sizeof(data));
    } else {
        snprintf(log_message, sizeof(log_message), "Failed to perform action on device at address 0x%02x", device_address);
        log_to_golioth_and_sd(log_message);
    }
}

// Log messages to Golioth and SD card
void log_to_golioth_and_sd(const char* log_message) {
    printf("%s\n", log_message); // Print to console
    // Log to Golioth client
    golioth_log(log_message);
    // Log to SD card (commented out)
    // sd_card_log(log_message);
}

// Main I2C function
void i2c_main() {
    i2c_init();
    i2c_manage_devices();
}

#endif // I2C_MANAGER_H
