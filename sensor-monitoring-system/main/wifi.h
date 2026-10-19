#pragma once

#include "esp_err.h"

/*
 * Lien Wi-Fi en mode station, avec adressage statique (voir info_config.h).
 */

/* Démarre la pile réseau puis lance la connexion. */
esp_err_t wifi_start(void);

/* Bloque jusqu'à l'obtention d'une IP valide. */
void wifi_wait_connected(void);