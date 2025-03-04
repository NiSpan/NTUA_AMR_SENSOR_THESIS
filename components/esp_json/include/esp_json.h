#ifndef ESP_JSON_H
#define ESP_JSON_H

#include "cJSON.h"
#include "esp_system.h"

typedef struct
{
    cJSON *jsonObject;
} JsonESP;

JsonESP *json_esp_create();
esp_err_t json_esp_add_string(JsonESP *json, const char *key, const char *value);
esp_err_t json_esp_add_number(JsonESP *json, const char *key, float value);
esp_err_t json_esp_add_bool(JsonESP *json, const char *key, int value);
char *json_esp_stringify(const JsonESP *json);
void json_esp_free(JsonESP *json);
esp_err_t json_esp_make_utesla_data_for_send(JsonESP *json, float utesla_x, float utesla_y, float utesla_z, float temperature, const uint8_t *mac);
esp_err_t json_esp_make_gauss_data_for_send(JsonESP *json, float gauss_x, float gauss_y, float gauss_z, float temperature, const uint8_t *mac);

#endif // ESP_JSON_H
