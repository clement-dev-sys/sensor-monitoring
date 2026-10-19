#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_system.h"

#include "led.h"
#include "mqtt_pub.h"
#include "sensor.h"
#include "time_sync.h"
#include "wifi.h"

#define MAX_SENSOR_FAILURES  5
#define SENSOR_RETRY_DELAY_MS  10000
#define PUBLISH_PERIOD_MS  4800
#define LED_PULSE_MS  200
#define STARTUP_DELAY_MS  5000
#define SNTP_TIMEOUT_MS  15000

#define PAYLOAD_SIZE  192

static bool build_payload(char *buffer, size_t size, uint32_t boot_id, uint32_t seq, const sensor_reading_t *reading)
{
    char measured_at[32];

    int len;
    if (time_sync_format_now(measured_at, sizeof(measured_at))) {
        len = snprintf(buffer, size,
                       "{\"boot\":%lu,\"seq\":%lu,\"ts\":\"%s\",\"temp\":%.2f,\"hum\":%.2f,\"pres\":%.2f}",
                       (unsigned long)boot_id, (unsigned long)seq, measured_at, (double)reading->temperature, (double)reading->humidity, (double)reading->pressure);
    } else {
        len = snprintf(buffer, size,
                       "{\"boot\":%lu,\"seq\":%lu,\"temp\":%.2f,\"hum\":%.2f,\"pres\":%.2f}",
                       (unsigned long)boot_id, (unsigned long)seq, (double)reading->temperature, (double)reading->humidity, (double)reading->pressure);
    }

    if (len < 0 || len >= (int)size) {
        ESP_LOGE("APP", "Erreur taille payload");
        return false;
    }

    return true;
}

void app_main(void)
{
    led_init();

    esp_err_t err = wifi_start();
    if (err != ESP_OK) {
        ESP_LOGE("APP", "Configuration Wi-Fi échouée: %s", esp_err_to_name(err));
        return;
    }
    wifi_wait_connected();

    time_sync_start();
    time_sync_wait(SNTP_TIMEOUT_MS);

    if (sensor_init() != ESP_OK) {
        ESP_LOGE("APP", "BME280 non configuré, nouvelles tentatives...");
    }

    if (mqtt_pub_start() != ESP_OK) {
        ESP_LOGE("APP", "Client MQTT non démarré");
    }

    vTaskDelay(pdMS_TO_TICKS(STARTUP_DELAY_MS));
    ESP_LOGI("APP", "Sensor Monitoring System");

    const uint32_t boot_id = esp_random();

    sensor_reading_t reading;
    uint32_t seq = 0;
    int failures = 0;

    while (1)
    {
        if (failures >= MAX_SENSOR_FAILURES) {
            ESP_LOGE("APP", "%d tentatives échouées, redémarrage...", failures);
            esp_restart();
        }

        if (!sensor_ready()) {
            failures++;
            ESP_LOGE("APP", "BME280 indisponible, reconfiguration... (essai %d)", failures);
            sensor_init();
            vTaskDelay(pdMS_TO_TICKS(SENSOR_RETRY_DELAY_MS));
            continue;
        }

        if (sensor_read(&reading) != ESP_OK) {
            failures++;
            ESP_LOGE("APP", "BME280 déconnecté, reconnexion... (essai %d)", failures);
            sensor_init();
            vTaskDelay(pdMS_TO_TICKS(SENSOR_RETRY_DELAY_MS));
            continue;
        }

        if (failures != 0) {
            ESP_LOGI("APP", "BME280 reconnecté, reprise...");
            failures = 0;
        }

        char payload[PAYLOAD_SIZE];
        if (!build_payload(payload, sizeof(payload), boot_id, seq, &reading)) {
            seq++;
            vTaskDelay(pdMS_TO_TICKS(PUBLISH_PERIOD_MS));
            continue;
        }

        if (mqtt_pub_send(payload, (int)strlen(payload))) {
            led_on();
            ESP_LOGI("APP", "Envoi: %s", payload);
            vTaskDelay(pdMS_TO_TICKS(LED_PULSE_MS));
            led_off();
        }

        seq++;
        vTaskDelay(pdMS_TO_TICKS(PUBLISH_PERIOD_MS));
    }
}
