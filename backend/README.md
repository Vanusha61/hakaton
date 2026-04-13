# YearReporter Backend

Backend на C++/Drogon, который отдаёт frontend метрики по прохождению курса учеником.

Сейчас API умеет отдавать:
- процент прогресса по четверти
- процент прогресса по всему курсу
- количество просмотренных видео
- количество набранных баллов
- количество изученных конспектов и материалов
- статистику по типам медиа-контента

## Запуск

### В Docker

```bash
docker compose up --build -d
```

После старта будут доступны:
- PostgreSQL: `localhost:5555`
- API: `http://localhost:8081`

Проверить состояние:

```bash
docker compose ps
```

Остановить:

```bash
docker compose down
```

### Локально без Docker

```bash
cmake -S . -B build
cmake --build build
./build/YearReporter config.json
```

## Настройка

### Конфиги

- [`config.json`](/Users/mikhaiil/CLionProjects/MIPT/cifrium/hakaton/backend/config.json) — локальный запуск
- [`config.docker.json`](/Users/mikhaiil/CLionProjects/MIPT/cifrium/hakaton/backend/config.docker.json) — запуск в контейнере

### Основные поля

- `listeners[0].port` — порт HTTP API
- `db_clients[0].host` — хост Postgres
- `db_clients[0].port` — порт Postgres
- `db_clients[0].dbname` — имя базы
- `db_clients[0].user` — пользователь БД
- `db_clients[0].passwd` — пароль БД

### Внешние порты в Docker

В [`docker-compose.yaml`](/Users/mikhaiil/CLionProjects/MIPT/cifrium/hakaton/backend/docker-compose.yaml):
- `8081:8080` — API
- `5555:5432` — PostgreSQL

## Общие правила API

### Формат запросов

Все endpoint’ы сейчас работают как `GET` с query-параметрами.

Обязательные параметры зависят от endpoint’а:
- `user_id`
- `course_id`
- `quarter` — только для quarter-endpoint’ов, значение `1..4`

### Формат ошибок

Если запрос невалидный или данных недостаточно, API возвращает JSON:

```json
{
  "reason": "Не указан обязательный параметр 'quarter'"
}
```

### Что важно frontend-разработчику

- Во всех ответах `user_id` и `course_id` возвращаются обратно для удобства связывания данных на клиенте.
- Quarter-endpoint’ы всегда относятся к одной из 4 четвертей и содержат поле `quarter` либо `quarted_id` в media-статистике.
- Все процентные поля в ответах уже округлены до целых значений.
- В `media` сумма трёх процентов всегда равна `100`, если `total_watched_videos_cnt > 0`.
- Если данных нет, backend не придумывает нули молча, а возвращает ошибку с `reason`.

## Эндпоинты

### 1. Прогресс

#### Прогресс по четверти

`GET /api/v1/progress/quarter?quarter=<1..4>&user_id=<user_id>&course_id=<course_id>`

Что отдаёт:
- `progress` — итоговый процент прогресса за четверть от `0` до `100`

Пример:

```bash
curl -s "http://localhost:8081/api/v1/progress/quarter?quarter=1&user_id=665767&course_id=755"
```

```json
{
  "quarter": 1,
  "progress": 100,
  "user_id": 665767,
  "course_id": 755
}
```

#### Прогресс по всему курсу

`GET /api/v1/progress/course?user_id=<user_id>&course_id=<course_id>`

Что отдаёт:
- `progress` — агрегированный процент прогресса за весь курс от `0` до `100`

Пример:

```bash
curl -s "http://localhost:8081/api/v1/progress/course?user_id=665767&course_id=755"
```

```json
{
  "progress": 62,
  "user_id": 665767,
  "course_id": 755
}
```

### 2. Видео

Эти endpoint’ы показывают, сколько видеоуроков пользователь посмотрел относительно общего числа видеоуроков.

#### Видео по четверти

`GET /api/v1/videos/quarter?quarter=<1..4>&user_id=<user_id>&course_id=<course_id>`

Поля:
- `watched_count` — сколько видео посмотрено
- `total_count` — сколько видео всего в этой четверти

```json
{
  "course_id": 755,
  "user_id": 665767,
  "quarter": 1,
  "watched_count": 1,
  "total_count": 1
}
```

#### Видео по году

`GET /api/v1/videos/year?user_id=<user_id>&course_id=<course_id>`

```json
{
  "course_id": 755,
  "user_id": 665767,
  "watched_count": 24,
  "total_count": 35
}
```

Как это можно использовать на frontend:
- прогресс-бар по видео
- подпись вида `24 / 35`

### 3. Баллы

Эти endpoint’ы показывают, сколько баллов ученик набрал относительно максимума.

#### Баллы по четверти

`GET /api/v1/points/quarter?quarter=<1..4>&user_id=<user_id>&course_id=<course_id>`

Поля:
- `earned_count` — сколько баллов набрано
- `total_count` — максимум баллов в четверти

```json
{
  "course_id": 755,
  "user_id": 665767,
  "quarter": 1,
  "earned_count": 33,
  "total_count": 36
}
```

#### Баллы по году

`GET /api/v1/points/year?user_id=<user_id>&course_id=<course_id>`

```json
{
  "course_id": 755,
  "user_id": 665767,
  "earned_count": 51,
  "total_count": 56
}
```

Как это можно использовать на frontend:
- круговой индикатор или progress bar
- подпись вида `51 из 56 баллов`

### 4. Конспекты и материалы

Эти endpoint’ы показывают, сколько материалов пользователь изучил.

Что считается изученным:
- открытый конспект/перевод урока
- открытый preparation material

#### Конспекты по четверти

`GET /api/v1/conspects/quarter?quarter=<1..4>&user_id=<user_id>&course_id=<course_id>`

Поля:
- `learned_count` — сколько материалов изучено
- `total_count` — сколько материалов всего в четверти

```json
{
  "course_id": 755,
  "user_id": 665767,
  "quarter": 1,
  "learned_count": 4,
  "total_count": 14
}
```

#### Конспекты по году

`GET /api/v1/conspects/year?user_id=<user_id>&course_id=<course_id>`

```json
{
  "course_id": 755,
  "user_id": 665767,
  "learned_count": 4,
  "total_count": 36
}
```

Как это можно использовать на frontend:
- блок “изучено материалов”
- отдельный progress bar по учебным материалам

### 5. Статистика по медиа-контенту

Эти endpoint’ы показывают, какого типа видео пользователь смотрел:
- `live_lesson` — просмотр прямой трансляции
- `recorded_lesson` — просмотр записи трансляции
- `prerecorded_lesson` — просмотр предзаписанного урока

Важно:
- `live_lesson_count + recorded_lesson_count + prerecorded_lesson_count = total_watched_videos_cnt`
- если `total_watched_videos_cnt > 0`, то сумма трёх процентов всегда `100`

#### Медиа по четверти

`GET /api/v1/media/quarter?quarter=<1..4>&user_id=<user_id>&course_id=<course_id>`

Поля:
- `quarted_id` — номер четверти
- `*_count` — сколько видео этого типа просмотрено
- `*_percentage` — доля этого типа среди всех просмотренных видео
- `total_watched_videos_cnt` — всего просмотренных видео

```json
{
  "user_id": 665767,
  "course_id": 755,
  "quarted_id": 1,
  "live_lesson_count": 0,
  "live_lesson_percentage": 0,
  "recorded_lesson_count": 3,
  "recorded_lesson_percentage": 60,
  "prerecorded_lesson_count": 2,
  "prerecorded_lesson_percentage": 40,
  "total_watched_videos_cnt": 5
}
```

#### Медиа по году

`GET /api/v1/media/year?user_id=<user_id>&course_id=<course_id>`

```json
{
  "user_id": 665767,
  "course_id": 755,
  "live_lesson_count": 0,
  "live_lesson_percentage": 0,
  "recorded_lesson_count": 8,
  "recorded_lesson_percentage": 57,
  "prerecorded_lesson_count": 6,
  "prerecorded_lesson_percentage": 43,
  "total_watched_videos_cnt": 14
}
```

Как это можно использовать на frontend:
- pie chart / donut chart по типам контента
- легенда `live / recorded / prerecorded`
- подпись общего количества просмотренных видео

## Быстрые запросы для проверки

```bash
curl -s "http://localhost:8081/api/v1/progress/course?user_id=665767&course_id=755"
curl -s "http://localhost:8081/api/v1/videos/year?user_id=665767&course_id=755"
curl -s "http://localhost:8081/api/v1/points/year?user_id=665767&course_id=755"
curl -s "http://localhost:8081/api/v1/conspects/year?user_id=665767&course_id=755"
curl -s "http://localhost:8081/api/v1/media/year?user_id=665767&course_id=755"
```

## Технические примечания

- API сейчас возвращает только JSON.
- Все endpoint’ы read-only, побочных эффектов нет.
- Для quarter-endpoint’ов backend сам делит пользовательскую траекторию курса на 4 части.
- Метрики считаются по данным из PostgreSQL, уже загруженным в `xakaton`.
