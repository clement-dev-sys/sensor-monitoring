#define _POSIX_C_SOURCE 200809L

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "db.h"
#include "mqtt_sub.h"
#include "log.h"

static volatile sig_atomic_t s_running = 1;

static void handle_signal(int signum)
{
    (void)signum;
    s_running = 0;
}

static void install_signal_handlers(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

int main(void)
{
    log_init();
    log_info("APP", "=== Serveur de collecte ===");

    install_signal_handlers();

    if (db_init() != 0) {
        db_close();
        return EXIT_FAILURE;
    }

    if (mqtt_sub_start() != 0) {
        db_close();
        return EXIT_FAILURE;
    }

    log_info("APP", "En attente des mesures...");

    while (s_running) {
        sleep(1);
    }

    log_info("APP", "Arrêt demandé, fermeture propre...");
    mqtt_sub_stop();
    db_close();
    log_info("APP", "Arrêt terminé.");

    return EXIT_SUCCESS;
}
