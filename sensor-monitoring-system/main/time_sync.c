#include <stdbool.h>
#include <stddef.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_netif_sntp.h"

#include "info_config.h"
#include "time_sync.h"

#define TIME_FORMAT  "%Y-%m-%d %H:%M:%S"
#define POLL_STEP_MS  100

static volatile bool s_synced = false;

static void sntp_sync_callback(struct timeval *tv)
{
    (void)tv;
    s_synced = true;
}

void time_sync_start(void)
{
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(ESP_SNTP_SERVER);
    config.sync_cb = sntp_sync_callback;

    esp_err_t err = esp_netif_sntp_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE("TIME_SYNC", "Erreur esp_netif_sntp_init: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI("TIME_SYNC", "SNTP démarré (%s)", ESP_SNTP_SERVER);
}

bool time_sync_wait(unsigned timeout_ms)
{
    for (unsigned waited = 0; !s_synced && waited < timeout_ms; waited += POLL_STEP_MS) {
        vTaskDelay(pdMS_TO_TICKS(POLL_STEP_MS));
    }

    if (s_synced) {
        ESP_LOGI("TIME_SYNC", "Horloge synchronisée");
    } else {
        ESP_LOGW("TIME_SYNC", "Pas de synchronisation SNTP dans le délai : envoi sans horodatage");
    }

    return s_synced;
}

bool time_sync_format_now(char *buffer, size_t size)
{
    if (!s_synced) {
        return false;
    }

    time_t now = time(NULL);
    struct tm utc;
    gmtime_r(&now, &utc);
    strftime(buffer, size, TIME_FORMAT, &utc);

    return true;
}
