// i2s_manager.cpp

#include "i2s_manager.h"
#include "driver/gpio.h"
#include <cstring>

I2SManager::I2SManager() {
    // Initialize I2S manager
    i2s_port0 = I2S_NUM_0; // Example port 0
    i2s_port1 = I2S_NUM_1; // Example port 1
    for (int i = 0; i < MAX_I2S_DEVICES; i++) {
        connected_devices[i] = -1; // -1 indicates no device connected
    }
    current_sd_pin = -1;
    interrupt_pin = GPIO_NUM_4; // Example interrupt pin
    active_device_index = -1;

    // Register ISR handler
    register_isr_handler();
}

I2SManager::~I2SManager() {
    // Cleanup if necessary
    gpio_isr_handler_remove(interrupt_pin);
    i2s_driver_uninstall(i2s_port0);
    i2s_driver_uninstall(i2s_port1);
}
// 
bool I2SManager::i2s_establish_connection(i2s_port_t i2s_num, i2s_config_t *i2s_config, i2s_pin_config_t *pin_config) {
    for (int i = 0; i < MAX_I2S_DEVICES; i++) {
        if (connected_devices[i] == -1) {
            i2s_driver_install(i2s_num, i2s_config, 0, NULL);
            i2s_set_pin(i2s_num, pin_config);
            connected_devices[i] = pin_config->data_out_num;
            return true;
        }
    }
    device_queue.push(pin_config->data_out_num);
    return false;
}

bool I2SManager::i2s_master_receive(i2s_port_t i2s_num, int sd_pin, char *data, size_t *data_len) {
    switch_sd_pin(sd_pin);
    int bytes_read = i2s_read_bytes(i2s_num, (void*)data, *data_len, portMAX_DELAY);
    *data_len = bytes_read;
    return (bytes_read > 0);
}

void I2SManager::handle_interrupt() {
    // Handle the interrupt and verify which device is active
    verify_active_devices();
}

void I2SManager::verify_active_devices() {
    // Verify connected I2S devices
    for (int i = 0; i < MAX_I2S_DEVICES; i++) {
        if (connected_devices[i] != -1) {
            // Implement device verification logic
        }
    }
    manage_device_queue();
}

void I2SManager::switch_sd_pin(int sd_pin) {
    if (sd_pin != current_sd_pin) {
        // Implement GPIO multiplexing logic to switch the SD pin
        // Example code to switch the SD pin
        gpio_set_level(current_sd_pin, 0); // Deactivate current pin
        gpio_set_level(sd_pin, 1); // Activate new pin
        current_sd_pin = sd_pin;
    }
}

void IRAM_ATTR I2SManager::gpio_isr_handler(void* arg) {
    I2SManager* instance = static_cast<I2SManager*>(arg);
    instance->handle_interrupt();
}

void I2SManager::register_isr_handler() {
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    io_conf.pin_bit_mask = (1ULL << interrupt_pin);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&io_conf);

    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(interrupt_pin, gpio_isr_handler, (void*)this);
}

void I2SManager::manage_device_queue() {
    // Check if there are any devices in the queue and if there is an available I2S port
    for (int i = 0; i < MAX_I2S_DEVICES; i++) {
        if (connected_devices[i] == -1 && !device_queue.empty()) {
            int next_device = device_queue.front();
            device_queue.pop();
            // Establish connection with the next device from the queue
            i2s_config_t i2s_config = {/* Fill in the I2S configuration parameters */};
            i2s_pin_config_t pin_config = {
                .bck_io_num = 26,
                .ws_io_num = 25,
                .data_out_num = next_device,
                .data_in_num = I2S_PIN_NO_CHANGE
            };
            i2s_establish_connection(i2s_port0, &i2s_config, &pin_config); // Example using i2s_port0
        }
    }
}

void I2SManager::switch_to_next_device() {
    // Implement logic to switch to the next device in the queue
    if (!device_queue.empty()) {
        int next_device = device_queue.front();
        device_queue.pop();
        connected_devices[active_device_index] = next_device;
        switch_sd_pin(next_device);
    }
}
