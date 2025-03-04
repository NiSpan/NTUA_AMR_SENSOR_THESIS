#include "esp_json.h"
#include <stdlib.h>
#include <string.h>
#include "esp_system.h"
#include "esp_mac.h"

JsonESP *json_esp_create()
{
    JsonESP *json = (JsonESP *)malloc(sizeof(JsonESP));
    if (json == NULL)
    {
        return NULL;
    }
    json->jsonObject = cJSON_CreateObject();
    if (json->jsonObject == NULL)
    {
        free(json);
        return NULL;
    }
    return json;
}

esp_err_t json_esp_add_string(JsonESP *json, const char *key, const char *value)
{
    if (json == NULL || json->jsonObject == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    cJSON *item = cJSON_AddStringToObject(json->jsonObject, key, value);
    if (item == NULL)
    {
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t json_esp_add_number(JsonESP *json, const char *key, float value)
{
    if (json == NULL || json->jsonObject == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    cJSON *item = cJSON_AddNumberToObject(json->jsonObject, key, value);
    if (item == NULL)
    {
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t json_esp_add_bool(JsonESP *json, const char *key, int value)
{
    if (json == NULL || json->jsonObject == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    cJSON *item = cJSON_AddBoolToObject(json->jsonObject, key, value);
    if (item == NULL)
    {
        return ESP_FAIL;
    }
    return ESP_OK;
}

char *json_esp_stringify(const JsonESP *json)
{
    if (json && json->jsonObject)
    {
        return cJSON_Print(json->jsonObject);
    }
    return NULL;
}

void json_esp_free(JsonESP *json)
{
    if (json)
    {
        if (json->jsonObject)
        {
            cJSON_Delete(json->jsonObject);
        }
        free(json);
    }
}

esp_err_t json_esp_make_utesla_data_for_send(JsonESP *json, float utesla_x, float utesla_y, float utesla_z, float temperature, const uint8_t *mac)
{
    if (json == NULL || json->jsonObject == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    cJSON *item = cJSON_AddNumberToObject(json->jsonObject, "x_axis_uTesla", utesla_x);
    if (item == NULL)
    {
        return ESP_FAIL;
    }
    item = cJSON_AddNumberToObject(json->jsonObject, "y_axis_uTesla", utesla_y);
    if (item == NULL)
    {
        return ESP_FAIL;
    }
    item = cJSON_AddNumberToObject(json->jsonObject, "z_axis_uTesla", utesla_z);
    if (item == NULL)
    {
        return ESP_FAIL;
    }
    item = cJSON_AddNumberToObject(json->jsonObject, "temperature(Celsius)", temperature);
    if (item == NULL)
    {
        return ESP_FAIL;
    }
    // Convert MAC address to a hexadecimal string
    char mac_str[18]; // 12 hex digits + 5 colons + null terminator
    snprintf(mac_str, sizeof(mac_str), MACSTR, MAC2STR(mac));
    item = cJSON_AddStringToObject(json->jsonObject, "MAC Address", mac_str);
    if (item == NULL)
    {
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t json_esp_make_gauss_data_for_send(JsonESP *json, float gauss_x, float gauss_y, float gauss_z, float temperature, const uint8_t *mac)
{
    if (json == NULL || json->jsonObject == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    cJSON *item = cJSON_AddNumberToObject(json->jsonObject, "x_axis_Gauss", gauss_x);
    if (item == NULL)
    {
        return ESP_FAIL;
    }
    item = cJSON_AddNumberToObject(json->jsonObject, "y_axis_Gauss", gauss_y);
    if (item == NULL)
    {
        return ESP_FAIL;
    }
    item = cJSON_AddNumberToObject(json->jsonObject, "z_axis_Gauss", gauss_z);
    if (item == NULL)
    {
        return ESP_FAIL;
    }
    item = cJSON_AddNumberToObject(json->jsonObject, "emperature(Celsius)", temperature);
    if (item == NULL)
    {
        return ESP_FAIL;
    }
    // Convert MAC address to a hexadecimal string
    char mac_str[18]; // 12 hex digits + 5 colons + null terminator
    snprintf(mac_str, sizeof(mac_str), MACSTR, MAC2STR(mac));
    item = cJSON_AddStringToObject(json->jsonObject, "MAC Address", mac_str);
    if (item == NULL)
    {
        return ESP_FAIL;
    }

    return ESP_OK;
}