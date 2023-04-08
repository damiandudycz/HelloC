#include <stdio.h>
#include "WiFi/wifi_client.h"
#include "WiFi/wifi_helpers.h"

#define WIFI_SSID "WiFI Name"
#define WIFI_PASSWORD "Hello World"
#define WIFI_IP "192.168.86.1"
#define WIFI_GATEWAY "192.168.86.170"
#define WIFI_NETMASK "255.255.255.0"
#define WIFI_SCAN_MAX_RESULTS 10
#define WIFI_FORCE_RECONFIGURE true
#define WIFI_USE_MANUAL_CONFIG true

// MARK: - Main application entry point.
void app_main() 
{
    // Platform assertions.
    static_assert (sizeof(uint8_t) == sizeof(unsigned char), "Char isn't 8-bit, aborting");

    struct wifi_client_config_t wifi_config = {
        .setup_nvs = true,
        .enable_logs = true
    };
    struct wifi_client_t wifi_client = WIFI_CLIENT(wifi_config);

    // Initialize main devices and settings related to WiFi.
    wifi_client_setup(&wifi_client);
    
    bool connection_restored = false;
    if (!WIFI_FORCE_RECONFIGURE) {
        wifi_client_restore_connection(&wifi_client, &connection_restored);        
    }

    if (!connection_restored) {
        printf("+-- Connecting with default WiFi network.\n");

        struct wifi_network_config *network_config = NULL;
        struct wifi_network_config nconfig = {
            .ssid = { WIFI_SSID },
            .password = { WIFI_PASSWORD }
        };
        network_config = &nconfig;

        struct wifi_address_config *manual_config = NULL;
        if (WIFI_USE_MANUAL_CONFIG) {
            const esp_netif_ip_info_t ipv4Config = {
                .ip = WIFI_GET_IPV4_ADDR(WIFI_IP),
                .netmask = WIFI_GET_IPV4_ADDR(WIFI_NETMASK),
                .gw = WIFI_GET_IPV4_ADDR(WIFI_GATEWAY)
            };
            struct wifi_address_config config = {
                .ipv4 = &ipv4Config,
                .ipv6 = NULL
            };
            manual_config = &config;
        }
        
        wifi_client_connect(&wifi_client, network_config, manual_config);
    }

    wifi_client_wait_for_ip(&wifi_client);

}
