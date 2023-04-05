#include "WiFI/wifi_helpers.h"
#include <string.h>
#include <esp_wifi.h>
#include <nvs_flash.h>
#include <esp_log.h>

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------

// Extract SSID from wifi_ap_record_t in form of char[32]
void wifi_ap_record_t_get_ssid(const wifi_ap_record_t *input, char ssid_output[WIFI_SSID_LENGTH]) 
{
    // TODO: Make sure if ssid_output shoudn't be [33] with \0 at the end
    size_t length = sizeof(char) * WIFI_SSID_LENGTH;
    bzero(ssid_output, length);
    memcpy(ssid_output, input->ssid, length - sizeof(char));
}

void wifi_ap_record_t_get_bssid(const wifi_ap_record_t *input, uint8_t bssid_output[WIFI_BSSID_LENGTH]) 
{
    memcpy(bssid_output, input->bssid, WIFI_BSSID_LENGTH);
}

// Setup NVS Storage used by ESP WiFi.
// Without this creating WiFi stack would crash.
void nvs_initialize() 
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

void wifi_disable_logs() 
{
    char *tags_to_disable[] = {"wifi", "esp_netif_handlers", "phy_init"};
    int num_tags = sizeof(tags_to_disable) / sizeof(char *);
    for (int i = 0; i < num_tags; i++) {
        esp_log_level_set(tags_to_disable[i], ESP_LOG_NONE);
    }
}