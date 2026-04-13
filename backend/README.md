# YearReporter

API на C++/Drogon для расчёта учебных метрик ученика по курсу.

## Запуск в контейнере

```bash
docker compose up --build -d
```

После старта будут доступны:
- PostgreSQL: `localhost:5555`
- Backend API: `http://localhost:8081`

Проверка состояния:

```bash
docker compose ps
```

Остановка:

```bash
docker compose down
```

## Настройка

Для Docker используется [`config.docker.json`](/Users/mikhaiil/CLionProjects/MIPT/cifrium/hakaton/backend/config.docker.json).
Для локального запуска используется [`config.json`](/Users/mikhaiil/CLionProjects/MIPT/cifrium/hakaton/backend/config.json).

Основные поля конфигурации:
- `listeners[0].port` — порт HTTP-сервера
- `db_clients[0].host` — хост Postgres
- `db_clients[0].port` — порт Postgres
- `db_clients[0].dbname` — имя базы
- `db_clients[0].user` — пользователь базы
- `db_clients[0].passwd` — пароль базы

В [`docker-compose.yaml`](/Users/mikhaiil/CLionProjects/MIPT/cifrium/hakaton/backend/docker-compose.yaml) наружу опубликованы:
- `8081:8080` для API
- `5555:5432` для PostgreSQL

## Эндпоинты

### Прогресс

`GET /api/v1/progress/quarter?quarter=<1..4>&user_id=<user_id>&course_id=<course_id>`

```json
{
  "quarter": 1,
  "progress": 88,
  "user_id": 665767,
  "course_id": 755
}
```

`GET /api/v1/progress/course?user_id=<user_id>&course_id=<course_id>`

```json
{
  "progress": 71,
  "user_id": 665767,
  "course_id": 755
}
```

### Видео

`GET /api/v1/videos/quarter?quarter=<1..4>&user_id=<user_id>&course_id=<course_id>`

```json
{
  "course_id": 755,
  "user_id": 665767,
  "quarter": 1,
  "watched_count": 1,
  "total_count": 1
}
```

`GET /api/v1/videos/year?user_id=<user_id>&course_id=<course_id>`

```json
{
  "course_id": 755,
  "user_id": 665767,
  "watched_count": 24,
  "total_count": 35
}
```

### Баллы

`GET /api/v1/points/quarter?quarter=<1..4>&user_id=<user_id>&course_id=<course_id>`

```json
{
  "course_id": 755,
  "user_id": 665767,
  "quarter": 1,
  "earned_count": 33,
  "total_count": 36
}
```

`GET /api/v1/points/year?user_id=<user_id>&course_id=<course_id>`

```json
{
  "course_id": 755,
  "user_id": 665767,
  "earned_count": 51,
  "total_count": 56
}
```

### Конспекты и материалы

`GET /api/v1/conspects/quarter?quarter=<1..4>&user_id=<user_id>&course_id=<course_id>`

```json
{
  "course_id": 755,
  "user_id": 665767,
  "quarter": 1,
  "learned_count": 4,
  "total_count": 14
}
```

`GET /api/v1/conspects/year?user_id=<user_id>&course_id=<course_id>`

```json
{
  "course_id": 755,
  "user_id": 665767,
  "learned_count": 4,
  "total_count": 36
}
```

### Ошибка

Все endpoint’ы при ошибке возвращают JSON вида:

```json
{
  "reason": "Не указан обязательный параметр 'quarter'"
}
```

## Быстрые примеры

```bash
curl -s "http://localhost:8081/api/v1/progress/course?user_id=665767&course_id=755"
curl -s "http://localhost:8081/api/v1/videos/year?user_id=665767&course_id=755"
curl -s "http://localhost:8081/api/v1/points/year?user_id=665767&course_id=755"
curl -s "http://localhost:8081/api/v1/conspects/year?user_id=665767&course_id=755"
```
