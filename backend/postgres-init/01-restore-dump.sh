#!/bin/sh
set -eu

DUMP_FILE="/backups/dump-xakaton-202603310120.sql"

if [ ! -f "$DUMP_FILE" ]; then
  echo "Dump file not found: $DUMP_FILE" >&2
  exit 1
fi

echo "Restoring database from $DUMP_FILE"
pg_restore \
  --verbose \
  --clean \
  --create \
  --if-exists \
  --no-owner \
  --no-privileges \
  --username="$POSTGRES_USER" \
  --dbname=postgres \
  "$DUMP_FILE"
