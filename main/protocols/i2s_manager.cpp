#include "driver/i2s.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "Golioth.h" // Hypothetical header for Golioth client

/*#define I2S_WS_PIN 13
  #define I2S_BCLK_PIN 14

#define I2S_BUS_0 I2S_NUM_0
#define I2S_BUS_1 I2S_NUM_1*/

static const char *TAG = "I2S_Manager";

// Function prototypes
void i2s_init(i2s_port_t i2s_num, int sd_pin, bool is_tx);
void i2s_activate_device(i2s_port_t i2s_num, bool activate);
void i2s_scan_bus();
void i2s_read_data(i2s_port_t i2s_num, char* buffer, size_t buffer_size, size_t* bytes_read);
void i2s_write_data(i2s_port_t i2s_num, char* buffer, size_t buffer_size, size_t* bytes_written);
void handle_golioth_client(Golioth& client);

// I2S initialization
void i2s_init(i2s_port_t i2s_num, int sd_pin, bool is_tx) {
    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | (is_tx ? I2S_MODE_TX : I2S_MODE_RX),
        .sample_rate = 44100,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 64,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCLK_PIN,
        .ws_io_num = I2S_WS_PIN,
        .data_out_num = (is_tx ? sd_pin : I2S_PIN_NO_CHANGE),
        .data_in_num = (!is_tx ? sd_pin : I2S_PIN_NO_CHANGE)
    };

    ESP_ERROR_CHECK(i2s_driver_install(i2s_num, &i2s_config, 0, NULL));
    ESP_ERROR_CHECK(i2s_set_pin(i2s_num, &pin_config));
    ESP_LOGI(TAG, "I2S bus %d initialized with SD pin %d", i2s_num, sd_pin);
}

// Activate or deactivate I2S device
void i2s_activate_device(i2s_port_t i2s_num, bool activate) {
    if (activate) {
        ESP_ERROR_CHECK(i2s_start(i2s_num));
    } else {
        ESP_ERROR_CHECK(i2s_stop(i2s_num));
    }
}

// Scan I2S bus (generic, optional)
void i2s_scan_bus() {
    ESP_LOGI(TAG, "Scanning I2S bus...");
    // Implement any checks needed to verify device functionality
}

// Read data from I2S device
void i2s_read_data(i2s_port_t i2s_num, char* buffer, size_t buffer_size, size_t* bytes_read) {
    esp_err_t ret = i2s_read(i2s_num, buffer, buffer_size, bytes_read, portMAX_DELAY);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2S read error: %s", esp_err_to_name(ret));
    }
}

// Write data to I2S device
void i2s_write_data(i2s_port_t i2s_num, char* buffer, size_t buffer_size, size_t* bytes_written) {
    esp_err_t ret = i2s_write(i2s_num, buffer, buffer_size, bytes_written, portMAX_DELAY);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2S write error: %s", esp_err_to_name(ret));
    }
}

// Handle Golioth client
void handle_golioth_client(Golioth& client) {
    client.handleInstructions();
}

// Main application loop
void app_main() {
    // Example initialization of two I2S buses with different SD pins
    i2s_init(I2S_BUS_0, 12, true); // Example TX bus
    i2s_init(I2S_BUS_1, 11, false); // Example RX bus

    // Scan I2S bus (optional)
    i2s_scan_bus();

    // Initialize Golioth client
    Golioth golioth_client;
    golioth_client.init();

    // Main loop
    size_t bytes_read, bytes_written;
    char i2s_read_buff[1024];
    char i2s_write_buff[1024];

    while (true) {
        // Ensure only one device is active on each bus
        i2s_activate_device(I2S_BUS_1, true);
        i2s_activate_device(I2S_BUS_0, false);

        // Read data from I2S device (example RX)
        i2s_read_data(I2S_BUS_1, i2s_read_buff, sizeof(i2s_read_buff), &bytes_read);

        // Process data as needed (e.g., send to cloud, apply DSP, etc.)
        // Here, we simply copy read buffer to write buffer
        memcpy(i2s_write_buff, i2s_read_buff, bytes_read);

        // Switch active device
        i2s_activate_device(I2S_BUS_1, false);
        i2s_activate_device(I2S_BUS_0, true);

        // Write data to I2S device (example TX)
        i2s_write_data(I2S_BUS_0, i2s_write_buff, bytes_read, &bytes_written);

        // Handle Golioth client instructions
        handle_golioth_client(golioth_client);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
