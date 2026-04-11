# YearReporter

API на C++/Drogon для расчёта прогресса ученика по курсу.

## Запуск в контейнере

1. Убедиться, что запущен Docker.
2. В корне проекта выполнить:

```bash
docker compose up --build -d
```

После старта будут доступны:
- PostgreSQL: `localhost:5555`
- Backend API: `http://localhost:8081`

Проверить состояние контейнеров:

```bash
docker compose ps
```

Остановить всё:

```bash
docker compose down
```

## Настройка

Для запуска в Docker используется файл [`config.docker.json`](/Users/mikhaiil/CLionProjects/MIPT/cifrium/YearReporter/config.docker.json).

Основные параметры:
- `listeners[0].port` — внутренний порт API в контейнере, сейчас `8080`
- `db_clients[0].host` — хост базы внутри docker-сети, сейчас `postgres`
- `db_clients[0].port` — порт базы внутри docker-сети, сейчас `5432`
- `db_clients[0].dbname` — имя базы, сейчас `xakaton`
- `db_clients[0].user` — пользователь БД
- `db_clients[0].passwd` — пароль БД

Внешние порты задаются в [`docker-compose.yaml`](/Users/mikhaiil/CLionProjects/MIPT/cifrium/YearReporter/docker-compose.yaml):
- `8081:8080` — API доступно с хоста на `8081`
- `5555:5432` — PostgreSQL доступен с хоста на `5555`

Для локального запуска без Docker используется отдельный файл [`config.json`](/Users/mikhaiil/CLionProjects/MIPT/cifrium/YearReporter/config.json).

## Доступные эндпоинты

### 1. Прогресс по четверти

`GET /api/v1/progress/quarter?quarter=<1..4>&user_id=<user_id>&course_id=<course_id>`

Пример:

```bash
curl -s "http://localhost:8081/api/v1/progress/quarter?quarter=1&user_id=665767&course_id=755"
```

Успешный ответ:

```json
{
  "quarter": 1,
  "progress": 88,
  "user_id": 665767,
  "course_id": 755
}
```

Ошибка:

```json
{
  "reason": "Не указан обязательный параметр 'quarter'"
}
```

### 2. Прогресс по всему курсу

`GET /api/v1/progress/course?user_id=<user_id>&course_id=<course_id>`

Пример:

```bash
curl -s "http://localhost:8081/api/v1/progress/course?user_id=665767&course_id=755"
```

Успешный ответ:

```json
{
  "progress": 71,
  "user_id": 665767,
  "course_id": 755
}
```

Ошибка:

```json
{
  "reason": "Курс или пользователь не найдены в базе"
}
```

## Быстрые примеры проверки

```bash
curl -s "http://localhost:8081/api/v1/progress/quarter?quarter=1&user_id=665767&course_id=755"
curl -s "http://localhost:8081/api/v1/progress/quarter?quarter=2&user_id=665767&course_id=755"
curl -s "http://localhost:8081/api/v1/progress/course?user_id=665767&course_id=755"
```
