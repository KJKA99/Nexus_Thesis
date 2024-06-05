// i2s_manager.h

#ifndef I2S_MANAGER_H
#define I2S_MANAGER_H

#include "driver/i2s.h"
#include "driver/gpio.h"
#include <queue>

#define MAX_I2S_DEVICES 2 // Maximum number of I2S devices that can be connected at once

class I2SManager {
public:
    I2SManager();
    ~I2SManager();

    bool i2s_establish_connection(i2s_port_t i2s_num, i2s_config_t *i2s_config, i2s_pin_config_t *pin_config);
    bool i2s_master_receive(i2s_port_t i2s_num, int sd_pin, char *data, size_t *data_len);
    void handle_interrupt();
    void verify_active_devices();
    void switch_to_next_device();

private:
    int connected_devices[MAX_I2S_DEVICES];
    std::queue<int> device_queue;
    i2s_port_t i2s_port0;
    i2s_port_t i2s_port1;
    int current_sd_pin;
    int interrupt_pin;
    int active_device_index;

    static void IRAM_ATTR gpio_isr_handler(void* arg);
    void register_isr_handler();
    void switch_sd_pin(int sd_pin);
    void manage_device_queue();
};

#endif // I2S_MANAGER_H
