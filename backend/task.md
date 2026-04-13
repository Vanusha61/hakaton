# Реализовать следующие эндпониты

## Стаститика по медиа контенту

Нужно вывести данные по четвертям и по году, в каком соотношении пользователь смотрел, какого типа материалы:
1. Трансляции (прямые)   - live_lesson
2. Трансляции (в записи) - recorded_lesson
3. Предзаписанные видео  - prerecorded_lesson

Нужно получать статистику по четверям и по году. 

### Формат ответа для четверти

```json
{
  "user_id": <int>,
  "course_id": <int>,
  "quarted_id": <int>, # 1, 2, 3, 4
  "live_lesson_count": <int>,       # количество просмотренных видео в прямом эфире
  "live_lesson_percentage": <int>,  # процент от просмотренных видео от 0 до 100
  "recorded_lesson_count": <int>,       # количество просмотренных видео в записи
  "recorded_lesson_percentage": <int>,  # процент от просмотренных видео от 0 до 100
  "prerecorded_lesson_count": <int>,       # количество просмотренных видео предзаписанных
  "prerecorded_lesson_percentage": <int>,  # процент от просмотренных видео от 0 до 100
  "total_watched_videos_cnt": <int> # общее количетсво просмотренных видео 
}
```

Важно:
1. live_lesson_count + recorded_lesson_count + prerecorded_lesson_count = total_watched_videos_cnt
2. live_lesson_percentage + recorded_lesson_percentage + prerecorded_lesson_percentage = 100

Для года -то же самое

### Формат ответа для года

```json
{
  "user_id": <int>,
  "course_id": <int>,
  "live_lesson_count": <int>,       # количество просмотренных видео в прямом эфире
  "live_lesson_percentage": <int>,  # процент от просмотренных видео от 0 до 100
  "recorded_lesson_count": <int>,       # количество просмотренных видео в записи
  "recorded_lesson_percentage": <int>,  # процент от просмотренных видео от 0 до 100
  "prerecorded_lesson_count": <int>,       # количество просмотренных видео предзаписанных
  "prerecorded_lesson_percentage": <int>,  # процент от просмотренных видео от 0 до 100
  "total_watched_videos_cnt": <int> # общее количетсво просмотренных видео 
}
```
