/*
    ESP32-C6 I2C - LIS2MDL AMR Sensor

    - ESP32-C6 is a microcontroller from Espressif Systems
      with Wi-Fi and Bluetooth capabilities: https://www.espressif.com/en/products/socs/esp32-c6

    - LIS2MDL information: https://www.st.com/en/mems-and-sensors/lis2mdl.html
*/

#include <stdio.h>
#include "esp_log.h"
#include <freertos/FreeRTOS.h>
#include "cJSON.h"
#include "esp_mac.h"
#include "esp_event.h"
#include "esp_mesh.h"

/** For using esp32-C6-devkit LED for status
 *  Must add component through component registry
 */
#include "driver/gpio.h"
#include "led_strip.h"

#define LED_GPIO 8                              // gpio pin for led strip of esp32-c6-devkit
#define LED_STRIP_RMT_RES_HZ (10 * 1000 * 1000) // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)

led_strip_handle_t led_strip; // led strip handle

/* ******************************************************************************** */
/*                              User defined                                        */
/* ******************************************************************************** */

#include "esp_lis2mdl.h"
#include "esp_json.h"
#include "mqtt_amr.h"
#include "mesh_amr.h"
#include "include/event_amr.h"

/* ******************************************************************************** */
/*                              MQTT                                                */
/* ******************************************************************************** */

esp_mqtt_client_handle_t client = NULL;

/* ******************************************************************************** */
/*                              GLOBAL                                              */
/* ******************************************************************************** */

#define SAMPLE_PERIOD_MS (5000)
#define WAIT_MQTT_CONNECTED_MS (2000)
#define WAIT_DS_CONNECTED_MS (2000)
#define WAIT_MESSAGE_QUEUE_MS (1000)
#define RX_SIZE (300)
#define TX_SIZE (300)
#define LIS2MDL_CHANGE_MODE_MS (40)
#define LIS2MDL_AVG_SAMPLES (30)

static const char *TAG = "AMR_IOT_SENSOR";

// Buffers for sending and receiving data
static uint8_t tx_buf[TX_SIZE] = {
    0,
};
static uint8_t rx_buf[RX_SIZE] = {
    0,
};

// Global variable to hold the MAC address
uint8_t mac[6];

// Global loop handler
esp_event_loop_handle_t event_loop;

// Define the custom event base
ESP_EVENT_DEFINE_BASE(AMR_IOT_EVENT);

bool mesh_ds_connection = false;
bool mqtt_connected = false;
bool parent_connected = false;
bool is_node = true;

bool root_task_running = false;
bool node_task_running = false;

/* ******************************************************************************** */
/*                              TASKS                                               */
/* ******************************************************************************** */

/**
 * @brief When the root of MESH network obtains an IP address,
 *        this function is called to initialize the MQTT client
 */
void root_mqtt_init()
{
    ESP_LOGI(TAG, "Initializing MQTT...");
    ESP_ERROR_CHECK(mqtt_start(event_loop));
    ESP_LOGI(TAG, "MQTT initialized successfully");

    vTaskDelete(NULL);
}

/**
 * @brief When the root of MESH network loses the IP address,
 *        this function is called to deinitialize the MQTT client
 */
void root_mqtt_deinit()
{
    ESP_LOGI(TAG, "Stoping MQTT...");
    ESP_ERROR_CHECK(mqtt_app_stop());
    ESP_LOGI(TAG, "MQTT stopped successfully");

    vTaskDelete(NULL);
}

void root_main()
{
    esp_err_t err;
    mesh_addr_t sending_node;
    mesh_data_t data;
    mesh_rx_pending_t *rx_pending = malloc(sizeof(mesh_rx_pending_t));
    int flag = 0;
    data.data = rx_buf;
    data.size = RX_SIZE;

    lis2mdl_sensor_handle_t my_sensor;

    // NODE - Colour = green
    led_strip_set_pixel(led_strip, 0, 0, 130, 0);
    led_strip_refresh(led_strip);

    while (!is_node)
    {
        // If Root is not connected to MQTT broker, do nothing
        while (!mqtt_connected)
        {
            ESP_LOGI(TAG, "MQTT not connected, waiting...");
            vTaskDelay(pdMS_TO_TICKS(WAIT_MQTT_CONNECTED_MS));
            continue;
        }

        ESP_LOGI(TAG, "Sending ROOT data to MQTT...");

        err = esp_lis2mdl_op_mode_set(LIS2MDL_CONTINUOUS_MODE);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error setting LIS2MDL to continuous mode");
            continue;
        }
        // Wait for the sensor to change mode and provide data
        vTaskDelay(pdMS_TO_TICKS(LIS2MDL_CHANGE_MODE_MS));

        err = esp_lis2mdl_get_avg_calibrated_uTesla(&my_sensor, LIS2MDL_AVG_SAMPLES);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error reading magnetic uTesla data");
            continue;
        }
        err = esp_lis2mdl_get_temperature_celsius(&my_sensor);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error reading temperature raw data");
            continue;
        }

        JsonESP *json = json_esp_create();
        if (json == NULL)
        {
            ESP_LOGE(TAG, "Failed to create JSON object");
            continue;
        }
        json_esp_make_utesla_data_for_send(json, my_sensor.mag_value.uTesla_x,
                                           my_sensor.mag_value.uTesla_y,
                                           my_sensor.mag_value.uTesla_z,
                                           my_sensor.temp_value.celsius,
                                           mac);
        char *jsonString = json_esp_stringify(json);
        if (jsonString == NULL)
        {
            ESP_LOGE(TAG, "Failed to serialize JSON object");
            json_esp_free(json);
            continue;
        }

        // Print the JSON string before sending
        ESP_LOGI(TAG, "JSON data: %s", jsonString);

        mqtt_app_publish("/sensor/data", jsonString);

        ESP_LOGI(TAG, "Checking for data from nodes...");
        esp_mesh_get_rx_pending(rx_pending);

        while (rx_pending->toSelf != 0)
        {
            data.size = RX_SIZE;
            err = esp_mesh_recv(&sending_node, &data, portMAX_DELAY, &flag, NULL, 0);
            if (err != ESP_OK || !data.size)
            {
                ESP_LOGE(TAG, "err:0x%x, size:%d", err, data.size);
                continue;
            }
            ESP_LOGI(TAG, "Node: " MACSTR " sent data", MAC2STR(sending_node.addr));

            // Ensure the received data is null-terminated
            if (data.size < RX_SIZE)
            {
                data.data[data.size] = '\0';
            }
            else
            {
                data.data[RX_SIZE - 1] = '\0';
            }
            mqtt_app_publish("/sensor/data", (char *)data.data);
            esp_mesh_get_rx_pending(rx_pending);
        }

        err = esp_lis2mdl_op_mode_set(LIS2MDL_POWER_DOWN);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error setting LIS2MDL to idle mode");
        }

        // Free JSON string and JSON object
        free(jsonString);
        json_esp_free(json);
        vTaskDelay(pdMS_TO_TICKS(SAMPLE_PERIOD_MS));
    }

    free(rx_pending);

    xTaskCreate(root_mqtt_deinit, "root_mqtt_deinit", 2048, NULL, 5, NULL);
    ESP_LOGI(TAG, "I am no longer the root - deleting root main task");
    root_task_running = false;
    vTaskDelete(NULL);
}

void node_main()
{
    mesh_data_t data;
    data.data = tx_buf;
    data.size = sizeof(tx_buf);
    data.proto = MESH_PROTO_JSON;
    data.tos = MESH_TOS_P2P;
    esp_err_t err;
    lis2mdl_sensor_handle_t my_sensor;

    // Convert MAC address to a hexadecimal string
    char mac_str[18]; // 12 hex digits + 5 colons + null terminator
    snprintf(mac_str, sizeof(mac_str), MACSTR, MAC2STR(mac));

    // NODE - Colour = blue
    led_strip_set_pixel(led_strip, 0, 0, 0, 130);
    led_strip_refresh(led_strip);

    while (is_node)
    {

        // If MESH does not have access to the external network, do nothing
        while (!mesh_ds_connection)
        {
            ESP_LOGI(TAG, "No connection to external network, waiting...");
            vTaskDelay(pdMS_TO_TICKS(WAIT_DS_CONNECTED_MS));
            continue;
        }

        err = esp_lis2mdl_op_mode_set(LIS2MDL_CONTINUOUS_MODE);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error setting LIS2MDL to continuous mode");
            continue;
        }
        // Wait for the sensor to change mode and provide data
        vTaskDelay(pdMS_TO_TICKS(LIS2MDL_CHANGE_MODE_MS));

        err = esp_lis2mdl_get_avg_calibrated_uTesla(&my_sensor, LIS2MDL_AVG_SAMPLES);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error reading magnetic uTesla data");
            continue;
        }

        err = esp_lis2mdl_get_temperature_celsius(&my_sensor);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error reading temperature raw data");
            continue;
        }

        JsonESP *json = json_esp_create();
        if (json == NULL)
        {
            ESP_LOGE(TAG, "Failed to create JSON object");
            continue;
        }

        json_esp_make_utesla_data_for_send(json, my_sensor.mag_value.uTesla_x,
                                           my_sensor.mag_value.uTesla_y,
                                           my_sensor.mag_value.uTesla_z,
                                           my_sensor.temp_value.celsius,
                                           mac);
        char *jsonString = json_esp_stringify(json);

        if (jsonString == NULL)
        {
            ESP_LOGE(TAG, "Failed to serialize JSON object");
            json_esp_free(json);
            continue;
        }

        // Copy JSON string to tx_buf
        strncpy((char *)tx_buf, jsonString, sizeof(tx_buf) - 1);
        tx_buf[sizeof(tx_buf) - 1] = '\0'; // Ensure null-termination

        // Update data size
        data.data = tx_buf;
        data.size = strlen((char *)tx_buf);
        if (parent_connected && is_node)
        {
            ESP_LOGI(TAG, "Sending data to parent...");
            err = esp_mesh_send(NULL, &data, MESH_DATA_P2P, NULL, 0);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "Failed to send data to parent: %s", esp_err_to_name(err));
            }
        }
        else
        {
            ESP_LOGW(TAG, "Parent not connected, cannot send data");
        }

        err = esp_lis2mdl_op_mode_set(LIS2MDL_POWER_DOWN);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error setting LIS2MDL to idle mode");
        }

        // Free JSON string and JSON object
        free(jsonString);
        json_esp_free(json);
        vTaskDelay(pdMS_TO_TICKS(SAMPLE_PERIOD_MS));
    }

    ESP_LOGI(TAG, "I am no longer just a node - deleting node main task");
    node_task_running = false;
    vTaskDelete(NULL);
}

/* ******************************************************************************** */
/*                              EVENT HANDLER                                       */
/* ******************************************************************************** */

static void event_handler(void *handler_args, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    switch (event_id)
    {
    case MESH_AMR_DS_OFF:
        ESP_LOGI(TAG, "Cannot Communicate with external network...");
        mesh_ds_connection = false;
        break;
    case MESH_AMR_DS_ON:
        ESP_LOGI(TAG, "Communication with external network OK!");
        mesh_ds_connection = true;
        break;
    case MESH_AMR_GOT_IP:
        ESP_LOGI(TAG, "Current root has obtained IP address");
        is_node = !(esp_mesh_is_root());
        if (!is_node && !root_task_running)
        {
            root_task_running = true;
            xTaskCreate(root_mqtt_init, "root_mqtt_init", 2048, NULL, 5, NULL);
            xTaskCreate(root_main, "root_main", 4096, NULL, 5, NULL);
        }
        break;
    case MQTT_AMR_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT Connected");
        mqtt_connected = true;
        esp_mesh_post_toDS_state(true);
        break;
    case MQTT_AMR_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT Disconnected");
        mqtt_connected = false;
        if (mesh_ds_connection)
        {
            esp_mesh_post_toDS_state(false);
        }
        break;
    case MESH_AMR_EVENT_PARENT_CONNECTED:
        ESP_LOGI(TAG, "Parent connected");
        parent_connected = true;
        is_node = !(esp_mesh_is_root());
        if (is_node && !node_task_running)
        {
            node_task_running = true;
            xTaskCreate(node_main, "node_main", 4096, NULL, 5, NULL);
        }
        break;
    case MESH_AMR_EVENT_PARENT_DISCONNECTED:
        ESP_LOGI(TAG, "Parent disconnected");
        parent_connected = false;
        break;
    default:
        ESP_LOGW(TAG, "Unknown event ID received");
        break;
    }
}

/* ******************************************************************************** */
/*                               LED                                                */
/* ******************************************************************************** */

esp_err_t configure_led_Strip()
{
    // LED strip general initialization, according to your led board design
    led_strip_config_t strip_config = {
        // Set the GPIO that the LED is connected
        .strip_gpio_num = LED_GPIO,
        // Set the number of connected LEDs in the strip
        .max_leds = 1,
        // Set the pixel format of your LED strip
        //.led_pixel_format = LED_PIXEL_FORMAT_GRB,
        // LED strip model
        .led_model = LED_MODEL_WS2812,
        // In some cases, the logic is inverted
        .flags.invert_out = false,
    };

    // LED strip backend configuration: RMT
    led_strip_rmt_config_t rmt_config = {
        // Set the clock source
        .clk_src = RMT_CLK_SRC_DEFAULT,
        // Set the RMT counter clock
        .resolution_hz = LED_STRIP_RMT_RES_HZ,
        // Set the DMA feature (not supported on the ESP32-C6)
        .flags.with_dma = false,
    };

    // LED Strip object handle
    return led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip);
}

/* ******************************************************************************** */
/*                              MAIN                                                */
/* ******************************************************************************** */

void app_main()
{
    // User event loop args
    esp_event_loop_args_t loop_args = {
        .queue_size = 10,
        .task_name = "amr_iot_sensor_event_task", // Task name for the event loop task
        .task_priority = uxTaskPriorityGet(NULL),
        .task_stack_size = 2048,
        .task_core_id = tskNO_AFFINITY};
    ESP_ERROR_CHECK(esp_event_loop_create(&loop_args, &event_loop));

    // Register the event handler
    ESP_ERROR_CHECK(esp_event_handler_register_with(event_loop, AMR_IOT_EVENT, ESP_EVENT_ANY_ID, event_handler, NULL));

    // Get MAC address
    ESP_ERROR_CHECK(esp_base_mac_addr_get(mac));
    ESP_LOGI(TAG, "Retrieved MAC address");

    // If no LED available, remove
    ESP_ERROR_CHECK(configure_led_Strip());

    // Default configuration is:
    // - Temperature compensation enabled
    // - High resolution mode
    // - Continuous read mode
    // - ODR 100 Hz
    // - Block data update enabled
    // - Little endian
    // - Offset cancellation continuous mode enabled
    // - Low pass filter enabled
    ESP_ERROR_CHECK(esp_lis2mdl_init_default());
    ESP_LOGI(TAG, "LIS2MDL initialized successfully");

    // Init WiFi MESH network
    ESP_ERROR_CHECK(mesh_amr_init(event_loop));
    ESP_LOGI(TAG, "MESH network initialized successfully");
}