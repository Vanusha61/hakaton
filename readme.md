# Backend Docker Quick Start

1. Перейдите в папку бэкенда:
```bash
cd /Users/mikhaiil/CLionProjects/hakaton/backend
```

2. Создайте `.env` из примера и при необходимости отредактируйте настройки PostgreSQL и порты:
```bash
cp env.example .env
```

3. Соберите и поднимите сервисы:
```bash
docker compose up -d --build
```

4. Бэкенд будет доступен на порту из `BACKEND_PORT`, PostgreSQL на порту из `POSTGRES_PORT`.

Важно: dump `dump-xakaton-202603310120.sql` автоматически восстанавливается только при первом запуске на пустом Docker volume. Если нужно заново развернуть БД из dump, выполните:
```bash
docker compose down -v
docker compose up -d --build
```
