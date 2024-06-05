#include "spi_manager.h"
#include "driver/gpio.h"
#include "esp_log.h"

SPIManager* SPIManager::instance = nullptr;
xQueueHandle SPIManager::isr_queue = nullptr;

SPIManager::SPIManager() : device_count(0) {
    instance = this;

    // Initialize the SPI bus configuration
    buscfg.miso_io_num = PIN_NUM_MISO;
    buscfg.mosi_io_num = PIN_NUM_MOSI;
    buscfg.sclk_io_num = PIN_NUM_CLK;
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;
    buscfg.max_transfer_sz = 4096;

    // Initialize the SPI bus
    spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CHAN);

    // Configure GPIO for CS pins with interrupt
    spi_gpio_config();

    // Create a FreeRTOS queue to handle ISR events
    isr_queue = xQueueCreate(10, sizeof(int));

    // Create a FreeRTOS task to handle ISR events
    xTaskCreate(handle_interrupt_task, "handle_interrupt_task", 2048, NULL, 10, NULL);
}

SPIManager::~SPIManager() {
    // Cleanup the SPI bus
    spi_bus_free(SPI2_HOST);
    vQueueDelete(isr_queue);
}

bool SPIManager::spi_establish_connection(spi_host_device_t host, int cs_pin, spi_device_handle_t* handle) {
    // Check if the device is already in the array
    for (int i = 0; i < device_count; ++i) {
        spi_device_interface_config_t devcfg = {};
        spi_get_device_info(devices[i], &devcfg);
        if (devcfg.spics_io_num == cs_pin) {
            *handle = devices[i];
            return true;
        }
    }

    // If the devices array is full, verify and update connections
    if (device_count >= MAX_DEVICES) {
        scan_and_update_devices();
        if (device_count >= MAX_DEVICES) {
            return false;
        }
    }

    spi_device_interface_config_t devcfg = {};
    devcfg.clock_speed_hz = 10 * 1000 * 1000; // Clock out at 10 MHz
    devcfg.mode = 0;                         // SPI mode 0
    devcfg.spics_io_num = cs_pin;            // CS pin
    devcfg.queue_size = 7;                   // We want to be able to queue 7 transactions at a time
    devcfg.pre_cb = nullptr;                 // Specify pre-transfer callback, if any

    // Attach the SPI device
    if (spi_bus_add_device(host, &devcfg, handle) != ESP_OK) {
        return false;
    }

    devices[device_count++] = *handle;
    return true;
}

bool SPIManager::spi_master_transmit(spi_device_handle_t handle, const uint8_t* data, size_t length) {
    spi_transaction_t t = {};
    t.length = length * 8; // Transaction length in bits
    t.tx_buffer = data;    // Data to send
    t.rx_buffer = nullptr; // We are not interested in receiving data

    return spi_device_transmit(handle, &t) == ESP_OK;
}

bool SPIManager::spi_master_receive(spi_device_handle_t handle, uint8_t* data, size_t length) {
    spi_transaction_t t = {};
    t.length = length * 8; // Transaction length in bits
    t.tx_buffer = nullptr; // We are not sending data
    t.rx_buffer = data;    // Data to receive

    return spi_device_transmit(handle, &t) == ESP_OK;
}

void SPIManager::spi_add_device(spi_device_handle_t handle) {
    if (device_count < MAX_DEVICES) {
        devices[device_count++] = handle;
    }
}

bool SPIManager::spi_verify_connection(spi_device_handle_t handle) {
    for (int i = 0; i < device_count; ++i) {
        if (devices[i] == handle) {
            return true;
        }
    }
    return false;
}

void SPIManager::scan_and_update_devices() {
    int valid_device_count = 0;
    for (int i = 0; i < device_count; ++i) {
        if (spi_verify_connection(devices[i])) {
            devices[valid_device_count++] = devices[i];
        }
    }
    device_count = valid_device_count;
}

void SPIManager::spi_gpio_config() {
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

void IRAM_ATTR SPIManager::gpio_isr_handler(void* arg) {
    int pin = (int) arg;
    xQueueSendFromISR(isr_queue, &pin, NULL);
}

void SPIManager::handle_interrupt_task(void* arg) {
    int pin;
    while (true) {
        if (xQueueReceive(isr_queue, &pin, portMAX_DELAY)) {
            // Handle interrupt outside ISR context
            instance->scan_and_update_devices();
        }
    }
}
