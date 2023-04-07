#include "WiFI/wifi_helpers.h"
#include <string.h>
#include <esp_wifi.h>
#include <nvs_flash.h>
#include <esp_log.h>

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
    const char *tags_to_disable[] = {"wifi", "esp_netif_handlers", "phy_init"};
    const int num_tags = sizeof(tags_to_disable) / sizeof(char *);
    for (int i = 0; i < num_tags; ++i) {
        esp_log_level_set(tags_to_disable[i], ESP_LOG_NONE);
    }
}

uint32_t wifi_get_ipv4_addr(const char *addr_string) 
{
    if (!addr_string) { return 0; };
    return esp_ip4addr_aton(addr_string);
}
