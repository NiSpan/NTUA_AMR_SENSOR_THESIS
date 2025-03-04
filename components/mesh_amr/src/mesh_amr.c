#include "mesh_amr.h"
#include "mesh_amr_secrets.h"
#include "esp_log.h"
#include "esp_mesh.h"
#include "esp_mesh_internal.h"
#include "esp_event.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "nvs_flash.h"

#include "event_amr.h"

static const char *TAG = "AMR_IOT_SENSOR_MESH";

static const uint8_t MESH_ID[6] = {0x02, 0x04, 0x01, 0x09, 0x08, 0x09};
static mesh_addr_t mesh_parent_addr;
static int mesh_layer = -1;
static esp_netif_t *netif_sta = NULL;
static esp_event_loop_handle_t mesh_event_loop;

static void ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == IP_EVENT) {
        switch (event_id) {
            case IP_EVENT_STA_GOT_IP: {
                ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
                ESP_LOGI(TAG, "<IP_EVENT_STA_GOT_IP> IP: " IPSTR, IP2STR(&event->ip_info.ip));
                if (esp_mesh_is_root()) {
                    ESP_ERROR_CHECK(esp_event_post_to(mesh_event_loop, AMR_IOT_EVENT, MESH_AMR_GOT_IP, NULL, 0, portMAX_DELAY));
                }
                break;
            }
            case IP_EVENT_STA_LOST_IP: {
                ESP_LOGI(TAG, "<IP_EVENT_STA_LOST_IP>");
                if (esp_mesh_is_root()) {
                    ESP_ERROR_CHECK(esp_mesh_post_toDS_state(false));
                }
                break;
            }
        }
    } else if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_DISCONNECTED: {
                ESP_LOGI(TAG, "<WIFI_EVENT_STA_DISCONNECTED>");
                if (esp_mesh_is_root()) {
                    ESP_ERROR_CHECK(esp_mesh_post_toDS_state(false));
                }
                break;
            }
        }
    }
}

static void mesh_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    mesh_addr_t id = {0,};

    switch (event_id) {
        case MESH_EVENT_STARTED: {
                esp_mesh_get_id(&id);
                ESP_LOGI(TAG, "<MESH_EVENT_MESH_STARTED>ID:"MACSTR"", MAC2STR(id.addr));
                }
            break;
        case MESH_EVENT_STOPPED: {
                ESP_LOGI(TAG, "<MESH_EVENT_STOPPED>");
                mesh_layer = esp_mesh_get_layer();
                }
            break;
        case MESH_EVENT_CHANNEL_SWITCH: {
                mesh_event_channel_switch_t *channel_switch = (mesh_event_channel_switch_t *)event_data;
                ESP_LOGI(TAG, "<MESH_EVENT_CHANNEL_SWITCH>new channel:%d", channel_switch->channel);
                }
            break;
        case MESH_EVENT_CHILD_CONNECTED: {
                mesh_event_child_connected_t *child_connected = (mesh_event_child_connected_t *)event_data;
                ESP_LOGI(TAG, "<MESH_EVENT_CHILD_CONNECTED>ID:,"MACSTR"", MAC2STR(child_connected->mac));
                }
            break;
        case MESH_EVENT_CHILD_DISCONNECTED: {
                mesh_event_child_disconnected_t *child_disconnected = (mesh_event_child_disconnected_t *)event_data;
                ESP_LOGI(TAG, "<MESH_EVENT_CHILD_DISCONNECTED>ID:"MACSTR"", MAC2STR(child_disconnected->mac));
                }
            break;
        case MESH_EVENT_ROUTING_TABLE_ADD: { 
                /**< routing table is changed by adding newly joined children */
                mesh_event_routing_table_change_t *routing_table = (mesh_event_routing_table_change_t *)event_data;
                ESP_LOGW(TAG, "<MESH_EVENT_ROUTING_TABLE_ADD>add %d, new:%d, layer:%d", routing_table->rt_size_change, routing_table->rt_size_new, mesh_layer);
                }
            break;
        case MESH_EVENT_ROUTING_TABLE_REMOVE: { 
                /**< routing table is changed by removing children */
                mesh_event_routing_table_change_t *routing_table = (mesh_event_routing_table_change_t *)event_data;
                ESP_LOGW(TAG, "<MESH_EVENT_ROUTING_TABLE_REMOVE>remove %d, new:%d, layer:%d", routing_table->rt_size_change, routing_table->rt_size_new, mesh_layer);
                }
            break;
        case MESH_EVENT_NO_PARENT_FOUND: {
                mesh_event_no_parent_found_t *no_parent = (mesh_event_no_parent_found_t *)event_data;
                ESP_LOGI(TAG, "<MESH_EVENT_NO_PARENT_FOUND>scan times:%d", no_parent->scan_times);
                }
            break;
        case MESH_EVENT_PARENT_CONNECTED: {
                mesh_event_connected_t *connected = (mesh_event_connected_t *)event_data;
                esp_mesh_get_id(&id);
                mesh_layer = connected->self_layer;
                memcpy(&mesh_parent_addr.addr, connected->connected.bssid, 6);
                ESP_LOGI(TAG, "<MESH_EVENT_PARENT_CONNECTED>parent:"MACSTR"%s, ID:"MACSTR", duty:%d",  MAC2STR(mesh_parent_addr.addr), esp_mesh_is_root() ? "<ROOT>" : (mesh_layer == 2) ? "<layer2>" : "", MAC2STR(id.addr), connected->duty);
                if (esp_mesh_is_root()) {
                    esp_netif_dhcpc_stop(netif_sta);
                    esp_netif_dhcpc_start(netif_sta);
                    }
                ESP_ERROR_CHECK(esp_event_post_to(mesh_event_loop, AMR_IOT_EVENT, MESH_AMR_EVENT_PARENT_CONNECTED, NULL, 0, portMAX_DELAY));
                }
            break;
        case MESH_EVENT_PARENT_DISCONNECTED: {
                mesh_event_disconnected_t *disconnected = (mesh_event_disconnected_t *)event_data;
                ESP_LOGI(TAG, "<MESH_EVENT_PARENT_DISCONNECTED>reason:%d", disconnected->reason);
                mesh_layer = esp_mesh_get_layer();
                ESP_ERROR_CHECK(esp_event_post_to(mesh_event_loop, AMR_IOT_EVENT, MESH_AMR_EVENT_PARENT_DISCONNECTED, NULL, 0, portMAX_DELAY));
                }
            break;
        case MESH_EVENT_LAYER_CHANGE: {
                mesh_event_layer_change_t *layer_change = (mesh_event_layer_change_t *)event_data;
                mesh_layer = layer_change->new_layer;
                ESP_LOGI(TAG, "<MESH_EVENT_LAYER_CHANGE>new layer:%d", mesh_layer);
                }
            break;
        case MESH_EVENT_TODS_STATE: {
                /**< state represents whether the root is able to access external IP network.
                This state is a manual event that needs to be triggered with esp_mesh_post_toDS_state(). */
                mesh_event_toDS_state_t *toDS_state = (mesh_event_toDS_state_t *)event_data;
                ESP_LOGI(TAG, "<MESH_EVENT_TODS_STATE>Does the root have access to external network: %s", *toDS_state == MESH_TODS_REACHABLE ? "YES" : "NO");
                if (*toDS_state == MESH_TODS_REACHABLE) {
                    ESP_ERROR_CHECK(esp_event_post_to(mesh_event_loop, AMR_IOT_EVENT, MESH_AMR_DS_ON, NULL, 0, portMAX_DELAY));
                } else {
                    ESP_ERROR_CHECK(esp_event_post_to(mesh_event_loop, AMR_IOT_EVENT, MESH_AMR_DS_OFF, NULL, 0, portMAX_DELAY));
                }
            }
            break;
        case MESH_EVENT_VOTE_STARTED: {
                /**< the process of voting a new root is started either by children or by the root */
                ESP_LOGI(TAG, "<MESH_EVENT_VOTE_STARTED>");
                }
            break;
        case MESH_EVENT_VOTE_STOPPED: {
                /**< the process of voting a new root is stopped */
                ESP_LOGI(TAG, "<MESH_EVENT_VOTE_STOPPED>");
                }
            break;
        case MESH_EVENT_ROOT_ADDRESS: {
                mesh_event_root_address_t *root_addr = (mesh_event_root_address_t *)event_data;
                ESP_LOGI(TAG, "<MESH_EVENT_ROOT_ADDRESS>root address:"MACSTR"", MAC2STR(root_addr->addr));
                }
            break;
        case MESH_EVENT_ROOT_SWITCH_REQ: { 
                /**< root switch request sent from a new voted root candidate */
                mesh_event_root_switch_req_t *switch_req = (mesh_event_root_switch_req_t *)event_data;
                ESP_LOGI(TAG, "<MESH_EVENT_ROOT_SWITCH_REQ>reason:%d, Requested by:"MACSTR"", switch_req->reason, MAC2STR( switch_req->rc_addr.addr));
                }
            break;
    case MESH_EVENT_ROOT_SWITCH_ACK: {
            /**< New root - root switch acknowledgment responds the above request sent from current root */
            mesh_layer = esp_mesh_get_layer();
            esp_mesh_get_parent_bssid(&mesh_parent_addr);
            ESP_LOGI(TAG, "<MESH_EVENT_ROOT_SWITCH_ACK>layer:%d, parent:"MACSTR"", mesh_layer, MAC2STR(mesh_parent_addr.addr));
            }
        break;
    case MESH_EVENT_ROOT_ASKED_YIELD: {
            mesh_event_root_conflict_t *root_conflict = (mesh_event_root_conflict_t *)event_data;
            ESP_LOGI(TAG, "<MESH_EVENT_ROOT_ASKED_YIELD>Request by:"MACSTR", with RSSI:%d, Number of devices in network:%d", MAC2STR(root_conflict->addr), root_conflict->rssi, root_conflict->capacity);
            }
        break;
    case MESH_EVENT_ROOT_FIXED: {
            mesh_event_root_fixed_t *root_fixed = (mesh_event_root_fixed_t *)event_data;
            ESP_LOGI(TAG, "<MESH_EVENT_ROOT_FIXED>%s", root_fixed->is_fixed ? "fixed" : "not fixed");
            }
        break;
    case MESH_EVENT_SCAN_DONE: {
            mesh_event_scan_done_t *scan_done = (mesh_event_scan_done_t *)event_data;
            ESP_LOGI(TAG, "<MESH_EVENT_SCAN_DONE>Number of APs scanned:%d", scan_done->number);
            }
        break;
    case MESH_EVENT_NETWORK_STATE: {
            /**< network state, whether current mesh network has a root. */
            mesh_event_network_state_t *network_state = (mesh_event_network_state_t *)event_data;
            ESP_LOGI(TAG, "<MESH_EVENT_NETWORK_STATE>is_rootless:%d", network_state->is_rootless);
            }
        break;
    case MESH_EVENT_STOP_RECONNECTION: {
            /**< the root stops reconnecting to the router and non-root devices stop reconnecting to their parents. */
            ESP_LOGI(TAG, "<MESH_EVENT_STOP_RECONNECTION>");
            }
        break;
    case MESH_EVENT_FIND_NETWORK: {
            /**< when the channel field in mesh configuration is set to zero, mesh stack will perform a
            full channel scan to find a mesh network that can join, and return the channel value
            after finding it. */
            mesh_event_find_network_t *find_network = (mesh_event_find_network_t *)event_data;
            ESP_LOGI(TAG, "<MESH_EVENT_FIND_NETWORK>new channel:%d, router BSSID:"MACSTR"", find_network->channel, MAC2STR(find_network->router_bssid));
            }
        break;
    case MESH_EVENT_ROUTER_SWITCH: {
            /**< if users specify BSSID of the router in mesh configuration, when the root connects to another
            router with the same SSID, this event will be posted and the new router information is attached. */
            mesh_event_router_switch_t *router_switch = (mesh_event_router_switch_t *)event_data;
            ESP_LOGI(TAG, "<MESH_EVENT_ROUTER_SWITCH>Found router with SSID:%s, channel:%d, BSSID:"MACSTR"", router_switch->ssid, router_switch->channel, MAC2STR(router_switch->bssid));
            }
        break;
    case MESH_EVENT_PS_PARENT_DUTY: {
            mesh_event_ps_duty_t *ps_duty = (mesh_event_ps_duty_t *)event_data;
            ESP_LOGI(TAG, "<MESH_EVENT_PS_PARENT_DUTY>duty:%d", ps_duty->duty);
            }
        break;
    case MESH_EVENT_PS_CHILD_DUTY: {
            mesh_event_ps_duty_t *ps_duty = (mesh_event_ps_duty_t *)event_data;
            ESP_LOGI(TAG, "<MESH_EVENT_PS_CHILD_DUTY>cidx:%d, "MACSTR", duty:%d", ps_duty->child_connected.aid-1, MAC2STR(ps_duty->child_connected.mac), ps_duty->duty);
            }
        break;
    default:
        ESP_LOGI(TAG, "unknown id:%" PRId32 "", event_id);
        break;
    }
}

esp_err_t mesh_amr_init(esp_event_loop_handle_t event_loop) {
    
    // Use this to post events to main event loop
    mesh_event_loop = event_loop;

    //esp_err_t ret;
    ESP_ERROR_CHECK(nvs_flash_init());

    // Initialize TCP/IP
    ESP_ERROR_CHECK(esp_netif_init());

    // Create default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /*  create network interfaces for mesh (only station instance saved for further manipulation, soft AP instance ignored */
    ESP_ERROR_CHECK(esp_netif_create_default_wifi_mesh_netifs(&netif_sta, NULL));
    /*  wifi initialization */
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&config));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &ip_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &ip_event_handler, NULL));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    ESP_ERROR_CHECK(esp_wifi_start());
    /*  mesh initialization */
    ESP_ERROR_CHECK(esp_mesh_init());
    /*  register mesh events handler */
    ESP_ERROR_CHECK(esp_event_handler_register(MESH_EVENT, ESP_EVENT_ANY_ID, &mesh_event_handler, NULL));
    esp_mesh_topology_t mesh_topo = MESH_TOPO_TREE;
    /*  set mesh topology */
    ESP_ERROR_CHECK(esp_mesh_set_topology(mesh_topo));

    /*  set mesh max layer according to the topology */
    ESP_ERROR_CHECK(esp_mesh_set_max_layer(10));
    ESP_ERROR_CHECK(esp_mesh_set_vote_percentage(0.9));
    ESP_ERROR_CHECK(esp_mesh_set_xon_qsize(128));

    /* Enable mesh PS function */
    ESP_ERROR_CHECK(esp_mesh_enable_ps());
    ESP_ERROR_CHECK(esp_mesh_set_ap_assoc_expire(20));
    ESP_ERROR_CHECK(esp_mesh_allow_root_conflicts(false));
    /* Enable the Mesh IE encryption by default */
    mesh_cfg_t cfg = MESH_INIT_CONFIG_DEFAULT();
    /* mesh ID */
    memcpy((uint8_t *) &cfg.mesh_id, MESH_ID, 6);
    /* channel (must match the router's channel) */
    cfg.channel = CONFIG_MESH_CHANNEL;
    /* router */
    cfg.router.ssid_len = strlen(CONFIG_MESH_ROUTER_SSID);
    memcpy((uint8_t *) &cfg.router.ssid, CONFIG_MESH_ROUTER_SSID, cfg.router.ssid_len);
    memcpy((uint8_t *) &cfg.router.password, CONFIG_MESH_ROUTER_PASSWD,strlen(CONFIG_MESH_ROUTER_PASSWD));
    /* mesh softAP */
    cfg.mesh_ap.max_connection = CONFIG_MESH_AP_CONNECTIONS;
    memcpy((uint8_t *) &cfg.mesh_ap.password, CONFIG_MESH_AP_PASSWD, strlen(CONFIG_MESH_AP_PASSWD));
    ESP_ERROR_CHECK(esp_mesh_set_config(&cfg));
    /* mesh start */
    ESP_ERROR_CHECK(esp_mesh_start());

    ESP_LOGI(TAG, "Mesh network started");
    return ESP_OK;
}