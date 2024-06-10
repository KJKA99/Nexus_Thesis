#include "gpio_expander.h" // Include the GPIO expander header
#include "spi_manager.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <cstring>

#define PIN_NUM_MISO 16
#define PIN_NUM_MOSI 17
#define PIN_NUM_CLK 18
#define SPI_DMA_CHAN 1 // Use DMA channel 1

// Define the common SPI register
spi_device_t common_spi_register[MAX_DEVICES];

SPIManager* SPIManager::instance = nullptr;
xQueueHandle SPIManager::isr_queue = nullptr;

/*
Constructor for the SPIManager class. It initializes the SPI bus and sets up the GPIO configurations. Specifically, it:

Sets the static instance of the SPIManager to this.
Initializes the common SPI register by calling initialize_common_spi_register.
Configures the SPI bus with spi_bus_initialize using the provided configuration.
Calls spi_gpio_config to set up the GPIO pins used for Chip Select (CS) lines.
Creates a FreeRTOS queue for handling interrupt service routine (ISR) events.
Creates a FreeRTOS task to handle ISR events by calling xTaskCreate.
*/
SPIManager::SPIManager() : device_count(0) {
    instance = this;

    initialize_common_spi_register();

    spi_bus_config_t buscfg = {};
    buscfg.miso_io_num = PIN_NUM_MISO;
    buscfg.mosi_io_num = PIN_NUM_MOSI;
    buscfg.sclk_io_num = PIN_NUM_CLK;
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;
    buscfg.max_transfer_sz = 4096;

    spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CHAN);

    spi_gpio_config();

    isr_queue = xQueueCreate(10, sizeof(int));

    xTaskCreate(handle_interrupt_task, "handle_interrupt_task", 2048, NULL, 10, NULL);
}

/*
Destructor for the SPIManager class. It cleans up resources when the SPIManager object is destroyed. Specifically, it:

Frees the SPI bus by calling spi_bus_free.
Deletes the FreeRTOS queue created for ISR events using vQueueDelete.
*/

SPIManager::~SPIManager() {
    spi_bus_free(SPI2_HOST);
    vQueueDelete(isr_queue);
}

/*
Initializes the common SPI register by resetting all entries. 
It iterates through the common_spi_register array and sets each entry's cs_pin to -1 (indicating no device), and nullifies the module_identifier and unit fields.
*/

void SPIManager::initialize_common_spi_register() {
    for (int i = 0; i < MAX_DEVICES; i++) {
        common_spi_register[i].cs_pin = -1;
        common_spi_register[i].module_identifier = NULL;
        common_spi_register[i].unit = -1;
    }
}

/*
Establishes a connection to an SPI device. It:

Checks if the device is already in the array of connected devices and returns the handle if it is.
If the devices array is full, it calls scan_and_update_devices to refresh the list.
Configures the SPI device interface and attaches the device to the SPI bus using spi_bus_add_device.
Updates the array of connected devices and increments the device count.
*/

bool SPIManager::spi_establish_connection(spi_host_device_t host, int cs_pin, spi_device_handle_t* handle) {
    for (int i = 0; i < device_count; ++i) {
        spi_device_interface_config_t devcfg = {};
        spi_get_device_info(devices[i], &devcfg);
        if (devcfg.spics_io_num == cs_pin) {
            *handle = devices[i];
            return true;
        }
    }

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

    if (spi_bus_add_device(host, &devcfg, handle) != ESP_OK) {
        return false;
    }

    devices[device_count++] = *handle;
    return true;
}

/*
Transmits data to an SPI device. It:

Sets up a spi_transaction_t structure with the data to be sent.
Uses spi_device_transmit to send the data over the SPI bus.
Returns true if the transmission is successful, false otherwise.
*/

bool SPIManager::spi_master_transmit(spi_device_handle_t handle, const uint8_t* data, size_t length) {
    spi_transaction_t t = {};
    t.length = length * 8;
    t.tx_buffer = data;
    t.rx_buffer = nullptr;

    return spi_device_transmit(handle, &t) == ESP_OK;
}

/*
Receives data from an SPI device. It:

Sets up a spi_transaction_t structure to receive data.
Uses spi_device_transmit to receive data from the SPI bus.
Returns true if the reception is successful, false otherwise.
*/

bool SPIManager::spi_master_receive(spi_device_handle_t handle, uint8_t* data, size_t length) {
    spi_transaction_t t = {};
    t.length = length * 8;
    t.tx_buffer = nullptr;
    t.rx_buffer = data;

    return spi_device_transmit(handle, &t) == ESP_OK;
}

/*
Scans and updates the list of valid SPI devices. It:

Iterates through the list of connected devices.
Checks if each device is still valid by attempting a transmission.
Updates the device count with the number of valid devices found.
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

/*
Configures the GPIO pins used for CS with interrupt capability. It:

Initializes the GPIO expander by calling initializeMCP23S17.
Configures the CS pins on the GPIO expander to be outputs.
Sets the initial state of the CS pins to HIGH (indicating they are not active).
Enables interrupts for the CS pins.
*/

void SPIManager::spi_gpio_config() {
    initializeMCP23S17(); // Initialize the MCP23S17 expander

    setMCP23S17PinMode(21, OUTPUT); // GPA0, CS_PIN1
    setMCP23S17PinMode(22, OUTPUT); // GPA1, CS_PIN2
    setMCP23S17PinMode(23, OUTPUT); // GPA2, CS_PIN3
    setMCP23S17PinMode(24, OUTPUT); // GPA3, CS_PIN4
    setMCP23S17PinMode(25, OUTPUT); // GPA4, CS_PIN5
    setMCP23S17PinMode(26, OUTPUT); // GPA5, CS_PIN6

    writeMCP23S17Pin(21, HIGH); // CS pins typically HIGH when not in use
    writeMCP23S17Pin(22, HIGH);
    writeMCP23S17Pin(23, HIGH);
    writeMCP23S17Pin(24, HIGH);
    writeMCP23S17Pin(25, HIGH);
    writeMCP23S17Pin(26, HIGH);

    enableInterrupt(21, CHANGE); // Enable interrupts on CS_PIN1
    enableInterrupt(22, CHANGE); // Enable interrupts on CS_PIN2
    enableInterrupt(23, CHANGE); // Enable interrupts on CS_PIN3
    enableInterrupt(24, CHANGE); // Enable interrupts on CS_PIN4
    enableInterrupt(25, CHANGE); // Enable interrupts on CS_PIN5
    enableInterrupt(26, CHANGE); // Enable interrupts on CS_PIN6
}

/*
ISR handler for GPIO interrupts.
Retrieves the pin number from the ISR argument.
Sends the pin number to the ISR queue using xQueueSendFromISR.
*/

void IRAM_ATTR SPIManager::gpio_isr_handler(void* arg) {
    int pin = (int)arg;
    xQueueSendFromISR(isr_queue, &pin, NULL);
}

/*
Handles GPIO interrupts by running as a FreeRTOS task. 

Continuously waits for pin numbers from the ISR queue.
When a pin number is received, it calls scan_and_update_devices to refresh the list of connected devices.
*/

void SPIManager::handle_interrupt_task(void* arg) {
    int pin;
    while (true) {
        if (xQueueReceive(isr_queue, &pin, portMAX_DELAY)) {
            instance->scan_and_update_devices();
        }
    }
}

/*
Reads the device identifier from a specified CS pin. It:

Sets up a spi_transaction_t structure to read from the device.
Selects the device by setting the CS pin low.
Transmits the address to read the identifier.
Deselects the device by setting the CS pin high.
Returns the identifier read from the device, or 0xFF if there was an error.
*/

uint8_t read_device_identifier(int cs_pin) {
    uint8_t identifier = 0;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));

    uint8_t address = 0x00;
    t.length = 8 * sizeof(address);
    t.tx_buffer = &address;
    t.rx_buffer = &identifier;
    t.flags = SPI_TRANS_USE_RXDATA;

    writeMCP23S17Pin(cs_pin, LOW);

    esp_err_t ret = spi_device_transmit(spi_handle, &t);

    writeMCP23S17Pin(cs_pin, HIGH);

    if (ret == ESP_OK) {
        return identifier;
    } else {
        return 0xFF;
    }
}

/*
Identifies the device type based on its identifier. It:

Uses a switch statement to return a string representation of the module type based on the identifier value.
Returns "unknown_module" if the identifier does not match any known values.
*/

const char* identify_device(uint8_t identifier) {
    switch (identifier) {
        case 0x01:
            return "humidity_module";
        case 0x02:
            return "temperature_module";
        default:
            return "unknown_module";
    }
}

/*
Scans and identifies devices connected to the CS pins. It:

Initializes the common SPI register.
Iterates through the CS pins, reading and identifying devices connected to each pin.
Updates the common_spi_register with the identified devices.
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
            common_spi_register[device_count].unit = device_count;
            device_count++;
        }
    }
}

/*
Retrieves device information based on the CS pin. It:

Iterates through the common_spi_register to find the entry with the matching CS pin.
Returns a pointer to the device information if found, or NULL if not found.
*/

const spi_device_t* get_device_info(int cs_pin) {
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (common_spi_register[i].cs_pin == cs_pin) {
            return &common_spi_register[i];
        }
    }
    return NULL;
}

/*
Handles SPI communication based on the device type. It:

Retrieves the device information for the specified CS pin.
Based on the module_identifier, it calls the appropriate handler function for the device (e.g., handle_humidity_module or handle_temperature_module).
Prints a message if the module type is unknown.
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

/*
Enables interrupts for a specific pin on the MCP23S17 GPIO expander.
*/

void SPIManager::enableInterrupt(uint8_t pin, uint8_t mode) {
    mcp.enableInterrupt(pin, mode);
}

/*
Disables interrupts for a specific pin on the MCP23S17 GPIO expander.
*/

void SPIManager::disableInterrupt(uint8_t pin) {
    mcp.disableInterrupt(pin);
}

/*
Handles the interrupt by reading the interrupt flag register and the interrupt capture register.
*/

void SPIManager::handleInterrupt() {
    uint16_t interrupt_flags = mcp.getInterruptFlagRegister();
    uint16_t interrupt_capture = mcp.getInterruptCaptureRegister();
    // Handle the interrupts accordingly
}
