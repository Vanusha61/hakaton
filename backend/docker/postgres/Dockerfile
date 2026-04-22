FROM postgres:18

COPY dump-xakaton-202603310120.sql /docker-entrypoint-initdb.d/dump-xakaton-202603310120.sql
COPY docker/postgres/init/01_restore_dump.sh /docker-entrypoint-initdb.d/01_restore_dump.sh

RUN chmod 644 /docker-entrypoint-initdb.d/dump-xakaton-202603310120.sql \
    && chmod 755 /docker-entrypoint-initdb.d/01_restore_dump.sh
