#include "mqtt_amr.h"
#include "esp_log.h"
#include "freertos/event_groups.h"
#include "mqtt_amr_secrets.h"

#include "event_amr.h"

static const char *TAG = "AMR_IOT_SENSOR_MQTT";
static esp_mqtt_client_handle_t client = NULL;
static esp_event_loop_handle_t mesh_event_loop;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_BEFORE_CONNECT:
        ESP_LOGI(TAG, "MQTT_EVENT_BEFORE_CONNECT");
        break;
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        ESP_ERROR_CHECK(esp_event_post_to(mesh_event_loop, AMR_IOT_EVENT, MQTT_AMR_EVENT_CONNECTED, NULL, 0, portMAX_DELAY));
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        ESP_ERROR_CHECK(esp_event_post_to(mesh_event_loop, AMR_IOT_EVENT, MQTT_AMR_EVENT_DISCONNECTED, NULL, 0, portMAX_DELAY));
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        ESP_LOGI(TAG, "TOPIC=%.*s", event->topic_len, event->topic);
        ESP_LOGI(TAG, "DATA=%.*s", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR: %d", event->error_handle->connect_return_code);
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

esp_err_t mqtt_start(esp_event_loop_handle_t event_loop)
{

    // Use this to post events to main event loop
    mesh_event_loop = event_loop;

    esp_err_t ret;
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = CONFIG_BROKER_URI,
        .credentials.username = CONFIG_BROKER_USERNAME,
        .credentials.authentication.password = CONFIG_BROKER_PASSWORD,
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    if (client == NULL)
    {
        ESP_LOGE(TAG, "Failed to initialize the MQTT client");
        return ESP_FAIL;
    }
    ret = esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to register the event handler");
        return ret;
    }
    ret = esp_mqtt_client_start(client);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to start the MQTT client");
        return ret;
    }
    return ESP_OK;
}

void mqtt_app_publish(const char *topic, const char *publish_string)
{
    if (client)
    {
        int msg_id = esp_mqtt_client_publish(client, topic, publish_string, 0, 2, 0);
        ESP_LOGI(TAG, "sent publish returned msg_id=%d", msg_id);
    }
}

void mqtt_app_subscribe(const char *topic)
{
    if (client)
    {
        int msg_id = esp_mqtt_client_subscribe(client, topic, 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
    }
}

esp_err_t mqtt_app_stop(void)
{
    if (client)
    {
        esp_err_t ret = esp_mqtt_client_stop(client);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to stop the MQTT client");
            return ret;
        }
        else
        {
            ESP_LOGI(TAG, "MQTT client stopped successfully");
        }
        ret = esp_mqtt_client_destroy(client);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to destroy the MQTT client");
            return ret;
        }
        else
        {
            ESP_LOGI(TAG, "MQTT client destroyed successfully");
        }
        client = NULL;
        mesh_event_loop = NULL;

        return ESP_OK;
    }
    return ESP_FAIL;
}