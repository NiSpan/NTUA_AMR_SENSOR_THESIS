#ifndef MESH_AMR_H
#define MESH_AMR_H

#include <stdbool.h>
#include "esp_err.h"
#include "esp_event.h"

// Define mesh configuration constants
#define CONFIG_MESH_CHANNEL 0
#define CONFIG_MESH_AP_CONNECTIONS 4

// Function to initialize the mesh
esp_err_t mesh_amr_init(esp_event_loop_handle_t event_loop);

#endif // MESH_AMR_H