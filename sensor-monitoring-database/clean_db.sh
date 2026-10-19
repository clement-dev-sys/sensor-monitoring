#!/bin/bash

# Purge des mesures plus vieilles que la rétention, par paquets, 
# puis récupération de l'espace. À lancer périodiquement (voir README).

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DB_FILE="$SCRIPT_DIR/db_esp32.db"
LOG_FILE="$SCRIPT_DIR/clean_db.log"

RETENTION='-3 hours'
BATCH_SIZE=2000

log() {
  echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" | tee -a "$LOG_FILE"
}

if [ ! -f "$DB_FILE" ]; then
  log "ERREUR : Base de données introuvable: $DB_FILE"
  exit 1
fi

TOTAL_DELETED=0

while true; do
  if ! DELETED=$(sqlite3 -cmd ".timeout 5000" "$DB_FILE" "
    DELETE FROM measures
    WHERE rowid IN (
      SELECT rowid FROM measures
      WHERE received_at < datetime('now', '$RETENTION')
      ORDER BY received_at
      LIMIT $BATCH_SIZE
    );
    SELECT changes();
  " 2>&1); then
    log "ERREUR : la purge sqlite3 a échoué : $DELETED"
    exit 1
  fi

  if ! [[ "$DELETED" =~ ^[0-9]+$ ]]; then
    log "ERREUR : réponse inattendue de sqlite3 : '$DELETED'"
    exit 1
  fi

  [ "$DELETED" -eq 0 ] && break

  TOTAL_DELETED=$((TOTAL_DELETED + DELETED))
done

if [ "$TOTAL_DELETED" -gt 0 ]; then
  log "Nettoyage : $TOTAL_DELETED lignes supprimées (rétention $RETENTION)"
  sqlite3 -cmd ".timeout 5000" "$DB_FILE" "PRAGMA incremental_vacuum(200);" >/dev/null
fi
