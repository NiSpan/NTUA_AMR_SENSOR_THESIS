#ifndef MQTT_AMR_H
#define MQTT_AMR_H

#include "mqtt_client.h"

// Function to start MQTT
esp_err_t mqtt_start(esp_event_loop_handle_t event_loop);

// Function to publish a message to a topic
void mqtt_app_publish(const char* topic, const char* publish_string);

// Function to subscribe to a topic
void mqtt_app_subscribe(const char* topic);

// Function to stop MQTT
esp_err_t mqtt_app_stop(void);

#endif // MQTT_AMR_H