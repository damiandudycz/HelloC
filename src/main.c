#include <stdio.h>
#include <string.h>
#include <nvs_flash.h>
#include <esp_wifi.h>
#include "WiFi/wifi_client.h"
#include "WiFi/wifi_helpers.h"

#define WIFI_SSID "Network name"
#define WIFI_PASSWORD "password"
#define WIFI_SCAN_MAX_RESULTS 10

struct wifi_client *wifiClient;

// MARK: - Main application entry point.
void app_main() 
{
    // Allocate zero filled space for wifiClient.
    wifiClient = calloc(1, sizeof(wifiClient));

    // Initialize main devices and settings related to WiFi.
    nvs_initialize();
    wifi_disable_logs();
    wifi_client_setup(wifiClient);

    bool connection_restored = false;
    wifi_client_restore_connection(wifiClient, &connection_restored);

    if (connection_restored) {
        printf("+-- WiFi connection restored.\n");
    }
    else {
        struct wifi_manual_config manual_config = {
            .ipv4 = {
                .addr = "192.168.86.150",
                .gateway = "192.168.86.1",
                .netmask = "255.255.255.0"
            },
            .ipv6 = {
                .addr = {0},
                .prefix_len = {0}
            }
        };

        printf("+-- Connecting with default WiFi network.\n");
        wifi_client_connect(wifiClient, WIFI_SSID, NULL, WIFI_PASSWORD, &manual_config);
    }

    wifi_client_wait_for_ip(wifiClient);
    printf("+-- IP Address assigned.\n");

}
