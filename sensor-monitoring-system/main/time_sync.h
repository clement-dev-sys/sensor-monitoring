#pragma once

#include <stdbool.h>
#include <stddef.h>

/*
 * Horloge système, synchronisée par SNTP.
 *
 * Sans synchronisation, le capteur ne peut pas dater ses mesures : les envois
 * partent alors sans champ d'horodatage et le serveur enregistre NULL.
 */

/* Démarre le service SNTP. Non bloquant : la synchronisation se fait en tâche de
 * fond, et l'état interne se met à jour quand elle aboutit. */
void time_sync_start(void);

/* Attend la première synchronisation, avec un maximum de temps de SNTP_TIMEOUT_MS 
* (voir sensor-monitoring-system.c). */
bool time_sync_wait(unsigned timeout_ms);

/* Écrit l'heure UTC au format "YYYY-MM-DD HH:MM:SS". 
* Retourne false si l'horloge n'est pas prête. */
bool time_sync_format_now(char *buffer, size_t size);