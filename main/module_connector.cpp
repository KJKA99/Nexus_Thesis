#include "protocols/spi_manager.h"
#include "protocols/i2c_manager.h"
#include "protocols/i2s_manager.h"
#include "modules/humidity_module.cpp"
#include "modules/accelerometer_and_gyroscope_module"



void module_connector(const char* protocol, const char* target_address, const char* action, int duration, int interval, bool store_in_lightDB) {
    if (strcmp(protocol, "I2C") == 0) {
        if (strcmp(target_address, "0x6B") == 0) {
            if (i2c_manager.i2c_establish_connection(target_address) == true) {
                battery_ic_module(action, duration, interval, store_in_lightDB);
            }
        }

        if (strcmp(target_address, "0x44") == 0 || strcmp(target_address, "0x45") == 0) {
            if (i2c_manager.i2c_establish_connection(target_address) == true) {
                humidity_module(action, duration, interval, store_in_lightDB);
            }
        }

        if (strcmp(target_address, "0x48") == 0) {
            if (i2c_manager.i2c_establish_connection(target_address) == true) {
                temperature_module(action, duration, interval, store_in_lightDB);
            }
        }

        if (strcmp(target_address, "0x68") == 0 || strcmp(target_address, "0x69") == 0) {
            if (i2c_manager.i2c_establish_connection(target_address) == true) {
                accelerometer_and_gyroscope_module(action, duration, interval, store_in_lightDB);
            }
        }

        if (strcmp(target_address, "0x29") == 0) {
            if (i2c_manager.i2c_establish_connection(target_address) == true) {
                light_module(action, duration, interval, store_in_lightDB);
            }
        }

        if (strcmp(target_address, "0x29") == 0) { // To do: Change address since light module is on address 0x29
            if (i2c_manager.i2c_establish_connection(target_address) == true) {
                proximity_module(action, duration, interval, store_in_lightDB);
            }
        }
    }

    
    if (strcmp(protocol, "I2S") == 0) {
        int possible_sd_pins[] = { GPIO_NUM_22, GPIO_NUM_21, GPIO_NUM_19, GPIO_NUM_18 };
        int num_pins = sizeof(possible_sd_pins) / sizeof(possible_sd_pins[0]);

        for (int i = 0; i < num_pins; i++) {
            int port = 0; // Use I2S_NUM_0 or I2S_NUM_1 based on your design
            i2s_establish_connection(port, possible_sd_pins[i]);

            int device_type = i2s_manager.i2s_identify_device_characteristics(port);
            if (device_type != DEVICE_TYPE_UNKNOWN) {
                printf("Detected device type %d on SD pin %d\n", device_type, possible_sd_pins[i]);
                // Store or process the identified device as needed
            }
        }
    }

    // Add other protocols (SPI, I2S) and their corresponding modules here


    if (strcmp(protocol, "SPI") == 0) {
        for (int i = 0; i < MAX_DEVICES; i++) {
            if (strcmp(common_spi_register[i].module_identifier, "pressure_module") == 0) {
                pressure_module(action, duration, interval, store_in_lightDB);
            }
            // Add other SPI modules here
        }
    }
}