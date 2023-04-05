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

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------

void wifi_client_setup(struct wifi_client *client) 
{

    assert(client->status == NOT_INITIALIZED);
    client->status = DISCONNECTED;
    
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    client->sta_interface = esp_netif_create_default_wifi_sta();

}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------

void wifi_client_scan(const struct wifi_client *client, const uint16_t max_results, wifi_ap_record_t results[], uint16_t *results_count) 
{

    assert(client->status == DISCONNECTED);

    wifi_config_t wifi_config = {0};
    wifi_config.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;

    // Start WiFi in scanning mode.
    ESP_ERROR_CHECK(esp_wifi_restore());
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    // Start scanning.
    ESP_ERROR_CHECK(esp_wifi_scan_start(NULL, true));
    // Get results.
    *results_count = max_results; // Setting to maximum value first is used by esp_wifi_scan_get_ap_records.
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(results_count, results));
    // Clean.
    ESP_ERROR_CHECK(esp_wifi_stop());

}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------

static void wifi_client_connect_using_config(struct wifi_client *client, wifi_config_t *wifi_config) 
{

    assert(client->status == DISCONNECTED);

    // Start WiFi in connection mdoe.
    ESP_ERROR_CHECK(esp_wifi_restore());
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    // Connect WiFi.
    ESP_ERROR_CHECK(esp_wifi_connect());

    client->status = CONNECTED;

}

void wifi_client_connect(struct wifi_client *client, const char ssid[], const uint8_t bssid[], const char password[], const struct wifi_manual_config *manual_config) 
{

    assert(client->status == DISCONNECTED);

    wifi_config_t wifi_config = {0};
    strcpy((char *)wifi_config.sta.ssid, ssid);
    strcpy((char *)wifi_config.sta.password, password);
    if (bssid != NULL) {
        wifi_config.sta.bssid_set = true;
        memcpy(wifi_config.sta.bssid, &bssid, 6);
    }

    // TODO: Przenieść to do funkcji wyej i w jakiś sposob rowniez wykrywać potrzebe wl/wyl dhcp przy przywracaniu konfiguracji z NVS.

    if (manual_config != NULL) {
        ESP_ERROR_CHECK(esp_netif_dhcpc_stop(client->sta_interface));

        // Set IPv4 configuration.
        if (strlen(manual_config->ipv4.addr) > 0) {
            esp_netif_ip_info_t ip_info = {
                .ip.addr = esp_ip4addr_aton(manual_config->ipv4.addr),
                .gw.addr = esp_ip4addr_aton(manual_config->ipv4.gateway),
                .netmask.addr = esp_ip4addr_aton(manual_config->ipv4.netmask)
            };
            ESP_ERROR_CHECK(esp_netif_set_ip_info(client->sta_interface, &ip_info));
        }

        // Set IPv6 configuration.
        if (strlen(manual_config->ipv6.addr) > 0) {
            // TODO:
        }
    }
    else {
        ESP_ERROR_CHECK(esp_netif_dhcpc_start(client->sta_interface));
    }

    wifi_client_connect_using_config(client, &wifi_config);

}

void wifi_client_disconnect(struct wifi_client *client) 
{

    assert(client->status == CONNECTED);

    ESP_ERROR_CHECK(esp_wifi_disconnect());
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_restore());

    client->status = DISCONNECTED;

}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------

void wifi_client_restore_connection(struct wifi_client *client, bool *restored) {

    assert(client->status == DISCONNECTED);

    // if ssid == FF:FF:FF:FF:FF:FF, means its not set.
    bool is_bssid_set(uint8_t bssid[6]) {
        for (int i = 0; i < WIFI_BSSID_LENGTH; ++i) {
            if (bssid[i] != 0xff) {
                return true;
            }
        }
        return false;
    }
    
    wifi_config_t wifi_config;
    if (esp_wifi_get_config(WIFI_IF_STA, &wifi_config) == ESP_OK && (strlen((char *)wifi_config.sta.ssid) || is_bssid_set(wifi_config.sta.bssid))) {
        wifi_client_connect_using_config(client, &wifi_config);
        *restored = true;
    }
    else {
        *restored = false;
    }

}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------

void wifi_client_wait_for_ip(struct wifi_client *client) 
{

    // Get current IP, and exit if it's already set.
    esp_netif_ip_info_t ip_info = {0};
    esp_ip4_addr_t empty_ip = {0}; // Used for compartion
    ESP_ERROR_CHECK(esp_netif_get_ip_info(client->sta_interface, &ip_info));
    if (memcmp(&ip_info.ip, &empty_ip, sizeof(esp_ip4_addr_t)) != 0) {
        // IP is already known
        return;
    }

    void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
        if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
            xEventGroupSetBits((EventGroupHandle_t)arg, BIT0);
        }
    }

    EventGroupHandle_t wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, wifi_event_group));    
    xEventGroupWaitBits(wifi_event_group, BIT0, pdFALSE, pdTRUE, portMAX_DELAY);
    esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler);
    vEventGroupDelete(wifi_event_group);

}
