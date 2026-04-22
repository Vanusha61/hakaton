#!/bin/sh
set -e

DUMP_FILE="/docker-entrypoint-initdb.d/dump-xakaton-202603310120.sql"

if [ ! -f "$DUMP_FILE" ]; then
  echo "Dump file not found: $DUMP_FILE"
  exit 1
fi

echo "Restoring PostgreSQL dump from $DUMP_FILE into database $POSTGRES_DB"
pg_restore \
  --username="$POSTGRES_USER" \
  --dbname="$POSTGRES_DB" \
  --no-owner \
  --no-privileges \
  "$DUMP_FILE"

echo "PostgreSQL dump restore finished"
