#define _POSIX_C_SOURCE 200809L

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <json-c/json.h>

#include "db.h"
#include "measurement.h"
#include "log.h"

static void format_utc_now(char *buffer, size_t size)
{
    time_t now = time(NULL);
    struct tm utc;
    gmtime_r(&now, &utc);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", &utc);
}

static bool get_measured_at(struct json_object *json, const char *key, const char **out)
{
    struct json_object *value = NULL;

    if (!json_object_object_get_ex(json, key, &value) || value == NULL) {
        return false;
    }
    if (json_object_get_type(value) != json_type_string) {
        return false;
    }

    *out = json_object_get_string(value);
    return true;
}

static bool get_info_data(struct json_object *json, const char *key, long *out)
{
    struct json_object *value = NULL;

    if (!json_object_object_get_ex(json, key, &value) || value == NULL) {
        return false;
    }
    if (json_object_get_type(value) != json_type_int) {
        return false;
    }

    *out = (long)json_object_get_int64(value);
    return true;
}

int measurement_handle(const char *payload, size_t length)
{
    struct json_tokener *tokener = json_tokener_new();
    if (tokener == NULL) {
        log_error("JSON", "Erreur d'allocation du tokener JSON");
        return -1;
    }

    struct json_object *json = json_tokener_parse_ex(tokener, payload, (int)length);
    enum json_tokener_error json_err = json_tokener_get_error(tokener);
    json_tokener_free(tokener);

    if (json_err != json_tokener_success || json == NULL) {
        char message[256];
        snprintf(message, sizeof(message), "Payload JSON invalide : %s", json_tokener_error_desc(json_err));
        log_error("JSON", message);
        if (json != NULL) {
            json_object_put(json);
        }
        return -1;
    }

    struct json_object *temp_obj = NULL;
    struct json_object *hum_obj = NULL;
    struct json_object *pres_obj = NULL;

    if (!json_object_object_get_ex(json, "temp", &temp_obj) || !json_object_object_get_ex(json, "hum", &hum_obj) || !json_object_object_get_ex(json, "pres", &pres_obj)) {
        log_error("JSON", "Payload incomplet : temp/hum/pres attendus");
        json_object_put(json);
        return -1;
    }

    char received_at[32];
    format_utc_now(received_at, sizeof(received_at));

    mesure_t mesure = {
        .received_at = received_at,
        .measured_at = NULL,
        .has_ids = false,
        .boot_id = 0,
        .seq = 0,
        .temperature = json_object_get_double(temp_obj),
        .humidite = json_object_get_double(hum_obj),
        .pression = json_object_get_double(pres_obj),
    };

    get_measured_at(json, "ts", &mesure.measured_at);

    long boot_id = 0;
    long seq = 0;
    if (get_info_data(json, "boot", &boot_id) && get_info_data(json, "seq", &seq)) {
        mesure.has_ids = true;
        mesure.boot_id = boot_id;
        mesure.seq = seq;
    }

    char buffer[256];

    int len = snprintf(buffer, sizeof(buffer),
        "Enregistrement : temp=%.2f, hum=%.2f, press=%.2f | recu=%s",
        mesure.temperature, mesure.humidite, mesure.pression, mesure.received_at
    );

    if (len < 0) {
        buffer[0] = '\0';
        len = 0;
    }

    if (mesure.measured_at != NULL && len < (int)sizeof(buffer) - 1) {
        len += snprintf(buffer + len, sizeof(buffer) - len, " | envoie=%s", mesure.measured_at);
    }

    if (mesure.has_ids && len < (int)sizeof(buffer) - 1) {
        len += snprintf(buffer + len, sizeof(buffer) - len, " | boot=%ld, seq=%ld", mesure.boot_id, mesure.seq);
    }

    log_info("JSON", buffer);

    int result = db_insert(&mesure);
    json_object_put(json);

    if (result == DB_INSERTED) {
        log_info("DB", "Enregistrement terminé");
        return 0;
    }
    if (result == DB_DUPLICATE) {
        log_warning("DB", "Doublon ignoré (même boot/seq)");
        return 1;
    }

    return -1;
}
