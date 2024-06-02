#include "spi_manager.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "SPI_MANAGER";

// Define CS pins for SPI devices
#define CS_PIN1  GPIO_NUM_5    // SPI2 CS1
#define CS_PIN2  GPIO_NUM_18   // SPI2 CS2
#define CS_PIN3  GPIO_NUM_19   // SPI2 CS3
#define CS_PIN4  GPIO_NUM_21   // SPI2 CS4
#define CS_PIN5  GPIO_NUM_22   // SPI2 CS5
#define CS_PIN6  GPIO_NUM_23   // SPI2 CS6
#define CS_PIN7  GPIO_NUM_25   // SPI3 CS1

// SPI device handles
spi_device_handle_t spi_device1;
spi_device_handle_t spi_device2;
spi_device_handle_t spi_device3;
spi_device_handle_t spi_device4;
spi_device_handle_t spi_device5;
spi_device_handle_t spi_device6;
spi_device_handle_t spi_device7;

// Device connection status
bool spi_device_connected[7] = {false, false, false, false, false, false, false};

void init_spi_bus(spi_host_device_t host, int dma_channel) {
    spi_bus_config_t buscfg = {
        .mosi_io_num = GPIO_NUM_23,
        .miso_io_num = GPIO_NUM_19,
        .sclk_io_num = GPIO_NUM_18,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096
    };
    ESP_ERROR_CHECK(spi_bus_initialize(host, &buscfg, dma_channel));
}

void init_spi_device(spi_host_device_t host, int cs_pin, spi_device_handle_t* spi_handle) {
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1*1000*1000,  // Clock out at 1 MHz
        .mode = 0,                      // SPI mode 0
        .spics_io_num = cs_pin,         // CS pin
        .queue_size = 7,                // Queue size
        .pre_cb = NULL,
        .post_cb = NULL
    };
    ESP_ERROR_CHECK(spi_bus_add_device(host, &devcfg, spi_handle));
}

void setup_spi() {
    // Initialize SPI2 and SPI3 buses
    init_spi_bus(SPI2_HOST, 1);
    init_spi_bus(SPI3_HOST, 2);

    // Initialize devices on SPI2
    init_spi_device(SPI2_HOST, CS_PIN1, &spi_device1);
    init_spi_device(SPI2_HOST, CS_PIN2, &spi_device2);
    init_spi_device(SPI2_HOST, CS_PIN3, &spi_device3);
    init_spi_device(SPI2_HOST, CS_PIN4, &spi_device4);
    init_spi_device(SPI2_HOST, CS_PIN5, &spi_device5);
    init_spi_device(SPI2_HOST, CS_PIN6, &spi_device6);

    // Initialize device on SPI3
    init_spi_device(SPI3_HOST, CS_PIN7, &spi_device7);
    // two more devices can be added to the SPI3 bus

    ESP_LOGI(TAG, "SPI devices initialized");
}

void spi_read_device(spi_device_handle_t spi_handle, uint8_t* data, size_t len) {
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = len * 8;  // Length in bits
    t.rx_buffer = data;
    ESP_ERROR_CHECK(spi_device_transmit(spi_handle, &t));
}

void spi_write_device(spi_device_handle_t spi_handle, uint8_t* data, size_t len) {
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = len * 8;  // Length in bits
    t.tx_buffer = data;
    ESP_ERROR_CHECK(spi_device_transmit(spi_handle, &t));
}

void spi_handle_command(const char* instruction, uint8_t address) {
    // Implement command handling logic for SPI devices
    // Use instruction and address as needed
}

void IRAM_ATTR gpio_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    ESP_LOGI(TAG, "GPIO %d changed state", gpio_num);

    // Update device connection status based on GPIO pin state
    switch (gpio_num) {
        case CS_PIN1:
            spi_device_connected[0] = !spi_device_connected[0];
            break;
        case CS_PIN2:
            spi_device_connected[1] = !spi_device_connected[1];
            break;
        case CS_PIN3:
            spi_device_connected[2] = !spi_device_connected[2];
            break;
        case CS_PIN4:
            spi_device_connected[3] = !spi_device_connected[3];
            break;
        case CS_PIN5:
            spi_device_connected[4] = !spi_device_connected[4];
            break;
        case CS_PIN6:
            spi_device_connected[5] = !spi_device_connected[5];
            break;
        case CS_PIN7:
            spi_device_connected[6] = !spi_device_connected[6];
            break;
    }
}

void gpio_init() {
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << CS_PIN1) | (1ULL << CS_PIN2) | (1ULL << CS_PIN3) |
                           (1ULL << CS_PIN4) | (1ULL << CS_PIN5) | (1ULL << CS_PIN6) |
                           (1ULL << CS_PIN7);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);

    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(CS_PIN1, gpio_isr_handler, (void*) CS_PIN1);
    gpio_isr_handler_add(CS_PIN2, gpio_isr_handler, (void*) CS_PIN2);
    gpio_isr_handler_add(CS_PIN3, gpio_isr_handler, (void*) CS_PIN3);
    gpio_isr_handler_add(CS_PIN4, gpio_isr_handler, (void*) CS_PIN4);
    gpio_isr_handler_add(CS_PIN5, gpio_isr_handler, (void*) CS_PIN5);
    gpio_isr_handler_add(CS_PIN6, gpio_isr_handler, (void*) CS_PIN6);
    gpio_isr_handler_add(CS_PIN7, gpio_isr_handler, (void*) CS_PIN7);
}

void spi_periodic_check_task(void* pvParameters) {
    uint8_t handshake_cmd = 0xAA;  // Example handshake command
    uint8_t response[1];

    while (true) {
        // Periodically check each SPI device for a valid response
        if (spi_device_connected[0]) {
            spi_write_device(spi_device1, &handshake_cmd, 1);
            spi_read_device(spi_device1, response, 1);
            if (response[0] != 0x55) {  // Example expected response
                ESP_LOGW(TAG, "Device 1 not responding");
                spi_device_connected[0] = false;
            }
        }

        if (spi_device_connected[1]) {
            spi_write_device(spi_device2, &handshake_cmd, 1);
            spi_read_device(spi_device2, response, 1);
            if (response[0] != 0x55) {  // Example expected response
                ESP_LOGW(TAG, "Device 2 not responding");
                spi_device_connected[1] = false;
            }
        }

        // Repeat for other devices...

        vTaskDelay(pdMS_TO_TICKS(1000));  // Adjust the delay as needed
    }
}