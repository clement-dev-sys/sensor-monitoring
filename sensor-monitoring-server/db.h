#pragma once

#include <stdbool.h>

/*
 * Persistance des mesures dans une base SQLite.
 *
 * Le fichier de base est résolu au démarrage relativement au répertoire du
 * binaire (DB_PATH de info_config.h, via build_database_path()), donc le
 * serveur fonctionne quel que soit le répertoire d'où il est lancé.
 */

#define DB_INSERTED  1
#define DB_DUPLICATE  0
#define DB_INSERT_ERROR  (-1)

typedef struct {
    /* Horodatage de la mesure, côté capteur, UTC "YYYY-MM-DD HH:MM:SS". */
    const char *measured_at;

    /* Horodatage de la réception par le serveur, UTC, même format. */
    const char *received_at;

    /* Clé d'idempotence (boot_id, seq), fournie par le capteur.
     * has_ids = false quand le payload ne les contient pas */
    bool has_ids;
    long boot_id;
    long seq;

    /* Grandeurs physiques : °C, %, hPa. */
    double temperature;
    double humidite;
    double pression;
} mesure_t;

/* Ouvre la base, applique les PRAGMA, crée le schéma et prépare l'insertion.
 * Retourne 0 (SQLITE_OK) en succès, un code erreur SQLite sinon. */
int db_init(void);

/* Enregistre une mesure.
 * Retourne DB_INSERTED, DB_DUPLICATE (si même clé d'idempotence) ou DB_INSERT_ERROR. */
int db_insert(const mesure_t *mesure);

/* Libère le statement et ferme la base. Sans effet si déjà fermée. */
void db_close(void);
