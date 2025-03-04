idf_component_register(SRCS "main.c"
                    REQUIRES mqtt_amr driver esp_lis2mdl esp_json nvs_flash esp_wifi mesh_amr
                    INCLUDE_DIRS ".")