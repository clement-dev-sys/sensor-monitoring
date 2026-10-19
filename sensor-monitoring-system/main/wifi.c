#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#include "info_config.h"
#include "wifi.h"

#define WIFI_CONNECTED_BIT  BIT0

static EventGroupHandle_t s_wifi_event_group = NULL;

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    (void)arg;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI("WIFI", "Connexion au Wi-Fi...");
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        ESP_LOGI("WIFI", "Wi-Fi associé, attente de l'IP...");
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);

        ESP_LOGW("WIFI", "Wi-Fi déconnecté, nouvelle tentative...");
        esp_err_t err = esp_wifi_connect();
        if (err != ESP_OK) {
            ESP_LOGE("WIFI", "Erreur esp_wifi_connect: %s", esp_err_to_name(err));
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        const ip_event_got_ip_t *event = (const ip_event_got_ip_t *)event_data;
        ESP_LOGI("WIFI", "Wi-Fi connecté, IP: " IPSTR, IP2STR(&event->ip_info.ip));

        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static esp_err_t init_nvs(void)
{
    esp_err_t err = nvs_flash_init();

    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW("WIFI", "NVS non effacer et non réinitialiser");

        err = nvs_flash_erase();
        if (err != ESP_OK) {
            return err;
        }
        err = nvs_flash_init();
    }

    return err;
}

static esp_err_t set_static_ip(esp_netif_t *netif)
{
    esp_netif_ip_info_t ip_info;

    esp_err_t err = esp_netif_str_to_ip4(ESP_STATIC_IP, &ip_info.ip);
    if (err != ESP_OK) {
        ESP_LOGE("WIFI", "Adresse IP statique invalide: %s", ESP_STATIC_IP);
        return err;
    }

    err = esp_netif_str_to_ip4(ESP_STATIC_GW, &ip_info.gw);
    if (err != ESP_OK) {
        ESP_LOGE("WIFI", "Passerelle invalide: %s", ESP_STATIC_GW);
        return err;
    }

    err = esp_netif_str_to_ip4(ESP_STATIC_NETMASK, &ip_info.netmask);
    if (err != ESP_OK) {
        ESP_LOGE("WIFI", "Masque invalide: %s", ESP_STATIC_NETMASK);
        return err;
    }

    err = esp_netif_dhcpc_stop(netif);
    if (err != ESP_OK) {
        ESP_LOGE("WIFI", "Erreur esp_netif_dhcpc_stop: %s", esp_err_to_name(err));
        return err;
    }

    return esp_netif_set_ip_info(netif, &ip_info);
}

esp_err_t wifi_start(void)
{
    s_wifi_event_group = xEventGroupCreate();
    if (s_wifi_event_group == NULL) {
        ESP_LOGE("WIFI", "Erreur xEventGroupCreate");
        return ESP_ERR_NO_MEM;
    }

    esp_err_t err = init_nvs();
    if (err != ESP_OK) {
        ESP_LOGE("WIFI", "Erreur nvs_flash_init: %s", esp_err_to_name(err));
        return err;
    }

    err = esp_netif_init();
    if (err != ESP_OK) {
        ESP_LOGE("WIFI", "Erreur esp_netif_init: %s", esp_err_to_name(err));
        return err;
    }

    err = esp_event_loop_create_default();
    if (err != ESP_OK) {
        ESP_LOGE("WIFI", "Erreur esp_event_loop_create_default: %s", esp_err_to_name(err));
        return err;
    }

    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    if (sta_netif == NULL) {
        ESP_LOGE("WIFI", "Erreur esp_netif_create_default_wifi_sta");
        return ESP_FAIL;
    }

    err = set_static_ip(sta_netif);
    if (err != ESP_OK) {
        return err;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&cfg);
    if (err != ESP_OK) {
        ESP_LOGE("WIFI", "Erreur esp_wifi_init: %s", esp_err_to_name(err));
        return err;
    }

    err = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);
    if (err != ESP_OK) {
        ESP_LOGE("WIFI", "Erreur handler WIFI_EVENT: %s", esp_err_to_name(err));
        return err;
    }

    err = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);
    if (err != ESP_OK) {
        ESP_LOGE("WIFI", "Erreur handler IP_EVENT: %s", esp_err_to_name(err));
        return err;
    }

    wifi_config_t wifi_config = {0};
    strlcpy((char *)wifi_config.sta.ssid, ESP_WIFI_SSID, sizeof(wifi_config.sta.ssid));
    strlcpy((char *)wifi_config.sta.password, ESP_WIFI_PASS, sizeof(wifi_config.sta.password));
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err != ESP_OK) {
        ESP_LOGE("WIFI", "Erreur esp_wifi_set_mode: %s", esp_err_to_name(err));
        return err;
    }

    err = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (err != ESP_OK) {
        ESP_LOGE("WIFI", "Erreur esp_wifi_set_config: %s", esp_err_to_name(err));
        return err;
    }

    err = esp_wifi_start();
    if (err != ESP_OK) {
        ESP_LOGE("WIFI", "Erreur esp_wifi_start: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI("WIFI", "Wi-Fi STA démarré");
    return ESP_OK;
}

void wifi_wait_connected(void)
{
    xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
}
