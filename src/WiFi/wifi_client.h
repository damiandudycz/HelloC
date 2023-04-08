#ifndef WIFI_CLIENT_H
#define WIFI_CLIENT_H

#include "wifi_connectionStatus.h"
#include "wifi_helpers.h"
#include <esp_wifi.h>

struct wifi_client_config_t
{
    bool setup_nvs;
    bool enable_logs;
};

struct wifi_client_t 
{
    struct wifi_client_config_t config;
    enum wifi_connectionStatus status;
    esp_netif_t *sta_interface;
};

#define WIFI_CLIENT(config_value) { \
    .config = config_value,         \
    .status = NOT_INITIALIZED,      \
    .sta_interface = NULL           \
};

struct wifi_manual_config 
{
    const esp_netif_ip_info_t *ipv4;
    const esp_netif_ip6_info_t *ipv6;
};

enum wifi_result_t wifi_client_setup(struct wifi_client_t *client);
enum wifi_result_t wifi_client_scan(const struct wifi_client_t *client, uint16_t max_results, wifi_ap_record_t *results, uint16_t *results_count);
enum wifi_result_t wifi_client_restore_connection(struct wifi_client_t *client, bool *restored);
enum wifi_result_t wifi_client_connect(struct wifi_client_t *client, const char *ssid, const char *password, const struct wifi_manual_config *manual_config);
enum wifi_result_t wifi_client_wait_for_ip(struct wifi_client_t *client);
enum wifi_result_t wifi_client_disconnect(struct wifi_client_t *client);

#endif // WIFI_CLIENT_H
