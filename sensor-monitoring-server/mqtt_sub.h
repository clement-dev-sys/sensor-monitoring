#pragma once

/*
 * Abonnement MQTT : reçoit les mesures et les remet à la couche applicative.
 *
 * Le client se connecte avec une session persistante et un identifiant stable 
 * (MQTT_CLIENT_ID). Le broker conserve alors les mesures publiées pendant que 
 * ce serveur est arrêté et les livre à la reconnexion.
 * Changer l'identifiant revient à repartir d'une session neuve : la file en
 * attente est perdue.
 */

/* Se connecte, s'abonne au topic et enregistre les callbacks.
 * Retourne 0 en succès, -1 sinon. En cas d'échec, le client est déjà libéré. */
int mqtt_sub_start(void);

/* Se déconnecte et libère le client. Sans effet s'il n'était pas démarré. */
void mqtt_sub_stop(void);