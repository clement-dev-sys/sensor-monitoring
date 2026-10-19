#define _POSIX_C_SOURCE 200809L

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sqlite3.h>

#include "db.h"
#include "info_config.h"
#include "log.h"

static sqlite3 *s_db = NULL;
static sqlite3_stmt *s_insert_stmt = NULL;

static void build_database_path(char *out, size_t size)
{
    char exe[1024];
    ssize_t len = readlink("/proc/self/exe", exe, sizeof(exe) - 1);

    if (len <= 0 || len >= (ssize_t)(sizeof(exe) - 1)) {
        snprintf(out, size, "%s", DB_PATH);
        log_warning("DB", "Erreur construction chemin bdd, utilisation du chemin relatif.");
        return;
    }

    exe[len] = '\0';

    char *slash = strrchr(exe, '/');
    if (slash != NULL) {
        *slash = '\0';
    }

    size_t dir_len = strlen(exe);
    size_t rel_len = strlen(DB_PATH);

    if (dir_len + 1 + rel_len + 1 > size) {
        snprintf(out, size, "%s", DB_PATH);
        log_warning("DB", "Erreur construction chemin bdd, utilisation du chemin relatif.");
        return;
    }

    memcpy(out, exe, dir_len);
    out[dir_len] = '/';
    memcpy(out + dir_len + 1, DB_PATH, rel_len + 1);
}

static int exec_sql(const char *sql)
{
    char *err_msg = NULL;
    int rc = sqlite3_exec(s_db, sql, NULL, NULL, &err_msg);

    if (rc != SQLITE_OK) {
        char message[256];
        snprintf(message, sizeof(message), "Erreur SQL : %s\n  requête : %s\n", err_msg != NULL ? err_msg : "cause inconnue", sql);
        log_error("DB", message);
        sqlite3_free(err_msg);
    }

    return rc;
}

static int create_table(void)
{
    return exec_sql(
        "CREATE TABLE IF NOT EXISTS measures ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "measured_at TEXT,"
        "received_at TEXT NOT NULL,"
        "boot_id INTEGER,"
        "seq INTEGER,"
        "temperature REAL,"
        "humidite REAL,"
        "pression REAL"
        ");");
}

static int create_indexes(void)
{
    if (exec_sql("CREATE INDEX IF NOT EXISTS idx_measures_received_at "
                 "ON measures(received_at);") != SQLITE_OK) {
        return SQLITE_ERROR;
    }

    if (exec_sql("CREATE UNIQUE INDEX IF NOT EXISTS idx_measures_boot_seq "
                 "ON measures(boot_id, seq);") != SQLITE_OK) {
        return SQLITE_ERROR;
    }

    return SQLITE_OK;
}

int db_init(void)
{
    char db_path[1024];
    build_database_path(db_path, sizeof(db_path));

    struct stat info;
    bool new_db = (stat(db_path, &info) != 0);

    int rc = sqlite3_open(db_path, &s_db);
    if (rc != SQLITE_OK) {
        log_error("DB", "Erreur d'ouverture de la base : sqlite3_open.");
        return rc;
    }

    exec_sql("PRAGMA journal_mode=WAL;");
    exec_sql("PRAGMA synchronous=NORMAL;");
    exec_sql("PRAGMA temp_store=MEMORY;");
    exec_sql("PRAGMA busy_timeout=5000;");
    exec_sql("PRAGMA auto_vacuum=INCREMENTAL;");

    if (create_table() != SQLITE_OK) {
        return SQLITE_ERROR;
    }

    if (create_indexes() != SQLITE_OK) {
        return SQLITE_ERROR;
    }

    if (new_db) {
        if (exec_sql("VACUUM;") != SQLITE_OK) {
            return SQLITE_ERROR;
        }
    }

    static const char *insert_sql =
        "INSERT OR IGNORE INTO measures "
        "(measured_at, received_at, boot_id, seq, temperature, humidite, pression) "
        "VALUES (?, ?, ?, ?, ?, ?, ?);";

    rc = sqlite3_prepare_v2(s_db, insert_sql, -1, &s_insert_stmt, NULL);
    if (rc != SQLITE_OK) {
        log_error("DB", "Erreur de préparation du statement : sqlite3_prepare_v2.");
        return rc;
    }

    log_info("DB", "Base de données prête !");
    return SQLITE_OK;
}

int db_insert(const mesure_t *mesure)
{
    if (s_insert_stmt == NULL) {
        log_error("DB", "Statement non initialisé");
        return DB_INSERT_ERROR;
    }

    if (mesure->measured_at != NULL) {
        sqlite3_bind_text(s_insert_stmt, 1, mesure->measured_at, -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(s_insert_stmt, 1);
    }

    sqlite3_bind_text(s_insert_stmt, 2, mesure->received_at, -1, SQLITE_TRANSIENT);

    if (mesure->has_ids) {
        sqlite3_bind_int64(s_insert_stmt, 3, (sqlite3_int64)mesure->boot_id);
        sqlite3_bind_int64(s_insert_stmt, 4, (sqlite3_int64)mesure->seq);
    } else {
        sqlite3_bind_null(s_insert_stmt, 3);
        sqlite3_bind_null(s_insert_stmt, 4);
    }

    sqlite3_bind_double(s_insert_stmt, 5, mesure->temperature);
    sqlite3_bind_double(s_insert_stmt, 6, mesure->humidite);
    sqlite3_bind_double(s_insert_stmt, 7, mesure->pression);

    int rc = sqlite3_step(s_insert_stmt);
    int changed = sqlite3_changes(s_db);

    sqlite3_reset(s_insert_stmt);
    sqlite3_clear_bindings(s_insert_stmt);

    if (rc != SQLITE_DONE) {
        log_error("DB", "Erreur d'insertion : sqlite3_step.");
        return DB_INSERT_ERROR;
    }

    return (changed > 0) ? DB_INSERTED : DB_DUPLICATE;
}

void db_close(void)
{
    if (s_insert_stmt != NULL) {
        sqlite3_finalize(s_insert_stmt);
        s_insert_stmt = NULL;
    }

    if (s_db != NULL) {
        sqlite3_close(s_db);
        s_db = NULL;
    }
}
