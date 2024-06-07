#include "protocols/spi_manager.h"
#include "protocols/i2c_manager.h"
#include "protocols/i2s_manager.h"

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
}
    
    if (strcmp(protocol, "I2S") == 0) {
        if (strcmp(target_address, "0xA0") == 0) {
            i2s_manager.i2s_establish_connection(target_address) &&
            humidity_module(action, duration, interval, store_in_lightDB);
        }
        // Add other I2C modules here
    }

    if (strcmp(protocol, "SPI") == 0) {
        if (strcmp(target_address, "0xA0") == 0) {
            spi_manager.spi_establish_connection(target_address) &&
            humidity_module(action, duration, interval, store_in_lightDB);
        }
        // Add other I2C modules here
    }
    // Add other protocols (SPI, I2S) and their corresponding modules here
}