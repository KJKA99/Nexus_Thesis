#include "golioth_manager.h"
#include "system_client.h"
#include "lightdb.h"
#include "golioth_log.h"

static golioth_client_t *_client;
static golioth_sys_sem_t _connected_sem;

// Current firmware version
static const char *_current_version = "1.2.5";

// Loop delay configuration
static int32_t _loop_delay_s = 10;

// LightDB function to handle incoming messages
static void on_message(
        golioth_client_t *client,
        const char *path,
        const uint8_t *data,
        size_t data_len,
        void *arg) {
    char buf[128] = {0};
    memcpy(buf, data, data_len);
    GLTH_LOGI(TAG, "Received message on path %s: %s", path, buf);
}

// System Client function to initialize Golioth client
void golioth_initialize() {
    golioth_client_config_t config = {
        .credentials = {
            .psk_id = CONFIG_GOLIOTH_PSK_ID,
            .psk = CONFIG_GOLIOTH_PSK,
        },
    };

    // Create the Golioth client
    _client = golioth_client_create(&config);
    
    if (!_client) {
        GLTH_LOGE(TAG, "Failed to create Golioth client");
        return;
    }

    // Initialize the Settings and RPC services
    struct golioth_settings *settings = golioth_settings_init(_client);
    struct golioth_rpc *rpc = golioth_rpc_init(_client);

    // Initialize firmware update service
    golioth_fw_update_init(_client, _current_version);

    // Register client event callback
    golioth_client_register_event_callback(_client, golioth_client_connection, NULL);

    // Register settings callbacks
    golioth_settings_register_int(settings, "LOOP_DELAY_S", on_loop_delay_setting, NULL);

    // Register RPC methods
    golioth_rpc_register(rpc, "multiply", on_multiply, NULL);

    // Start the Golioth system client
    golioth_system_client_start(_client);

    // Observe LightDB
    golioth_lightdb_observe_async(_client, "desired/my_config", GOLIOTH_CONTENT_TYPE_JSON, on_my_config, NULL);

    // Log message indicating initialization is complete
    GLTH_LOGI(TAG, "Golioth initialization complete");
}

// Establish connection with Golioth client
static void golioth_client_connection(struct golioth_client *client,
                            enum golioth_client_event event,
                            void *arg) {
    bool is_connected = (event == GOLIOTH_CLIENT_EVENT_CONNECTED);
    if (is_connected) {
        golioth_sys_sem_give(_connected_sem);
    }
    GLTH_LOGI(TAG, "Golioth client %s", is_connected ? "connected" : "disconnected");
}

// Change settings on the device remotely
static enum golioth_settings_status on_loop_delay_setting(int32_t new_value, void *arg) {
    GLTH_LOGI(TAG, "Setting loop delay to %" PRId32 " s", new_value);
    _loop_delay_s = new_value;
    return GOLIOTH_SETTINGS_SUCCESS;
}

// Logging function for different log types
void golioth_log(const char *log_type, const char *log_message) {
    if (strcmp(log_type, "debug") == 0) {
        golioth_log_debug_async(_client, "TAG", log_message, NULL, NULL);
    } else if (strcmp(log_type, "info") == 0) {
        golioth_log_info_async(_client, "TAG", log_message, NULL, NULL);
    } else if (strcmp(log_type, "warning") == 0) {
        golioth_log_warning_async(_client, "TAG", log_message, NULL, NULL);
    } else if (strcmp(log_type, "error") == 0) {
        golioth_log_error_async(_client, "TAG", log_message, NULL, NULL);
    }
}

// LightDB function to send data periodically
void send_data_periodically(const char *path, const char *data) {
    golioth_lightdb_set_async(_client, path, GOLIOTH_CONTENT_TYPE_JSON, (const uint8_t *)data, strlen(data), NULL, NULL);
}

// LightDB function to send data continuously
void send_data_continuously(const char *path, const char *data) {
    golioth_stream_set_async(_client, path, GOLIOTH_CONTENT_TYPE_JSON, (const uint8_t *)data, strlen(data), NULL, NULL);
}

// RPC method to handle "multiply" command
static golioth_rpc_status_t on_multiply(
    zcbor_state_t *request_params_array,
    zcbor_state_t *response_detail_map,
    void *callback_arg) 

/* Send commands from client through RPC
static golioth_rpc_status_t client_commands_to_device (
    zcbor_state_t *request_params_array,
    zcbor_state_t *response_detail_map,
    void *callback_arg)
{
    // Parse the request parameters
    char i2c_target_address[128];
    char spi_target_cs[6]
    char command[32];
    char protocol[3];
    char intervall[60]
    zcbor_string_decode(request_params_array, target_address, sizeof(target_address));
    zcbor_string_decode(request_params_array, command, sizeof(command));
    zcbor_string_decode(request_params_array, protocol, sizeof(protocol));

    // Perform the requested action based on the command
    if (strcmp(command, "send_data_continuously") == 0) {
        // Establish contact with the target address using the appropriate protocol (I2C, I2S, or SPI)
        // If the connection is successful, send the command to the device to send data continuously
        // Receive the data from the device
        // Encode the received data into the response detail map

        switch (protocol) {
            case "i2c":
                if (i2c_manager.establish_connection(i2c_target_address) == true) { // Established connection with the target address using I2C
                    i2c_manager.i2c_master_receive(target_address, data, data_len); // Receive data from the target address using I2C
                    golioth_stream_set_async(_client, "data", GOLIOTH_CONTENT_TYPE_JSON, data, data_len, NULL, NULL); // Send the data to the Golioth server
                    response_detail_map = zcbor_map_create(); // Encode the received data into the response detail map
                    golioth_log("info", "Data sent successfully");
                }
                break;
            case "spi":
                if (spi_manager.establish_connection(spi_target_cs) == true) {
                    spi_manager.spi_read_device(spi_target_cs, data, data_len);
                    golioth_stream_set_async(_client, "data", GOLIOTH_CONTENT_TYPE_JSON, data, data_len, NULL, NULL);
                    response_detail_map = zcbor_map_create();
                    golioth_log("info", "Data sent successfully");

                }
                break;
            case "i2s":
                if (i2s_manager.establish_connection(target_address) == true) {
                    i2s_manager.i2s_read_device(target_address, data, data_len);
                }
                break;
            default:
                log_to_golioth("Invalid protocol specified");
                break;
        }
        
        zcbor_tstr_encode_ptr(response_detail_map, "data", data, data_len);
    } else if (strcmp(command, "send_data_periodically") == 0) {
        // Similar steps as above, but for sending data periodically
    }

    return GOLIOTH_RPC_OK;
}*/

// Send commands from client through RPC
static golioth_rpc_status_t client_commands_to_device(
    zcbor_state_t *request_params_array,
    zcbor_state_t *response_detail_map,
    void *callback_arg)
{
    // Parse the request parameters
    char i2c_address[128] = {0};
    char spi_cs[10] = {0};
    char i2s_port[10] = {0};
    char command[32] = {0};
    char protocol[3] = {0};
    char interval_str[10] = {0};
    char duration_str[10] = {0};
    char storage[10] = {0};

    int interval = 0; // 0 means send continuously
    int duration = 0; // 0 means send indefinitely
    bool store_in_lightDB = false;

    // Decode parameters from request
    if (!zcbor_string_decode(request_params_array, target_address, sizeof(target_address)) ||
        !zcbor_string_decode(request_params_array, command, sizeof(command)) ||
        !zcbor_string_decode(request_params_array, protocol, sizeof(protocol))) {
        return GOLIOTH_RPC_FAILED;
    }

    // Decode optional parameters if they are present
    zcbor_string_decode(request_params_array, interval_str, sizeof(interval_str));
    zcbor_string_decode(request_params_array, duration_str, sizeof(duration_str));
    zcbor_string_decode(request_params_array, storage, sizeof(storage)); // lightDB or other storage

    if (interval_str[0] != '\0') { // Check if interval is provided
        interval = atoi(interval_str);
    }
    if (duration_str[0] != '\0') { // Check if duration is provided
        duration = atoi(duration_str);
    }
    if (strcmp(storage, "lightDB") == 0) { // Check if data should be stored in lightDB
        store_in_lightDB = true;
    }

    // Perform the requested action based on the command
    if (strcmp(command, "send_data") == 0) {
        // Data buffer
        uint8_t data[256];
        size_t data_len = sizeof(data);
        bool success = false;

        // Establish connection and receive data based on the protocol
        if (strcmp(protocol, "i2c") == 0) {
            success = i2c_manager.establish_connection(i2c_address) && // Establish connection with the target address using I2C
                      i2c_manager.i2c_master_receive(i2c_address, data, &data_len); // Receive data from the target address using I2C
        } else if (strcmp(protocol, "spi") == 0) {
            success = spi_manager.establish_connection(spi_cs) &&
                      spi_manager.spi_read_device(spi_cs, data, &data_len);
        } else if (strcmp(protocol, "i2s") == 0) {
            success = i2s_manager.establish_connection(i2s_port) &&
                      i2s_manager.i2s_read_device(i2s_port, data, &data_len);
        } else {
            log_to_golioth("Invalid protocol specified");
            return GOLIOTH_RPC_FAILED;
        }

        if (success) {
            // Send the data to the Golioth server
            golioth_stream_set_async(_client, "data", GOLIOTH_CONTENT_TYPE_JSON, data, data_len, NULL, NULL);
            zcbor_tstr_encode_ptr(response_detail_map, "data", data, data_len);
            golioth_log("info", "Data sent successfully");

            if (store_in_lightDB) {
                send_data_periodically("path/to/store/data", (const char*)data); // Store data in LightDB // Todo: add actual lightDB path
            }

             // Periodic data sending logic (if interval > 0)
            if (interval > 0) {
                stop_sending = false;
                send_timer = xTimerCreate("SendTimer", pdMS_TO_TICKS(interval * 1000), pdTRUE, NULL, send_data_callback);
                xTimerStart(send_timer, 0);
            }

            // Duration logic (if duration > 0)
            if (duration > 0) {
                duration_timer = xTimerCreate("DurationTimer", pdMS_TO_TICKS(duration * 1000), pdFALSE, NULL, duration_callback);
                xTimerStart(duration_timer, 0);
            }

            return GOLIOTH_RPC_OK;
        } else {
            log_to_golioth("Failed to establish connection or receive data");
            return GOLIOTH_RPC_FAILED;
        }
    }

    return GOLIOTH_RPC_FAILED;
}
   
