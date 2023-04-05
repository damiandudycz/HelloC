#ifndef WIFI_CLIENT_H
#define WIFI_CLIENT_H

#include "wifi_connectionStatus.h"
#include "wifi_helpers.h"
#include <esp_wifi.h>

struct wifi_client {
    enum wifi_connectionStatus status;
    esp_netif_t *sta_interface;
};

struct ipv4_config {
    char addr[16];
    char netmask[16];
    char gateway[16];
};

struct ipv6_config {
    char addr[40];
    char prefix_len[3];
};

// TODO: Przerobić typy na proste, uzywane przez ESP-IDF bezposrednio i dodac funkcje pomocnicze generujace to z char.
struct wifi_manual_config {
    struct ipv4_config ipv4;
    struct ipv6_config ipv6;
};

void wifi_client_setup(struct wifi_client *client);
void wifi_client_scan(const struct wifi_client *client, uint16_t max_results, wifi_ap_record_t results[], uint16_t *results_count);
void wifi_client_restore_connection(struct wifi_client *client, bool *restored);
void wifi_client_connect(struct wifi_client *client, const char ssid[], const uint8_t bssid[], const char password[], const struct wifi_manual_config *manual_config);
void wifi_client_wait_for_ip(struct wifi_client *client);
void wifi_client_disconnect(struct wifi_client *client);

#endif // WIFI_CLIENT_H
