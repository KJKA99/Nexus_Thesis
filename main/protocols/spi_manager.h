#ifndef SPI_MANAGER_H
#define SPI_MANAGER_H

#include "driver/spi_master.h"
#include <array>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define MAX_DEVICES 7
#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK 18
#define SPI_DMA_CHAN 1

#define CS_PIN1 5
#define CS_PIN2 17
#define CS_PIN3 16
#define CS_PIN4 4
#define CS_PIN5 0
#define CS_PIN6 2
#define CS_PIN7 15

class SPIManager {
public:
    SPIManager();
    ~SPIManager();
    bool spi_establish_connection(spi_host_device_t host, int cs_pin, spi_device_handle_t* handle);
    bool spi_master_transmit(spi_device_handle_t handle, const uint8_t* data, size_t length);
    bool spi_master_receive(spi_device_handle_t handle, uint8_t* data, size_t length);
    void spi_add_device(spi_device_handle_t handle);
    bool spi_verify_connection(spi_device_handle_t handle);
    void scan_and_update_devices();
    void spi_gpio_config();

private:
    static void IRAM_ATTR gpio_isr_handler(void* arg);
    static void handle_interrupt_task(void* arg);
    spi_bus_config_t buscfg;
    std::array<spi_device_handle_t, MAX_DEVICES> devices;
    int device_count;
    static SPIManager* instance;
    static xQueueHandle isr_queue;
};

#endif // SPI_MANAGER_H
