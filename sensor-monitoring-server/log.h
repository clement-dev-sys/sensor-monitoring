#pragma once

/*
 * Logger.
 *
 * Trois niveaux, et un "topic" par module (APP, DB, MQTT, JSON, ...).
 *
 * Sorties : log_info sur stdout et log_warning, log_error sur stderr.
 * log_error n'est jamais filtré : une erreur s'affiche toujours.
 * LEVEL_LOG (info_config.h) filtre les deux autres niveaux.
 */

/* À appeler une fois au démarrage, avant tout autre appel : fixe l'origine des
 * horodatages. Les valeurs affichées sont des millisecondes depuis cet appel. */
void log_init(void);

void log_info(const char *topic, const char *message);
void log_error(const char *topic, const char *message);
void log_warning(const char *topic, const char *message);