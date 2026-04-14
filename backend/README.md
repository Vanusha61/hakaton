# YearReporter Backend

Backend на C++/Drogon, который отдаёт frontend метрики по прохождению курса учеником.

Сейчас API умеет отдавать:
- процент прогресса по четверти
- процент прогресса по всему курсу
- количество просмотренных видео
- количество набранных баллов
- количество изученных конспектов и материалов
- статистику по типам медиа-контента
- космический хронотип ученика
- количество полностью решённых уроков
- флаг, посещены ли все уроки
- топ-3 сложных урока, которые пользователь решил, а другие часто не решали
- самый редкий из посещённых пользователем уроков с готовым storytelling-текстом
- bool-флаг, был ли пользователь первым, кто начал смотреть видео
- профиль внимания по сегментам видео с AI-интерпретацией

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
curl -s "http://localhost:8081/api/v1/chronotype/year?user_id=665767&course_id=755"
curl -s "http://localhost:8081/api/v1/solved_lessons/year?user_id=665767&course_id=755"
curl -s "http://localhost:8081/api/v1/attendance/year?user_id=665767&course_id=755"
curl -s "http://localhost:8081/api/v1/rare_lessons/year?user_id=665767&course_id=755"
curl -s "http://localhost:8081/api/v1/rarest_visited_lesson/year?user_id=665767&course_id=755"
curl -s "http://localhost:8081/api/v1/first_video_starter/year?user_id=665767&course_id=755"
curl -s "http://localhost:8081/api/v1/video_attention/year?user_id=665767&course_id=755"
```

## Технические примечания

- API сейчас возвращает только JSON.
- Все endpoint’ы read-only, побочных эффектов нет.
- Для quarter-endpoint’ов backend сам делит пользовательскую траекторию курса на 4 части.
- Метрики считаются по данным из PostgreSQL, уже загруженным в `xakaton`.

### 6. Космический хронотип (Storytelling)

Эти endpoint’ы определяют, в какое локальное время ученик чаще всего проявляет активность, и возвращают готовый narrative-блок для годового отчёта.

Источник данных:
- `wk_users_courses_actions.created_at` — время действий пользователя
- `students_of_interest.timezone` — timezone ученика для перевода из UTC в локальное время

Что считается активностью:
- любые записи из `wk_users_courses_actions` внутри выбранного курса пользователя
- для quarter-версии курс режется на 4 равные временные части по интервалу от `user_courses.created_at` до `user_courses.access_finished_at`

#### Космический хронотип по четверти

`GET /api/v1/chronotype/quarter?quarter=<1..4>&user_id=<user_id>&course_id=<course_id>`

Поля:
- `timezone` — локальная timezone ученика
- `peak_local_hour` — локальный час с максимальным количеством действий
- `peak_time` — текстовый диапазон пикового времени
- `type` — тип хронотипа
- `report_name` — название блока для отчёта
- `insight` — готовый narrative-текст
- `peak_actions_count` — сколько действий пришлось на пиковый час
- `total_actions_count` — сколько действий учтено в четверти

```json
{
  "course_id": 755,
  "user_id": 665767,
  "quarter": 1,
  "timezone": "Asia/Chita",
  "peak_local_hour": 21,
  "peak_time": "18:00 - 22:00",
  "type": "Вечерний",
  "report_name": "Вечерняя звезда",
  "insight": "Твои двигатели работают на полную в конце дня. Спокойная вечерняя атмосфера помогает тебе лучше концентрироваться.",
  "peak_actions_count": 15,
  "total_actions_count": 71
}
```

#### Космический хронотип по году

`GET /api/v1/chronotype/year?user_id=<user_id>&course_id=<course_id>`

```json
{
  "course_id": 755,
  "user_id": 665767,
  "timezone": "Asia/Chita",
  "peak_local_hour": 22,
  "peak_time": "18:00 - 22:00",
  "type": "Вечерний",
  "report_name": "Вечерняя звезда",
  "insight": "Твои двигатели работают на полную в конце дня. Спокойная вечерняя атмосфера помогает тебе лучше концентрироваться.",
  "peak_actions_count": 25,
  "total_actions_count": 138
}
```

Как это можно использовать на frontend:
- как storytelling-карточку в годовом отчёте
- как бейдж или subtitle в профиле ученика
- как часть narrative-блока рядом с графиками прогресса

### 7. Полностью решённые уроки

Эти endpoint’ы показывают, в скольких уроках пользователь решил все задания.

Что считается “полностью решённым уроком”:
- урок относится к задачным: `lessons.task_expected = true` или `lessons.wk_task_count > 0`
- у пользователя по нему `user_lessons.solved = true`

#### Полностью решённые уроки по четверти

`GET /api/v1/solved_lessons/quarter?quarter=<1..4>&user_id=<user_id>&course_id=<course_id>`

```json
{
  "course_id": 755,
  "user_id": 665767,
  "quarted_id": 1,
  "full_solved_lessons_count": 7
}
```

#### Полностью решённые уроки по году

`GET /api/v1/solved_lessons/year?user_id=<user_id>&course_id=<course_id>`

```json
{
  "course_id": 755,
  "user_id": 665767,
  "full_solved_lessons_count": 11
}
```

Как это можно использовать на frontend:
- как achievement-counter в карточке ученика
- как число рядом с блоком “задачи” или “успехи”

### 8. Посетил ли ученик все уроки

Эти endpoint’ы отвечают на вопрос, не пропустил ли пользователь ни одного урока.

Что считается посещённым уроком:
- у пользователя есть запись в `user_lessons` для этого урока в его траектории

#### Посещены ли все уроки по четверти

`GET /api/v1/attendance/quarter?quarter=<1..4>&user_id=<user_id>&course_id=<course_id>`

```json
{
  "course_id": 755,
  "user_id": 665767,
  "quarted_id": 1,
  "is_user_visited_all_lessons": true
}
```

#### Посещены ли все уроки по году

`GET /api/v1/attendance/year?user_id=<user_id>&course_id=<course_id>`

```json
{
  "course_id": 755,
  "user_id": 665767,
  "is_user_visited_all_lessons": false
}
```

Как это можно использовать на frontend:
- как badge “без прогулов”
- как булев achievement для годового отчёта

### 9. Топ-3 сложных урока

Эти endpoint’ы возвращают уроки, которые пользователь решил, но которые глобально решило мало учеников на этом курсе.

Алгоритм:
- берём только уроки пользователя, где `user_lessons.solved = true`
- для каждого такого урока считаем глобальный success rate по всем ученикам курса
- сортируем по возрастанию success rate
- в ответ кладём `not_solved_percentage = 100 - success_rate`

Важно:
- в текущем датасете у уроков нет отдельного текстового title, поэтому backend отдаёт стабильный fallback-title вида `Урок 12` или `Урок #5511`

#### Топ-3 сложных урока по четверти

`GET /api/v1/rare_lessons/quarter?quarter=<1..4>&user_id=<user_id>&course_id=<course_id>`

```json
{
  "course_id": 755,
  "user_id": 665767,
  "quarted_id": 1,
  "rare_lessons": {
    "1": {
      "title": "Урок 2",
      "not_solved_percentage": 30
    },
    "2": {
      "title": "Урок 3",
      "not_solved_percentage": 30
    },
    "3": {
      "title": "Урок 4",
      "not_solved_percentage": 30
    }
  }
}
```

#### Топ-3 сложных урока по году

`GET /api/v1/rare_lessons/year?user_id=<user_id>&course_id=<course_id>`

```json
{
  "course_id": 755,
  "user_id": 665767,
  "rare_lessons": {
    "1": {
      "title": "Урок 12",
      "not_solved_percentage": 50
    },
    "2": {
      "title": "Урок 16",
      "not_solved_percentage": 50
    },
    "3": {
      "title": "Урок 11",
      "not_solved_percentage": 40
    }
  }
}
```

Как это можно использовать на frontend:
- как storytelling-блок “твои уникальные победы”
- как список из 3 карточек с процентом учеников, которые не справились

### 10. Самый редкий посещённый урок

Эти endpoint’ы находят самый редкий урок среди тех, которые пользователь действительно посетил.

Как считается редкость:
- урок считается посещённым, если в `user_lessons` есть `translation_visited = true` или `video_visited = true`
- для каждого такого урока считаем глобальный охват: сколько уникальных учеников курса его посетило
- делим число посетивших урок на общее число учеников курса из `user_courses`
- выбираем урок с минимальным `visited_percentage`

Важно:
- для storytelling-блока backend берёт только пользовательские уроки с нормальным `lesson_number`, чтобы в ответе не появлялись технические внутренние элементы без номера

#### Самый редкий посещённый урок по четверти

`GET /api/v1/rarest_visited_lesson/quarter?quarter=<1..4>&user_id=<user_id>&course_id=<course_id>`

```json
{
  "course_id": 755,
  "user_id": 665767,
  "quarted_id": 1,
  "title": "Урок 4",
  "visited_percentage": 60,
  "visited_users_count": 6,
  "total_users_count": 10,
  "report_name": "Ты посетил самый редкий урок",
  "insight": "Пока другие отдыхали, ты изучал Урок 4. Это занятие оказалось самым эксклюзивным в этой четверти — лишь 6 из 10 учеников добрались до этой темы. Твоя жажда знаний помогает ракете прокладывать маршруты там, где другие боятся лететь."
}
```

#### Самый редкий посещённый урок по году

`GET /api/v1/rarest_visited_lesson/year?user_id=<user_id>&course_id=<course_id>`

```json
{
  "course_id": 755,
  "user_id": 665767,
  "title": "Урок 12",
  "visited_percentage": 40,
  "visited_users_count": 4,
  "total_users_count": 10,
  "report_name": "Ты посетил самый редкий урок",
  "insight": "Пока другие отдыхали, ты изучал Урок 12. Это занятие оказалось самым эксклюзивным в этом году — лишь 4 из 10 учеников добрались до этой темы. Твоя жажда знаний помогает ракете прокладывать маршруты там, где другие боятся лететь."
}
```

Как это можно использовать на frontend:
- как отдельную storytelling-карточку в годовом отчёте
- как narrative-подпись рядом с самым интересным lesson insight

### 11. Первый приступил к просмотру видео

Эти endpoint’ы отвечают на вопрос, был ли пользователь самым первым, кто начал смотреть видео в рамках курса или выбранной четверти.

Как считается:
- берём все `media_view_sessions`, относящиеся к урокам траектории пользователя
- для каждого viewer считаем его минимальный `started_at`
- сравниваем старт пользователя с самым ранним стартом по всем ученикам курса

#### Первый старт просмотра по четверти

`GET /api/v1/first_video_starter/quarter?quarter=<1..4>&user_id=<user_id>&course_id=<course_id>`

```json
{
  "course_id": 755,
  "user_id": 665767,
  "quarted_id": 1,
  "is_user_first_video_starter": false,
  "user_first_started_at": "2025-12-04 20:10:00",
  "global_first_started_at": "2025-12-01 14:59:00"
}
```

#### Первый старт просмотра по году

`GET /api/v1/first_video_starter/year?user_id=<user_id>&course_id=<course_id>`

```json
{
  "course_id": 755,
  "user_id": 665767,
  "is_user_first_video_starter": false,
  "user_first_started_at": "2025-12-04 20:10:00",
  "global_first_started_at": "2025-12-01 14:59:00"
}
```

Как это можно использовать на frontend:
- как badge “первопроходец”
- как булев achievement в годовом отчёте

### 12. Профиль внимания по сегментам видео

Эти endpoint’ы агрегируют `viewed_segments` из `media_view_sessions`, переводят просмотренные сегменты в относительную позицию по формуле `segment_index / segments_total * 100` и ищут самый частый участок просмотра.

AI-интерпретация:
- пик в начале `0-15%` -> `Быстрый старт`
- пик в середине `40-60%` -> `Исследователь сути`
- пик в конце `80-100%` -> `Финалист`
- несколько сопоставимых пиков -> `Деталист`

#### Профиль внимания по четверти

`GET /api/v1/video_attention/quarter?quarter=<1..4>&user_id=<user_id>&course_id=<course_id>`

```json
{
  "course_id": 755,
  "user_id": 665767,
  "quarted_id": 1,
  "peak_position_percentage": 0,
  "role": "Быстрый старт",
  "report_name": "Пик в начале",
  "insight": "Ты всегда тщательно проверяешь системы перед взлетом. Вводные инструкции — твой фундамент!"
}
```

#### Профиль внимания по году

`GET /api/v1/video_attention/year?user_id=<user_id>&course_id=<course_id>`

```json
{
  "course_id": 755,
  "user_id": 665767,
  "peak_position_percentage": 0,
  "role": "Быстрый старт",
  "report_name": "Пик в начале",
  "insight": "Ты всегда тщательно проверяешь системы перед взлетом. Вводные инструкции — твой фундамент!"
}
```

Как это можно использовать на frontend:
- как AI-storytelling-карточку про стиль просмотра видео
- как подпись рядом с графиком attention heatmap
