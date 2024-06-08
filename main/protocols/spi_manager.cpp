#include "spi_manager.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <cstring>

// Define the common SPI register
spi_device_t common_spi_register[MAX_DEVICES];

SPIManager* SPIManager::instance = nullptr;
xQueueHandle SPIManager::isr_queue = nullptr;

/**
 * @brief Constructor for SPIManager class. Initializes the SPI bus and GPIO configurations.
 */
SPIManager::SPIManager() : device_count(0) {
    instance = this;

    // Initialize the common SPI register
    initialize_common_spi_register();

    // Initialize the SPI bus configuration
    spi_bus_config_t buscfg = {};
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

/**
 * @brief Destructor for SPIManager class. Cleans up the SPI bus and deletes the ISR queue.
 */
SPIManager::~SPIManager() {
    // Cleanup the SPI bus
    spi_bus_free(SPI2_HOST);
    vQueueDelete(isr_queue);
}

/**
 * @brief Initializes the common SPI register by resetting all entries.
 */
void SPIManager::initialize_common_spi_register() {
    for (int i = 0; i < MAX_DEVICES; i++) {
        common_spi_register[i].cs_pin = -1;
        common_spi_register[i].module_identifier = NULL;
        common_spi_register[i].unit = -1;
    }
}

/**
 * @brief Establishes a connection to an SPI device.
 * 
 * @param host SPI host device.
 * @param cs_pin Chip select pin for the SPI device.
 * @param handle Pointer to the SPI device handle.
 * @return true if the connection is successful, false otherwise.
 */
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

    // If the devices array is full, scan and update connections
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

/**
 * @brief Transmits data to an SPI device.
 * 
 * @param handle Handle of the SPI device.
 * @param data Pointer to the data to be transmitted.
 * @param length Length of the data to be transmitted.
 * @return true if the transmission is successful, false otherwise.
 */
bool SPIManager::spi_master_transmit(spi_device_handle_t handle, const uint8_t* data, size_t length) {
    spi_transaction_t t = {};
    t.length = length * 8; // Transaction length in bits
    t.tx_buffer = data;    // Data to send
    t.rx_buffer = nullptr; // We are not interested in receiving data

    return spi_device_transmit(handle, &t) == ESP_OK;
}

/**
 * @brief Receives data from an SPI device.
 * 
 * @param handle Handle of the SPI device.
 * @param data Pointer to the buffer where received data will be stored.
 * @param length Length of the data to be received.
 * @return true if the reception is successful, false otherwise.
 */
bool SPIManager::spi_master_receive(spi_device_handle_t handle, uint8_t* data, size_t length) {
    spi_transaction_t t = {};
    t.length = length * 8; // Transaction length in bits
    t.tx_buffer = nullptr; // We are not sending data
    t.rx_buffer = data;    // Data to receive

    return spi_device_transmit(handle, &t) == ESP_OK;
}

/**
 * @brief Scans and updates the list of valid SPI devices.
 */
void SPIManager::scan_and_update_devices() {
    int valid_device_count = 0;
    for (int i = 0; i < device_count; ++i) {
        spi_device_interface_config_t devcfg = {};
        spi_get_device_info(devices[i], &devcfg);
        if (spi_device_transmit(devices[i], nullptr) == ESP_OK) {
            devices[valid_device_count++] = devices[i];
        }
    }
    device_count = valid_device_count;
}

/**
 * @brief Configures the GPIO pins for CS with interrupt capability.
 */
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

/**
 * @brief ISR handler for GPIO interrupts.
 * 
 * @param arg Pointer to the CS pin number.
 */
void IRAM_ATTR SPIManager::gpio_isr_handler(void* arg) {
    int pin = (int) arg;
    xQueueSendFromISR(isr_queue, &pin, NULL);
}

/**
 * @brief Task to handle GPIO interrupts.
 * 
 * @param arg Unused parameter.
 */
void SPIManager::handle_interrupt_task(void* arg) {
    int pin;
    while (true) {
        if (xQueueReceive(isr_queue, &pin, portMAX_DELAY)) {
            // Handle interrupt outside ISR context
            instance->scan_and_update_devices();
        }
    }
}

/**
 * @brief Reads the device identifier from the specified CS pin.
 * 
 * @param cs_pin Chip select pin of the SPI device.
 * @return Identifier of the device, or 0xFF if an error occurred.
 */
uint8_t read_device_identifier(int cs_pin) {
    uint8_t identifier = 0;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));

    uint8_t address = 0x00; // Example register address for identifier
    t.length = 8 * sizeof(address); // Length in bits
    t.tx_buffer = &address; // Transmit the address
    t.rx_buffer = &identifier; // Buffer to receive identifier
    t.flags = SPI_TRANS_USE_RXDATA; // Use RX buffer

    // Select the device by setting CS pin low
    gpio_set_level(cs_pin, 0);

    // Transmit the transaction
    esp_err_t ret = spi_device_transmit(spi_handle, &t);

    // Deselect the device by setting CS pin high
    gpio_set_level(cs_pin, 1);

    if (ret == ESP_OK) {
        return identifier;
    } else {
        // Handle error
        return 0xFF; // Return an invalid identifier on error
    }
}

/**
 * @brief Identifies the device type based on the identifier.
 * 
 * @param identifier Identifier of the device.
 * @return String representation of the module type.
 */
const char* identify_device(uint8_t identifier) {
    switch (identifier) {
        case 0x01:
            return "humidity_module";
        case 0x02:
            return "temperature_module";
        // Add more cases for other modules
        default:
            return "unknown_module";
    }
}

/**
 * @brief Scans and identifies devices connected to the CS pins.
 */
void spi_identify_devices() {
    SPIManager::instance->initialize_common_spi_register();

    int device_count = 0;
    for (int cs_pin = 0; cs_pin < NUM_CS_PINS; cs_pin++) {
        uint8_t identifier = read_device_identifier(cs_pin);
        const char* module_identifier = identify_device(identifier);

        if (strcmp(module_identifier, "unknown_module") != 0) {
            common_spi_register[device_count].cs_pin = cs_pin;
            common_spi_register[device_count].module_identifier = module_identifier;
            common_spi_register[device_count].unit = device_count; // Example unit assignment
            device_count++;
        }
    }
}

/**
 * @brief Retrieves device information based on the CS pin.
 * 
 * @param cs_pin Chip select pin of the SPI device.
 * @return Pointer to the device information, or NULL if not found.
 */
const spi_device_t* get_device_info(int cs_pin) {
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (common_spi_register[i].cs_pin == cs_pin) {
            return &common_spi_register[i];
        }
    }
    return NULL;
}

/**
 * @brief Handles SPI communication based on the device type.
 * 
 * @param cs_pin Chip select pin of the SPI device.
 */
void handle_spi_communication(int cs_pin) {
    const spi_device_t* device_info = get_device_info(cs_pin);
    if (device_info == NULL) {
        printf("No device connected to CS pin: %d\n", cs_pin);
        return;
    }

    if (strcmp(device_info->module_identifier, "humidity_module") == 0) {
        handle_humidity_module(cs_pin);
    } else if (strcmp(device_info->module_identifier, "temperature_module") == 0) {
        handle_temperature_module(cs_pin);
    } else {
        printf("Unknown module connected to CS pin: %d\n", cs_pin);
    }
}
