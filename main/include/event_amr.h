#ifndef EVENT_AMR_H
#define EVENT_AMR_H

#include "esp_event.h"

// Define a custom event base
ESP_EVENT_DECLARE_BASE(AMR_IOT_EVENT);

// Define custom event IDs
typedef enum
{
    MQTT_AMR_EVENT_CONNECTED,    // MQTT Connected event - only the current root will listen to mqtt events
    MQTT_AMR_EVENT_DISCONNECTED, // MQTT Disconnected event - only the current root will listen to mqtt events
    MQTT_AMR_EVENT_SUBSCRIBED,   // Unused
    MQTT_AMR_EVENT_UNSUBSCRIBED, // Unused
    MQTT_AMR_EVENT_PUBLISHED,    // Unused
    MQTT_AMR_EVENT_DATA,         // Unused
    MQTT_AMR_EVENT_ERROR,        // Unused
    MQTT_AMR_GOT_COMMAND,        // Unused
    MESH_AMR_ROUTER_LOST,        // Unused
    MESH_AMR_DS_OFF,             // If root has lost IP or connection to parent, the MESH cannot communicate with external network DS
    MESH_AMR_DS_ON,              // Communication with external network DS is possible
    MESH_AMR_GOT_IP,             // The root has obtained an IP address and can start the MQTT Client
    MESH_WIFI_STA_DISCONNECTED,  // The root has lost WiFi connection
    MESH_AMR_EVENT_STARTED,
    MESH_AMR_EVENT_STOPPED,
    MESH_AMR_EVENT_CHANNEL_SWITCH,
    MESH_AMR_EVENT_CHILD_CONNECTED,
    MESH_AMR_EVENT_CHILD_DISCONNECTED,
    MESH_AMR_EVENT_ROUTING_TABLE_ADD,
    MESH_AMR_EVENT_ROUTING_TABLE_REMOVE,
    MESH_AMR_EVENT_NO_PARENT_FOUND,
    MESH_AMR_EVENT_PARENT_CONNECTED,    // The parent of the mesh node has been connected
    MESH_AMR_EVENT_PARENT_DISCONNECTED, // The parent of the mesh node has been disconnected
    MESH_AMR_EVENT_LAYER_CHANGE,
    MESH_AMR_EVENT_TODS_STATE,
    MESH_AMR_EVENT_VOTE_STARTED,
    MESH_AMR_EVENT_VOTE_STOPPED,
    MESH_AMR_EVENT_ROOT_ADDRESS,
    MESH_AMR_EVENT_ROOT_SWITCH_REQ,
    MESH_AMR_EVENT_ROOT_SWITCH_ACK,
    MESH_AMR_EVENT_ROOT_ASKED_YIELD,
    MESH_AMR_EVENT_ROOT_FIXED,
    MESH_AMR_EVENT_SCAN_DONE,
    MESH_AMR_EVENT_NETWORK_STATE,
    MESH_AMR_EVENT_STOP_RECONNECTION,
    MESH_AMR_EVENT_FIND_NETWORK,
    MESH_AMR_EVENT_ROUTER_SWITCH,
    MESH_AMR_EVENT_PS_PARENT_DUTY,
    MESH_AMR_EVENT_PS_CHILD_DUTY,
} my_event_id_t;

#endif // EVENT_AMR_H