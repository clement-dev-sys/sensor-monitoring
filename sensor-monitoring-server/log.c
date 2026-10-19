#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <inttypes.h>

#include "log.h"
#include "info_config.h"

#define RESET  "\033[0m"
#define RED  "\033[31m"
#define GREEN  "\033[32m"
#define YELLOW  "\033[33m"

static uint64_t debut_ms;

static uint64_t millis()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
}

static uint64_t timestamp()
{
    return millis() - debut_ms;
}

void log_init(void)
{
    debut_ms = millis();
}

void log_info(const char *topic, const char *message)
{
    if (LEVEL_LOG) {
        fprintf(stdout, "%sI (%" PRIu64 ") %s: %s%s\n", GREEN, timestamp(), topic, message, RESET);
    }
}

void log_error(const char *topic, const char *message)
{
    fprintf(stderr, "%sE (%" PRIu64 ") %s: %s%s\n", RED, timestamp(), topic, message, RESET);
}

void log_warning(const char *topic, const char *message)
{
    if (LEVEL_LOG) {
        fprintf(stderr, "%sW (%" PRIu64 ") %s: %s%s\n", YELLOW, timestamp(), topic, message, RESET);
    }
}
