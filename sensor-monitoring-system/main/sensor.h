#pragma once

#include <stdbool.h>

#include "esp_err.h"

/*
 * Capteur BME280 sur bus I2C.
 */

typedef struct {
    float temperature;  /* °C   */
    float humidity;  /* %  */
    float pressure;  /* hPa  */
} sensor_reading_t;

/* Configuration du bus I2C et du capteur. Libère au préalable toute
 * configuration précédente. */
esp_err_t sensor_init(void);

/* Vrai si le capteur est configuré et utilisable. */
bool sensor_ready(void);

/* Lit les trois grandeurs. La première erreur rencontrée invalide la lecture */
esp_err_t sensor_read(sensor_reading_t *out);