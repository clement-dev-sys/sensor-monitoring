#pragma once

#include <stdbool.h>

#include "esp_err.h"

/*
 * Publication des mesures sur le broker MQTT, en QoS1.
 */

/* Crée le client, enregistre le handler d'événements et démarre la connexion. */
esp_err_t mqtt_pub_start(void);

/* Publie un payload en QoS1.
 *
 * Hors ligne, le message est conservé dans l'outbox du client et envoyé à la
 * reconnexion : c'est ce qui évite de perdre des mesures pendant une coupure.
 * Retourne false si la mise en file a échoué (outbox pleine) : la mesure est
 * alors réellement perdue. */
bool mqtt_pub_send(const char *payload, int length);