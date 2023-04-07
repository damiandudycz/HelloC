#ifndef WIFI_HELPERS_H
#define WIFI_HELPERS_H

#include <esp_wifi.h>

#define WIFI_SSID_LENGTH 32
#define WIFI_BSSID_LENGTH 6
#define WIFI_PASSWORD_LENGTH 64

// Results from calling various WiFi functions.
enum wifi_result_t
{
    WIFI_OK,
    WIFI_REQUIRED_PARAMETER_IS_NULL,
    WIFI_WRONG_CLIENT_STATUS,
    WIFI_ESP_ERROR,
    WIFI_CONNECTION_FAILED
};

void nvs_initialize();
void wifi_disable_logs();
uint32_t wifi_get_ipv4_addr(const char *addr_string);
#define WIFI_GET_IPV4_ADDR(address) { wifi_get_ipv4_addr(address) }

#endif // WIFI_HELPERS_H
