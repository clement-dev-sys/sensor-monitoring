#include <stdbool.h>

#include "esp_err.h"
#include "esp_log.h"
#include "mqtt_client.h"

#include "mqtt_pub.h"
#include "info_config.h"

static esp_mqtt_client_handle_t s_client = NULL;
static volatile bool s_connected = false;

static void mqtt_event_handler(void *args, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    (void)args;
    (void)event_base;
    (void)event_data;

    if (event_id == MQTT_EVENT_BEFORE_CONNECT) {
        ESP_LOGI("MQTT", "Connexion au broker...");
    }
    else if (event_id == MQTT_EVENT_CONNECTED) {
        s_connected = true;
        ESP_LOGI("MQTT", "Connecté au broker");
    }
    else if (event_id == MQTT_EVENT_DISCONNECTED) {
        s_connected = false;
        ESP_LOGW("MQTT", "Déconnecté du broker");
    }
    else if (event_id == MQTT_EVENT_ERROR) {
        s_connected = false;
        ESP_LOGE("MQTT", "Erreur MQTT");
    }
}

esp_err_t mqtt_pub_start(void)
{
    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER,
        .credentials.username = MQTT_USERNAME,
        .credentials.authentication.password = MQTT_PASS,
    };

    s_client = esp_mqtt_client_init(&mqtt_cfg);
    if (s_client == NULL) {
        ESP_LOGE("MQTT", "Échec esp_mqtt_client_init");
        return ESP_FAIL;
    }

    esp_err_t err = esp_mqtt_client_register_event(s_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    if (err != ESP_OK) {
        ESP_LOGE("MQTT", "Erreur esp_mqtt_client_register_event: %s", esp_err_to_name(err));
        return err;
    }

    err = esp_mqtt_client_start(s_client);
    if (err != ESP_OK) {
        ESP_LOGE("MQTT", "Erreur esp_mqtt_client_start: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI("MQTT", "Client MQTT démarré");
    return ESP_OK;
}

bool mqtt_pub_send(const char *payload, int length)
{
    if (s_client == NULL) {
        return false;
    }

    int msg_id = esp_mqtt_client_publish(s_client, MQTT_TOPIC_MEASUREMENTS, payload, length, 1, 0);
    if (msg_id == -1) {
        ESP_LOGE("MQTT", "Erreur de publication: %s", payload);
        return false;
    }
    if (msg_id == -2) {
        ESP_LOGE("MQTT", "Outbox pleine: %s", payload);
        return false;
    }

    if (!s_connected) {
        ESP_LOGW("MQTT", "Hors ligne : mesure mise en file");
    }

    return true;
}
