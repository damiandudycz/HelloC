#ifndef WIFI_HELPERS_H
#define WIFI_HELPERS_H

#include <esp_wifi.h>

#define WIFI_SSID_LENGTH 32
#define WIFI_BSSID_LENGTH 6
#define WIFI_PASSWORD_LENGTH 64

void nvs_initialize();
void wifi_ap_record_t_get_ssid(const wifi_ap_record_t *input, char ssid_output[WIFI_SSID_LENGTH]);
void wifi_ap_record_t_get_bssid(const wifi_ap_record_t *input, uint8_t bssid_output[WIFI_BSSID_LENGTH]);
void wifi_disable_logs();

#endif // WIFI_HELPERS_H