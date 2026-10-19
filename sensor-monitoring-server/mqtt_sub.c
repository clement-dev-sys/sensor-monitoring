#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <MQTTClient.h>

#include "measurement.h"
#include "mqtt_sub.h"
#include "info_config.h"
#include "log.h"

#define RECONNECT_ATTEMPTS  5
#define RECONNECT_DELAY_S  10

static MQTTClient_connectOptions s_connect_opts = MQTTClient_connectOptions_initializer;
static MQTTClient s_client = NULL;

static void connection_lost(void *context, char *cause)
{
    (void)context;

    char message[256];
    snprintf(message, sizeof(message), "Connexion MQTT perdue : %s", cause != NULL ? cause : "raison inconnue");
    log_error("MQTT", message);

    for (int tentative = 1; tentative <= RECONNECT_ATTEMPTS; tentative++) {
        log_warning("MQTT", "Tentative de reconnexion...");

        if (MQTTClient_connect(s_client, &s_connect_opts) == MQTTCLIENT_SUCCESS) {
            if (s_connect_opts.returned.sessionPresent) {
                log_info("MQTT", "Reconnecté, abonnement toujours actif");
            } else {
                log_error("MQTT", "Reconnecté mais session non reprise : abonnement perdu");
            }
            return;
        }

        if (tentative < RECONNECT_ATTEMPTS) {
            sleep(RECONNECT_DELAY_S);
        }
    }

    snprintf(message, sizeof(message), "Reconnexion abandonnée après %d tentatives : collecte interrompue", RECONNECT_ATTEMPTS);
    log_error("MQTT", message);
}

static int message_arrived(void *context, char *topic_name, int topic_len, MQTTClient_message *message)
{
    (void)context;
    (void)topic_len;

    if (message->payloadlen > 0) {
        measurement_handle((const char *)message->payload, (size_t)message->payloadlen);
    } else {
        log_error("MQTT", "Message MQTT vide, ignoré");
    }

    MQTTClient_freeMessage(&message);
    MQTTClient_free(topic_name);

    return 1;
}

int mqtt_sub_start(void)
{
    int rc = MQTTClient_create(&s_client, MQTT_BROKER, MQTT_CLIENT_ID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    if (rc != MQTTCLIENT_SUCCESS) {
        log_error("MQTT", "Échec MQTTClient_create");
        return -1;
    }

    rc = MQTTClient_setCallbacks(s_client, NULL, connection_lost, message_arrived, NULL);
    if (rc != MQTTCLIENT_SUCCESS) {
        log_error("MQTT", "Échec MQTTClient_setCallbacks");
        MQTTClient_destroy(&s_client);
        return -1;
    }

    s_connect_opts.username = MQTT_USERNAME;
    s_connect_opts.password = MQTT_PASS;
    s_connect_opts.keepAliveInterval = 60;
    s_connect_opts.cleansession = 0;
    s_connect_opts.MQTTVersion = MQTTVERSION_3_1_1;

    rc = MQTTClient_connect(s_client, &s_connect_opts);
    if (rc != MQTTCLIENT_SUCCESS) {
        log_error("MQTT", "Échec de connexion au broker");
        MQTTClient_destroy(&s_client);
        return -1;
    }

    rc = MQTTClient_subscribe(s_client, MQTT_TOPIC_MEASUREMENTS, 1);
    if (rc != MQTTCLIENT_SUCCESS) {
        log_error("MQTT", "Échec d'abonnement au topic");
        MQTTClient_disconnect(s_client, 10000);
        MQTTClient_destroy(&s_client);
        return -1;
    }

    char message[128];
    snprintf(message, sizeof(message), "Abonné à %s", MQTT_TOPIC_MEASUREMENTS);
    log_info("MQTT", message);
    return 0;
}

void mqtt_sub_stop(void)
{
    if (s_client != NULL) {
        MQTTClient_disconnect(s_client, 10000);
        MQTTClient_destroy(&s_client);
        s_client = NULL;
    }
}
