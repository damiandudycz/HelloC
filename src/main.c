#include <stdio.h>
#include "WiFi/wifi_client.h"
#include "WiFi/wifi_helpers.h"

#define WIFI_SSID "Home 835B"
#define WIFI_PASSWORD "wzdtrmaso4"
#define WIFI_IP "192.168.86.1"
#define WIFI_GATEWAY "192.168.86.170"
#define WIFI_NETMASK "255.255.255.0"
#define WIFI_SCAN_MAX_RESULTS 10

// MARK: - Main application entry point.
void app_main() 
{
    // Platform assertions.
    static_assert (sizeof(uint8_t) == sizeof(unsigned char), "Char isn't 8-bit, aborting");

    struct wifi_client_t wifi_client = {0};

    // Initialize main devices and settings related to WiFi.
    nvs_initialize();
    wifi_disable_logs();
    wifi_client_setup(&wifi_client);
    
    bool connection_restored = false;
    wifi_client_restore_connection(&wifi_client, &connection_restored);

    if (connection_restored) {
        printf("+-- WiFi connection restored.\n");
    }
    else {
        printf("+-- Connecting with default WiFi network.\n");

        const esp_netif_ip_info_t ipv4Config = {
            .ip = WIFI_GET_IPV4_ADDR(WIFI_IP),
            .netmask = WIFI_GET_IPV4_ADDR(WIFI_NETMASK),
            .gw = WIFI_GET_IPV4_ADDR(WIFI_GATEWAY)
        };
        const struct wifi_manual_config manual_config = {
            .ipv4 = &ipv4Config,
            .ipv6 = NULL
        };
        int res = wifi_client_connect(&wifi_client, WIFI_SSID, NULL, WIFI_PASSWORD, NULL);
        printf("resss: %i\n", res);
    }

    wifi_client_wait_for_ip(&wifi_client);

}
