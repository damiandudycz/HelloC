#include "WiFI/wifi_client.h"
#include "WiFI/wifi_helpers.h"

#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <nvs_flash.h>
#include <lwip/err.h>
#include <lwip/sys.h>
#include <lwip/ip6_addr.h>

// ---------------------------------------------------------------------------------------------------------------------

enum wifi_result_t wifi_client_setup(struct wifi_client_t *client) 
{
    if (!client) { return WIFI_REQUIRED_PARAMETER_IS_NULL; }
    if (client->status != NOT_INITIALIZED) { return WIFI_WRONG_CLIENT_STATUS; }

    client->status = DISCONNECTED;
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    if (esp_netif_init() != ESP_OK) { return WIFI_ESP_ERROR; }
    if (esp_event_loop_create_default() != ESP_OK) { return WIFI_ESP_ERROR; }
    if (esp_wifi_init(&cfg) != ESP_OK) { return WIFI_ESP_ERROR; }

    client->sta_interface = esp_netif_create_default_wifi_sta();

    return WIFI_OK;
}

// ---------------------------------------------------------------------------------------------------------------------

enum wifi_result_t wifi_client_scan(const struct wifi_client_t *client, const uint16_t max_results, wifi_ap_record_t *results, uint16_t *results_count) 
{
    if (!client) { return WIFI_REQUIRED_PARAMETER_IS_NULL; }
    if (!results) { return WIFI_REQUIRED_PARAMETER_IS_NULL; }
    if (!results_count) { return WIFI_REQUIRED_PARAMETER_IS_NULL; }
    if (client->status != DISCONNECTED) { return WIFI_WRONG_CLIENT_STATUS; }

    wifi_config_t wifi_config = {0};
    wifi_config.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;

    // Start WiFi in scanning mode.
    if (esp_wifi_restore() != ESP_OK) { return WIFI_ESP_ERROR; }
    if (esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK) { return WIFI_ESP_ERROR; }
    if (esp_wifi_set_config(WIFI_IF_STA, &wifi_config) != ESP_OK) { return WIFI_ESP_ERROR; }
    if (esp_wifi_start() != ESP_OK) { return WIFI_ESP_ERROR; }
    // Start scanning.
    if (esp_wifi_scan_start(NULL, true) != ESP_OK) { return WIFI_ESP_ERROR; }
    // Get results.
    *results_count = max_results; // Setting to maximum value first is used by esp_wifi_scan_get_ap_records.
    if (esp_wifi_scan_get_ap_records(results_count, results) != ESP_OK) { return WIFI_ESP_ERROR; }
    // Clean.
    if (esp_wifi_stop() != ESP_OK) { return WIFI_ESP_ERROR; }

    return WIFI_OK;
}

// ---------------------------------------------------------------------------------------------------------------------

// Add additional object pointer to details passwd to event handler if needed.
struct wifi_event_handler_data 
{
    EventGroupHandle_t eventGroupHandle;
    void *object;
};

static void wifi_client_connect_using_config_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    struct wifi_event_handler_data *data = arg;
    switch (event_id) {
    case WIFI_EVENT_STA_DISCONNECTED:
        *(bool *)data->object = false;
        xEventGroupSetBits(data->eventGroupHandle, BIT0);
        break;
    case WIFI_EVENT_STA_CONNECTED:
        *(bool *)data->object = true;
        xEventGroupSetBits(data->eventGroupHandle, BIT0);
        break;
    default:
        break;
    }
}

enum wifi_result_t wifi_client_connect_using_config(struct wifi_client_t *client, wifi_config_t *wifi_config) 
{
    if (!client) { return WIFI_REQUIRED_PARAMETER_IS_NULL; }
    if (!wifi_config) { return WIFI_REQUIRED_PARAMETER_IS_NULL; }
    if (client->status != DISCONNECTED) { return WIFI_WRONG_CLIENT_STATUS; }

    // Start WiFi in connection mdoe.
    if (esp_wifi_restore() != ESP_OK) { return WIFI_ESP_ERROR; }
    if (esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK) { return WIFI_ESP_ERROR; }
    if (esp_wifi_set_config(WIFI_IF_STA, wifi_config) != ESP_OK) { return WIFI_ESP_ERROR; }
    if (esp_wifi_start() != ESP_OK) { return WIFI_ESP_ERROR; }

    // Connect WiFi.
    if (esp_wifi_connect() != ESP_OK) { return WIFI_ESP_ERROR; }

    EventGroupHandle_t wifi_event_group = xEventGroupCreate();
    bool success;
    struct wifi_event_handler_data event_data = {
        .eventGroupHandle = wifi_event_group,
        .object = &success
    };

    if (
        esp_event_handler_instance_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            &wifi_client_connect_using_config_event_handler,
            &event_data,
            NULL
        )
    ) { return WIFI_ESP_ERROR; }

    xEventGroupWaitBits(wifi_event_group, BIT0, pdFALSE, pdTRUE, portMAX_DELAY);
    esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_client_connect_using_config_event_handler);
    vEventGroupDelete(wifi_event_group);

    if (success) {
        client->status = CONNECTED;
        return WIFI_OK;
    }
    else {
        if (esp_wifi_disconnect() != ESP_OK) { return WIFI_ESP_ERROR; }
        if (esp_wifi_stop() != ESP_OK) { return WIFI_ESP_ERROR; }
        if (esp_wifi_restore() != ESP_OK) { return WIFI_ESP_ERROR; }
        client->status = DISCONNECTED;
        return WIFI_CONNECTION_FAILED;
    }

}

enum wifi_result_t wifi_client_connect(struct wifi_client_t *client, const char *ssid, const char *password, const struct wifi_manual_config *manual_config) 
{
    if (!client) { return WIFI_REQUIRED_PARAMETER_IS_NULL; }
    if (!ssid) { return WIFI_REQUIRED_PARAMETER_IS_NULL; }
    if (client->status != DISCONNECTED) { return WIFI_WRONG_CLIENT_STATUS; }

    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    if (password) {
        strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
    }

    if (manual_config) {
        if (esp_netif_dhcpc_stop(client->sta_interface) != ESP_OK) { return WIFI_ESP_ERROR; }

        // Set IPv4 configuration.
        if (manual_config->ipv4) {
            if (esp_netif_set_ip_info(client->sta_interface, manual_config->ipv4) != ESP_OK) { return WIFI_ESP_ERROR; }
        }
        // TODO: IPv6
    }
    else {
        if (esp_netif_dhcpc_start(client->sta_interface) != ESP_OK) { return WIFI_ESP_ERROR; }
    }

    return wifi_client_connect_using_config(client, &wifi_config);
}

enum wifi_result_t wifi_client_disconnect(struct wifi_client_t *client) 
{
    if (!client) { return WIFI_REQUIRED_PARAMETER_IS_NULL; }
    if (client->status != CONNECTED) { return WIFI_WRONG_CLIENT_STATUS; }

    if (esp_wifi_disconnect() != ESP_OK) { return WIFI_ESP_ERROR; }
    if (esp_wifi_stop() != ESP_OK) { return WIFI_ESP_ERROR; }
    if (esp_wifi_restore() != ESP_OK) { return WIFI_ESP_ERROR; }

    client->status = DISCONNECTED;

    return WIFI_OK;
}

// ---------------------------------------------------------------------------------------------------------------------

enum wifi_result_t wifi_client_restore_connection(struct wifi_client_t *client, bool *restored) 
{
    if (!client) { return WIFI_REQUIRED_PARAMETER_IS_NULL; }
    if (client->status != DISCONNECTED) { return WIFI_WRONG_CLIENT_STATUS; }

    wifi_config_t wifi_config;
    if (esp_wifi_get_config(WIFI_IF_STA, &wifi_config) == ESP_OK && (wifi_config.sta.ssid || wifi_config.sta.bssid)) {
        wifi_client_connect_using_config(client, &wifi_config);
        *restored = true;
    }
    else {
        *restored = false;
    }

    return WIFI_OK;
}

// ---------------------------------------------------------------------------------------------------------------------

static void wifi_client_wait_for_ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    struct wifi_event_handler_data *data = arg;
    xEventGroupSetBits(data->eventGroupHandle, BIT0);
}

enum wifi_result_t wifi_client_wait_for_ip(struct wifi_client_t *client) 
{
    if (!client) { return WIFI_REQUIRED_PARAMETER_IS_NULL; }
    if (client->status != CONNECTED) { return WIFI_WRONG_CLIENT_STATUS; }
    
    // Get current IP, and exit if it's already set.
    esp_netif_ip_info_t ip_info = {0};
    if (esp_netif_get_ip_info(client->sta_interface, &ip_info) != ESP_OK) { return WIFI_ESP_ERROR; }
    if (ip_info.ip.addr) {
        // IP is already known
        return WIFI_OK;
    }

    EventGroupHandle_t wifi_event_group = xEventGroupCreate();

    struct wifi_event_handler_data event_data = {
        .eventGroupHandle = wifi_event_group,
        .object = NULL
    };

    if (
        esp_event_handler_instance_register(
            IP_EVENT, 
            IP_EVENT_STA_GOT_IP, 
            &wifi_client_wait_for_ip_event_handler, 
            &event_data, 
            NULL
        )   
    ) { return WIFI_ESP_ERROR; }
    
    xEventGroupWaitBits(wifi_event_group, BIT0, pdFALSE, pdTRUE, portMAX_DELAY);
    esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_client_wait_for_ip_event_handler);
    vEventGroupDelete(wifi_event_group);

    return WIFI_OK;
}
