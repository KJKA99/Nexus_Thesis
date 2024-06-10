#ifndef SPI_MANAGER_H
#define SPI_MANAGER_H

#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#define NUM_CS_PINS 6
#define MAX_DEVICES 10

typedef struct {
    int cs_pin;
    const char* module_identifier;
    int unit; // Identifier for specific devices of the same type
} spi_device_t;

class SPIManager {
public:
    static SPIManager* instance;

    SPIManager();
    ~SPIManager();

    bool spi_establish_connection(spi_host_device_t host, int cs_pin, spi_device_handle_t* handle);
    bool spi_master_transmit(spi_device_handle_t handle, const uint8_t* data, size_t length);
    bool spi_master_receive(spi_device_handle_t handle, uint8_t* data, size_t length);

    void enableInterrupt(uint8_t pin, uint8_t mode); // Ensure the parameters match the definition
    void disableInterrupt(uint8_t pin);
    void handleInterrupt();

    void initialize_common_spi_register();

private:
    static xQueueHandle isr_queue;
    spi_device_handle_t devices[MAX_DEVICES];
    int device_count;

    
    void scan_and_update_devices();
    void spi_gpio_config();
    static void IRAM_ATTR gpio_isr_handler(void* arg);
    static void handle_interrupt_task(void* arg);
};

// Add your external functions declarations here
uint8_t read_device_identifier(int cs_pin);
const char* identify_device(uint8_t identifier);
void spi_identify_devices();
const spi_device_t* get_device_info(int cs_pin);
void handle_spi_communication(int cs_pin);

// Declare the external common SPI register
extern spi_device_t common_spi_register[MAX_DEVICES];

#endif // SPI_MANAGER_H
