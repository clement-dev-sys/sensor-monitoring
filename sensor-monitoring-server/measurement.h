#pragma once

#include <stddef.h>

/*
 * Traitement d'un payload de mesure reçu par MQTT : parsing JSON puis
 * enregistrement en base.
 *
 * Le payload attendu, produit par le firmware :
 *
 *   {"boot":<u32>,"seq":<u32>,"ts":"YYYY-MM-DD HH:MM:SS","temp":<float>,"hum":<float>,"pres":<float>}
 *
 * Seuls temp/hum/pres sont obligatoires : "boot", "seq" et "ts" sont optionnels.
 *
 * Retourne 0 si la mesure a été enregistrée, 1 si c'était un doublon ignoré,
 * une valeur négative en cas d'erreur.
 */

int measurement_handle(const char *payload, size_t length);
