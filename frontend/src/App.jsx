import { useState, useEffect, useRef, useCallback } from 'react'
import html2canvas from 'html2canvas'
import {
  LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip,
  ResponsiveContainer, PieChart, Pie, Cell, Legend,
  BarChart, Bar,
} from 'recharts'
import ModelViewer from './components/ModelViewer'
import {
  fetchQuarterProgress, fetchCourseProgress,
  fetchVideosQuarter,   fetchVideosYear,
  fetchPointsQuarter,   fetchPointsYear,
  fetchConspectsQuarter,fetchConspectsYear,
  fetchMediaQuarter,    fetchMediaYear,
  fetchChronotypeQuarter, fetchChronotypeYear,
  fetchSolvedLessonsQuarter, fetchSolvedLessonsYear,
  fetchAttendanceQuarter,    fetchAttendanceYear,
  fetchRareLessonsQuarter,   fetchRareLessonsYear,
  fetchRarestLessonQuarter,  fetchRarestLessonYear,

  fetchVideoAttentionQuarter,    fetchVideoAttentionYear,
  fetchStudyRecsQuarter,         fetchStudyRecsYear,
  fetchPerseveranceYear,
  fetchTrainingShortestQuarter,  fetchTrainingShortestYear,
  fetchTrainingLongestQuarter,   fetchTrainingLongestYear,
  fetchTrainingSolvedTasksQuarter, fetchTrainingSolvedTasksYear,
  fetchTrainingPerfectStreakQuarter, fetchTrainingPerfectStreakYear,
  fetchTrainingStarterRankQuarter,  fetchTrainingStarterRankYear,
  fetchTrainingDifficultyStatsQuarter, fetchTrainingDifficultyStatsYear,
  fetchTrainingTypeProfileYear,
  fetchAwardBadgesCountQuarter,  fetchAwardBadgesCountYear,
  fetchAwardBadgesLevelsQuarter, fetchAwardBadgesLevelsYear,
  fetchAwardBadgesRarestYear,
  fetchPartialCreditShareYear,
  fetchSchoolVideoRankQuarter,   fetchSchoolVideoRankYear,
  fetchRegionPointsRankQuarter,  fetchRegionPointsRankYear,
  fetchClassTextDepthRankQuarter, fetchClassTextDepthRankYear,
  fetchFirstVideoStarterQuarter, fetchFirstVideoStarterYear,
  fetchHomeworkTasksQuarter,     fetchHomeworkTasksYear,
  fetchLessonTasksQuarter,       fetchLessonTasksYear,
} from './api'
import './App.css'

// ── СТАТИКА ───────────────────────────────────────────────────────────────────
const STUDENT = { name: 'Иван Петров', course: 'Английский язык · 2024–2025' }

const QUARTER_MODELS = [
  '/models/stage1.glb',
  '/models/stage2.glb',
  '/models/stage3.glb',
  '/models/stage4.glb',
]

const MEDIA_COLORS = ['#6c63ff', '#36c2cf', '#f97316']
const MEDIA_LABELS = {
  live_lesson:        'Прямой эфир',
  recorded_lesson:    'Запись трансляции',
  prerecorded_lesson: 'Предзаписанный урок',
}

// Все 4 хронотипа — строки таблицы (фиксированные)
const CHRONOTYPE_ROWS = [
  { type: 'Утренний', time: '06:00 – 10:00', icon: '🌅', report_name: 'Первый луч' },
  { type: 'Дневной',  time: '11:00 – 17:00', icon: '☀️', report_name: 'Солнечный странник' },
  { type: 'Вечерний', time: '18:00 – 22:00', icon: '🌆', report_name: 'Вечерняя звезда' },
  { type: 'Ночной',   time: '23:00 – 04:00', icon: '🦉', report_name: 'Ночной охотник' },
]

// Генерируем кривую внимания (колокол вокруг peak_position_percentage)
function makeAttentionCurve(peakPct = 10) {
  const sigma = 22
  return Array.from({ length: 21 }, (_, i) => {
    const x     = i * 5
    const gauss = Math.exp(-0.5 * ((x - peakPct) / sigma) ** 2)
    const wave  = Math.sin(x * 0.18) * 0.08
    return { pos: `${x}%`, freq: Math.round(Math.max(10, (gauss + wave) * 85 + 12)) }
  })
}
// ─────────────────────────────────────────────────────────────────────────────

// Фолбек: реалистичные данные когда бэк недоступен
const MOCK = {
  quarterProgress: [100, 75, 85, 45],
  year: {
    progress:   62,
    videos:     { watched_count: 24, total_count: 35 },
    points:     { earned_count: 51, total_count: 56 },
    conspects:  { learned_count: 4,  total_count: 36 },
    media:      { live_lesson_count: 0, live_lesson_percentage: 0, recorded_lesson_count: 8, recorded_lesson_percentage: 57, prerecorded_lesson_count: 6, prerecorded_lesson_percentage: 43, total_watched_videos_cnt: 14 },
    chrono:     { timezone: 'Asia/Chita', peak_local_hour: 22, peak_time: '18:00 - 22:00', type: 'Вечерний', report_name: 'Вечерняя звезда', insight: 'Твои двигатели работают на полную в конце дня. Спокойная вечерняя атмосфера помогает тебе лучше концентрироваться.', peak_actions_count: 25, total_actions_count: 138 },
    solved:     { full_solved_lessons_count: 11 },
    attendance: { is_user_visited_all_lessons: false },
    rare:       { rare_lessons: { '1': { title: 'Урок 12', not_solved_percentage: 50 }, '2': { title: 'Урок 16', not_solved_percentage: 50 }, '3': { title: 'Урок 11', not_solved_percentage: 40 } } },
    rarest:     { title: 'Урок 12', visited_percentage: 40, visited_users_count: 4, total_users_count: 10, report_name: 'Ты посетил самый редкий урок', insight: 'Пока другие отдыхали, ты изучал Урок 12. Это занятие оказалось самым эксклюзивным в этом году — лишь 4 из 10 учеников добрались до этой темы. Твоя жажда знаний помогает ракете прокладывать маршруты там, где другие боятся лететь.' },
    attention:    { peak_position_percentage: 0, role: 'Быстрый старт', report_name: 'Пик в начале', insight: 'Ты всегда тщательно проверяешь системы перед взлетом. Вводные инструкции — твой фундамент!' },
    studyRecs:    { video: { title: 'Видео', level_percentage: 61, influence: 'Высокое 🚀' }, conspects: { title: 'Конспекты', level_percentage: 42, influence: 'Среднее 📈' }, materials: { title: 'Доп. материалы', level_percentage: 18, influence: 'Поддерживающее 🛠' }, current_average_score: 4.1, target_average_score: 4.7, recommended_focus_title: 'Конспекты', ai_insight: 'Твоя ракета летит на 2/3 мощности. Сейчас твой средний балл — 4.1. Ученики, которые уделяют «Конспекты» хотя бы 75% внимания в этом году, в среднем получают 4.7 балла. Добавь больше активности в этот блок, и твой результат может заметно вырасти.' },
    perseverance:  { repeated_tasks_count: 6, first_attempt_points: 1.0, final_points: 4.5, extra_points_from_retries: 3.5, report_name: 'Профит от упорства', insight: 'Ты не бросаешь задачу после первой ошибки и умеешь выжимать из повторных попыток дополнительный результат. Это хороший навык для длинных космических дистанций и сложных тем.' },
    partialCredit: { partial_points_percentage: 6.2, partial_points: 2.9, total_points: 47.0, partial_answers_count: 4, report_name: 'Копейка рубль бережёт', insight: 'Основная часть твоих баллов приходит из полностью решённых задач. Это сильный сигнал. Если ещё чаще забирать частичные баллы в пограничных случаях, итоговый результат вырастет ещё выше.' },
    trainShort:    { duration_human: '02:36', training_name: 'Промежуточный контроль (тестирование)' },
    trainLong:     { duration_human: '06:41', training_name: 'Промежуточный контроль (тестирование)' },
    trainTasks:    { solved_tasks_count: 14, trainings_count: 3 },
    trainStreak:   { max_solved_without_mistakes: 5, report_name: 'Серия без ошибок', insight: 'Ты решил 5 задач в тренинге без ошибок. Это отличный показатель концентрации и точности.' },
    trainRank:     { user_rank: 1, is_user_first_starter: true, is_user_top3_starter: true, training_name: 'Промежуточный контроль (тестирование)' },
    trainDiff:     { difficulty_stats: { easy_count: 0, medium_count: 0, hard_count: 3, total_count: 3 } },
    trainTypes:    { lesson_training_count: 3, lesson_training_percentage: 75, regular_training_count: 1, regular_training_percentage: 25, olympiad_training_count: 0, olympiad_training_percentage: 0, total_count: 4, has_olympiad_training: false, report_name: 'Укротитель олимпиад', insight: 'Сейчас твой профиль опирается на курсовые и регулярные тренинги. До олимпиадной орбиты пока не добрался, но фундамент для следующего рывка уже собран.' },
    badgesCount:   { award_badges_count: 3 },
    badgesLevels:  { award_badges_levels: { special_count: 0, total_count: 3, level_1_count: 1, level_2_count: 1, level_3_count: 1, level_4_count: 0, level_5_count: 0 } },
    badgesRarest:  { rarest_award_badges: [ { badge_id: 1, title: 'Олимпиадник', level: 1, special: true, image_url: 'https://u.foxford.ngcdn.ru/uploads/inner_file/file/31889/ach_olymp.svg', owners_percentage: 0.2 }, { badge_id: 6, title: 'Я решаю', level: 5, special: false, image_url: 'https://u.foxford.ngcdn.ru/uploads/inner_file/file/31894/ach_solv_5.svg', owners_percentage: 3.0 } ] },
    firstVideoStarter: { is_user_first_video_starter: true, user_first_started_at: '2025-09-02 09:15:00', global_first_started_at: '2025-09-02 09:15:00' },
    homeworkTasks: { solved_homework_tasks_count: 38 },
    lessonTasks:   { solved_lesson_tasks_count: 72 },
    schoolRank:    { school_name: 'Школа №171', watched_lessons_count: 24, total_video_lessons_count: 35, user_rank: 7, school_users_count: 14, display_mode: 'place', display_value: 7, display_text: 'Ты 7 по своей школе по просмотренным занятиям' },
    regionRank:    { region_name: 'Москва', earned_points: 148, total_points: 214, user_rank: 84, region_users_count: 1324, display_mode: 'top', display_value: 90, display_text: 'Ты входишь в топ 90 в своём регионе по баллам' },
    classRank:     { class_name: '8А', learned_count: 41, total_materials_count: 68, user_rank: 10, class_users_count: 28, display_mode: 'place', display_value: 10, display_text: 'Ты 10 в классе по глубине изучения текстов' },
  },
  quarter: {
    1: {
      progress:   100,
      videos:     { watched_count: 1,  total_count: 1  },
      points:     { earned_count: 33,  total_count: 36 },
      conspects:  { learned_count: 4,  total_count: 14 },
      media:      { quarted_id: 1, live_lesson_count: 0, live_lesson_percentage: 0, recorded_lesson_count: 3, recorded_lesson_percentage: 60, prerecorded_lesson_count: 2, prerecorded_lesson_percentage: 40, total_watched_videos_cnt: 5 },
      chrono:     { quarter: 1, timezone: 'Asia/Chita', peak_local_hour: 21, peak_time: '18:00 - 22:00', type: 'Вечерний', report_name: 'Вечерняя звезда', insight: 'Твои двигатели работают на полную в конце дня. Спокойная вечерняя атмосфера помогает тебе лучше концентрироваться.', peak_actions_count: 15, total_actions_count: 71 },
      solved:     { quarted_id: 1, full_solved_lessons_count: 7 },
      attendance: { quarted_id: 1, is_user_visited_all_lessons: true },
      rare:       { quarted_id: 1, rare_lessons: { '1': { title: 'Урок 2', not_solved_percentage: 30 }, '2': { title: 'Урок 3', not_solved_percentage: 30 }, '3': { title: 'Урок 4', not_solved_percentage: 30 } } },
      rarest:     { quarted_id: 1, title: 'Урок 4', visited_percentage: 60, visited_users_count: 6, total_users_count: 10, report_name: 'Ты посетил самый редкий урок', insight: 'Пока другие отдыхали, ты изучал Урок 4. Это занятие оказалось самым эксклюзивным в первой четверти — лишь 6 из 10 учеников добрались до этой темы. Твоя жажда знаний помогает ракете прокладывать маршруты там, где другие боятся лететь.' },
      attention:  { quarted_id: 1, peak_position_percentage: 0, role: 'Быстрый старт', report_name: 'Пик в начале', insight: 'Ты всегда тщательно проверяешь системы перед взлетом. Вводные инструкции — твой фундамент!' },
      studyRecs:   { quarter: 1, video: { title: 'Видео', level_percentage: 34, influence: 'Высокое 🚀' }, conspects: { title: 'Конспекты', level_percentage: 28, influence: 'Среднее 📈' }, materials: { title: 'Доп. материалы', level_percentage: 10, influence: 'Поддерживающее 🛠' }, current_average_score: 4.1, target_average_score: 4.6, recommended_focus_title: 'Видео', ai_insight: 'Твоя ракета летит на 1/3 мощности. Сейчас твой средний балл — 4.1. Ученики, которые уделяют «Видео» хотя бы 75% внимания, в среднем получают 4.6 балла.' },
      trainShort:  { duration_human: '01:48', training_name: 'Промежуточный контроль' },
      trainLong:   { duration_human: '04:20', training_name: 'Промежуточный контроль' },
      trainTasks:  { solved_tasks_count: 4, trainings_count: 1 },
      trainStreak: { max_solved_without_mistakes: 4, report_name: 'Серия без ошибок', insight: 'Ты решил 4 задачи без ошибок — отличная концентрация!' },
      trainRank:   { user_rank: 2, is_user_first_starter: false, is_user_top3_starter: true, training_name: 'Промежуточный контроль' },
      trainDiff:   { difficulty_stats: { easy_count: 0, medium_count: 0, hard_count: 1, total_count: 1 } },
      badgesCount: { award_badges_count: 2 },
      badgesLevels:{ award_badges_levels: { special_count: 0, total_count: 2, level_1_count: 1, level_2_count: 1, level_3_count: 0, level_4_count: 0, level_5_count: 0 } },
      firstVideoStarter: { is_user_first_video_starter: true, user_first_started_at: '2025-12-01 14:59:00', global_first_started_at: '2025-12-01 14:59:00' },
      schoolRank:  { school_name: 'Школа №171', watched_lessons_count: 1, total_video_lessons_count: 1, user_rank: 5, school_users_count: 14, display_mode: 'place', display_value: 5, display_text: 'Ты 5 по своей школе в 1 четверти' },
      regionRank:  { region_name: 'Москва', earned_points: 33, total_points: 36, user_rank: 60, region_users_count: 1324, display_mode: 'top', display_value: 75, display_text: 'Ты входишь в топ 75 в своём регионе' },
      classRank:   { class_name: '8А', learned_count: 4, total_materials_count: 14, user_rank: 12, class_users_count: 28, display_mode: 'place', display_value: 12, display_text: 'Ты 12 в классе по изучению текстов' },
      homeworkTasks: { solved_homework_tasks_count: 8 },
      lessonTasks:   { solved_lesson_tasks_count: 18 },
    },
    2: {
      progress:   75,
      videos:     { watched_count: 8,  total_count: 10 },
      points:     { earned_count: 45,  total_count: 52 },
      conspects:  { learned_count: 6,  total_count: 12 },
      media:      { quarted_id: 2, live_lesson_count: 0, live_lesson_percentage: 0, recorded_lesson_count: 5, recorded_lesson_percentage: 63, prerecorded_lesson_count: 3, prerecorded_lesson_percentage: 37, total_watched_videos_cnt: 8 },
      chrono:     { quarter: 2, timezone: 'Asia/Chita', peak_local_hour: 21, peak_time: '18:00 - 22:00', type: 'Вечерний', report_name: 'Вечерняя звезда', insight: 'Твои двигатели работают на полную в конце дня. Спокойная вечерняя атмосфера помогает тебе лучше концентрироваться.', peak_actions_count: 18, total_actions_count: 82 },
      solved:     { quarted_id: 2, full_solved_lessons_count: 9 },
      attendance: { quarted_id: 2, is_user_visited_all_lessons: false },
      rare:       { quarted_id: 2, rare_lessons: { '1': { title: 'Урок 7', not_solved_percentage: 45 }, '2': { title: 'Урок 9', not_solved_percentage: 40 }, '3': { title: 'Урок 6', not_solved_percentage: 35 } } },
      rarest:     { quarted_id: 2, title: 'Урок 7', visited_percentage: 50, visited_users_count: 5, total_users_count: 10, report_name: 'Ты посетил самый редкий урок', insight: 'Пока другие отдыхали, ты изучал Урок 7. Это занятие оказалось самым эксклюзивным во второй четверти — лишь 5 из 10 учеников добрались до этой темы. Твоя жажда знаний помогает ракете прокладывать маршруты там, где другие боятся лететь.' },
      attention:  { quarted_id: 2, peak_position_percentage: 50, role: 'Исследователь сути', report_name: 'Пик в середине', insight: 'Ты идёшь прямо к сути. Середина урока — это твоя орбита, где происходит главное открытие.' },
      studyRecs:   { quarter: 2, video: { title: 'Видео', level_percentage: 45, influence: 'Высокое 🚀' }, conspects: { title: 'Конспекты', level_percentage: 32, influence: 'Среднее 📈' }, materials: { title: 'Доп. материалы', level_percentage: 14, influence: 'Поддерживающее 🛠' }, current_average_score: 3.7, target_average_score: 4.5, recommended_focus_title: 'Конспекты', ai_insight: 'Твоя ракета набирает скорость. Сейчас твой средний балл — 3.7. Добавь больше работы с конспектами, и результат вырастет до 4.5.' },
      trainShort:  { duration_human: '02:10', training_name: 'Промежуточный контроль' },
      trainLong:   { duration_human: '05:30', training_name: 'Промежуточный контроль' },
      trainTasks:  { solved_tasks_count: 6, trainings_count: 1 },
      trainStreak: { max_solved_without_mistakes: 5, report_name: 'Серия без ошибок', insight: 'Ты решил 5 задач без ошибок — чёткая работа!' },
      trainRank:   { user_rank: 1, is_user_first_starter: true, is_user_top3_starter: true, training_name: 'Промежуточный контроль' },
      trainDiff:   { difficulty_stats: { easy_count: 0, medium_count: 0, hard_count: 1, total_count: 1 } },
      badgesCount: { award_badges_count: 1 },
      badgesLevels:{ award_badges_levels: { special_count: 0, total_count: 1, level_1_count: 0, level_2_count: 1, level_3_count: 0, level_4_count: 0, level_5_count: 0 } },
      firstVideoStarter: { is_user_first_video_starter: false, user_first_started_at: '2026-01-15 19:00:00', global_first_started_at: '2026-01-14 10:00:00' },
      schoolRank:  { school_name: 'Школа №171', watched_lessons_count: 8, total_video_lessons_count: 10, user_rank: 4, school_users_count: 14, display_mode: 'place', display_value: 4, display_text: 'Ты 4 по своей школе во 2 четверти' },
      regionRank:  { region_name: 'Москва', earned_points: 45, total_points: 52, user_rank: 50, region_users_count: 1324, display_mode: 'top', display_value: 85, display_text: 'Ты входишь в топ 85 в своём регионе' },
      classRank:   { class_name: '8А', learned_count: 6, total_materials_count: 12, user_rank: 9, class_users_count: 28, display_mode: 'place', display_value: 9, display_text: 'Ты 9 в классе по изучению текстов' },
      homeworkTasks: { solved_homework_tasks_count: 12 },
      lessonTasks:   { solved_lesson_tasks_count: 24 },
    },
    3: {
      progress:   85,
      videos:     { watched_count: 12, total_count: 14 },
      points:     { earned_count: 48,  total_count: 55 },
      conspects:  { learned_count: 8,  total_count: 14 },
      media:      { quarted_id: 3, live_lesson_count: 1, live_lesson_percentage: 8, recorded_lesson_count: 7, recorded_lesson_percentage: 58, prerecorded_lesson_count: 4, prerecorded_lesson_percentage: 33, total_watched_videos_cnt: 12 },
      chrono:     { quarter: 3, timezone: 'Asia/Chita', peak_local_hour: 20, peak_time: '18:00 - 22:00', type: 'Вечерний', report_name: 'Вечерняя звезда', insight: 'Твои двигатели работают на полную в конце дня. Спокойная вечерняя атмосфера помогает тебе лучше концентрироваться.', peak_actions_count: 22, total_actions_count: 95 },
      solved:     { quarted_id: 3, full_solved_lessons_count: 10 },
      attendance: { quarted_id: 3, is_user_visited_all_lessons: true },
      rare:       { quarted_id: 3, rare_lessons: { '1': { title: 'Урок 10', not_solved_percentage: 55 }, '2': { title: 'Урок 13', not_solved_percentage: 48 }, '3': { title: 'Урок 8', not_solved_percentage: 42 } } },
      rarest:     { quarted_id: 3, title: 'Урок 10', visited_percentage: 45, visited_users_count: 4, total_users_count: 9, report_name: 'Ты посетил самый редкий урок', insight: 'Пока другие отдыхали, ты изучал Урок 10. Это занятие оказалось самым эксклюзивным в третьей четверти — лишь 4 из 9 учеников добрались до этой темы. Твоя жажда знаний помогает ракете прокладывать маршруты там, где другие боятся лететь.' },
      attention:  { quarted_id: 3, peak_position_percentage: 85, role: 'Финалист', report_name: 'Пик в конце', insight: 'Ты доводишь каждый урок до финала. Именно в конце видео скрыты самые важные выводы — и ты это знаешь.' },
      studyRecs:   { quarter: 3, video: { title: 'Видео', level_percentage: 58, influence: 'Высокое 🚀' }, conspects: { title: 'Конспекты', level_percentage: 40, influence: 'Среднее 📈' }, materials: { title: 'Доп. материалы', level_percentage: 18, influence: 'Поддерживающее 🛠' }, current_average_score: 3.9, target_average_score: 4.6, recommended_focus_title: 'Конспекты', ai_insight: 'Хороший темп! Твой средний балл — 3.9. Подтяни конспекты и сможешь выйти на уровень 4.6 — как у лучших в потоке.' },
      trainShort:  { duration_human: '02:00', training_name: 'Промежуточный контроль' },
      trainLong:   { duration_human: '06:00', training_name: 'Промежуточный контроль' },
      trainTasks:  { solved_tasks_count: 8, trainings_count: 1 },
      trainStreak: { max_solved_without_mistakes: 6, report_name: 'Серия без ошибок', insight: 'Ты решил 6 задач без ошибок — отличная точность!' },
      trainRank:   { user_rank: 3, is_user_first_starter: false, is_user_top3_starter: true, training_name: 'Промежуточный контроль' },
      trainDiff:   { difficulty_stats: { easy_count: 0, medium_count: 0, hard_count: 1, total_count: 1 } },
      badgesCount: { award_badges_count: 1 },
      badgesLevels:{ award_badges_levels: { special_count: 0, total_count: 1, level_1_count: 1, level_2_count: 0, level_3_count: 0, level_4_count: 0, level_5_count: 0 } },
      firstVideoStarter: { is_user_first_video_starter: false, user_first_started_at: '2026-02-20 18:30:00', global_first_started_at: '2026-02-19 09:00:00' },
      schoolRank:  { school_name: 'Школа №171', watched_lessons_count: 12, total_video_lessons_count: 14, user_rank: 3, school_users_count: 14, display_mode: 'place', display_value: 3, display_text: 'Ты 3 по своей школе в 3 четверти' },
      regionRank:  { region_name: 'Москва', earned_points: 48, total_points: 55, user_rank: 42, region_users_count: 1324, display_mode: 'top', display_value: 92, display_text: 'Ты входишь в топ 92 в своём регионе' },
      classRank:   { class_name: '8А', learned_count: 8, total_materials_count: 14, user_rank: 7, class_users_count: 28, display_mode: 'place', display_value: 7, display_text: 'Ты 7 в классе по изучению текстов' },
      homeworkTasks: { solved_homework_tasks_count: 14 },
      lessonTasks:   { solved_lesson_tasks_count: 20 },
    },
    4: {
      progress:   45,
      videos:     { watched_count: 5,  total_count: 10 },
      points:     { earned_count: 32,  total_count: 48 },
      conspects:  { learned_count: 3,  total_count: 10 },
      media:      { quarted_id: 4, live_lesson_count: 0, live_lesson_percentage: 0, recorded_lesson_count: 3, recorded_lesson_percentage: 60, prerecorded_lesson_count: 2, prerecorded_lesson_percentage: 40, total_watched_videos_cnt: 5 },
      chrono:     { quarter: 4, timezone: 'Asia/Chita', peak_local_hour: 22, peak_time: '18:00 - 22:00', type: 'Вечерний', report_name: 'Вечерняя звезда', insight: 'Твои двигатели работают на полную в конце дня. Спокойная вечерняя атмосфера помогает тебе лучше концентрироваться.', peak_actions_count: 12, total_actions_count: 54 },
      solved:     { quarted_id: 4, full_solved_lessons_count: 5 },
      attendance: { quarted_id: 4, is_user_visited_all_lessons: false },
      rare:       { quarted_id: 4, rare_lessons: { '1': { title: 'Урок 15', not_solved_percentage: 60 }, '2': { title: 'Урок 18', not_solved_percentage: 55 }, '3': { title: 'Урок 14', not_solved_percentage: 45 } } },
      rarest:     { quarted_id: 4, title: 'Урок 15', visited_percentage: 35, visited_users_count: 3, total_users_count: 9, report_name: 'Ты посетил самый редкий урок', insight: 'Пока другие отдыхали, ты изучал Урок 15. Это занятие оказалось самым эксклюзивным в четвёртой четверти — лишь 3 из 9 учеников добрались до этой темы. Твоя жажда знаний помогает ракете прокладывать маршруты там, где другие боятся лететь.' },
      attention:  { quarted_id: 4, peak_position_percentage: 0, role: 'Быстрый старт', report_name: 'Пик в начале', insight: 'Ты всегда тщательно проверяешь системы перед взлетом. Вводные инструкции — твой фундамент!' },
      studyRecs:   { quarter: 4, video: { title: 'Видео', level_percentage: 30, influence: 'Высокое 🚀' }, conspects: { title: 'Конспекты', level_percentage: 20, influence: 'Среднее 📈' }, materials: { title: 'Доп. материалы', level_percentage: 8, influence: 'Поддерживающее 🛠' }, current_average_score: 2.8, target_average_score: 4.4, recommended_focus_title: 'Видео', ai_insight: 'Есть куда расти! Твой средний балл — 2.8. Сосредоточься на видео в этой четверти — это самый быстрый путь к результату.' },
      trainShort:  { duration_human: '01:30', training_name: 'Промежуточный контроль' },
      trainLong:   { duration_human: '03:50', training_name: 'Промежуточный контроль' },
      trainTasks:  { solved_tasks_count: 4, trainings_count: 1 },
      trainStreak: { max_solved_without_mistakes: 3, report_name: 'Серия без ошибок', insight: 'Ты решил 3 задачи без ошибок подряд.' },
      trainRank:   { user_rank: 4, is_user_first_starter: false, is_user_top3_starter: false, training_name: 'Промежуточный контроль' },
      trainDiff:   { difficulty_stats: { easy_count: 0, medium_count: 0, hard_count: 1, total_count: 1 } },
      badgesCount: { award_badges_count: 0 },
      badgesLevels:{ award_badges_levels: { special_count: 0, total_count: 0, level_1_count: 0, level_2_count: 0, level_3_count: 0, level_4_count: 0, level_5_count: 0 } },
      firstVideoStarter: { is_user_first_video_starter: false, user_first_started_at: '2026-03-10 21:00:00', global_first_started_at: '2026-03-10 08:30:00' },
      schoolRank:  { school_name: 'Школа №171', watched_lessons_count: 5, total_video_lessons_count: 10, user_rank: 7, school_users_count: 14, display_mode: 'place', display_value: 7, display_text: 'Ты 7 по своей школе в 4 четверти' },
      regionRank:  { region_name: 'Москва', earned_points: 32, total_points: 48, user_rank: 84, region_users_count: 1324, display_mode: 'top', display_value: 90, display_text: 'Ты входишь в топ 90 в своём регионе' },
      classRank:   { class_name: '8А', learned_count: 3, total_materials_count: 10, user_rank: 15, class_users_count: 28, display_mode: 'place', display_value: 15, display_text: 'Ты 15 в классе по изучению текстов' },
      homeworkTasks: { solved_homework_tasks_count: 6 },
      lessonTasks:   { solved_lesson_tasks_count: 10 },
    },
  },
}

const TABS = [
  { id: 1, label: '1' },
  { id: 2, label: '2' },
  { id: 3, label: '3' },
  { id: 4, label: '4' },
  { id: 'year', label: 'Год' },
]

function StarField() {
  const canvasRef = useRef(null)

  useEffect(() => {
    const canvas = canvasRef.current
    const ctx = canvas.getContext('2d')
    let raf

    const LAYERS = [
      { count: 180, speed: 0.015, sizeMin: 0.4, sizeMax: 1.0, opacity: 0.55 },
      { count: 80,  speed: 0.035, sizeMin: 0.8, sizeMax: 1.6, opacity: 0.75 },
      { count: 30,  speed: 0.07,  sizeMin: 1.2, sizeMax: 2.4, opacity: 0.90 },
    ]

    let stars = []

    function resize() {
      canvas.width  = window.innerWidth
      canvas.height = window.innerHeight
      stars = LAYERS.flatMap(({ count, speed, sizeMin, sizeMax, opacity }) =>
        Array.from({ length: count }, () => ({
          x:      Math.random() * canvas.width,
          y:      Math.random() * canvas.height,
          r:      sizeMin + Math.random() * (sizeMax - sizeMin),
          speed,
          opacity,
          twinkleOffset: Math.random() * Math.PI * 2,
          twinkleSpeed:  0.4 + Math.random() * 0.8,
          color: Math.random() < 0.15 ? '#a0c4ff'
               : Math.random() < 0.08 ? '#ffd6a5'
               : '#ffffff',
        }))
      )
    }

    function draw(t) {
      ctx.clearRect(0, 0, canvas.width, canvas.height)
      const ts = t * 0.001

      for (const s of stars) {
        s.y += s.speed
        if (s.y > canvas.height) { s.y = 0; s.x = Math.random() * canvas.width }

        const twinkle = 0.5 + 0.5 * Math.sin(ts * s.twinkleSpeed + s.twinkleOffset)
        const alpha = s.opacity * (0.4 + 0.6 * twinkle)

        ctx.beginPath()
        ctx.arc(s.x, s.y, s.r, 0, Math.PI * 2)
        ctx.fillStyle = s.color
        ctx.globalAlpha = alpha
        ctx.fill()
      }

      ctx.globalAlpha = 1
      raf = requestAnimationFrame(draw)
    }

    resize()
    window.addEventListener('resize', resize)
    raf = requestAnimationFrame(draw)
    return () => { cancelAnimationFrame(raf); window.removeEventListener('resize', resize) }
  }, [])

  return <canvas ref={canvasRef} className="starfield" />
}

export default function App() {
  const [tab, setTab] = useState('year')

  // ── Данные ──────────────────────────────────────────────────────────────
  const [quarterProgress, setQuarterProgress] = useState([null, null, null, null])
  const [progress,   setProgress]   = useState(null)
  const [videos,     setVideos]     = useState(null)
  const [points,     setPoints]     = useState(null)
  const [conspects,  setConspects]  = useState(null)
  const [media,      setMedia]      = useState(null)
  const [chrono,     setChrono]     = useState(null)
  const [solved,     setSolved]     = useState(null)
  const [attendance, setAttendance] = useState(null)
  const [rare,       setRare]       = useState(null)
  const [rarest,     setRarest]     = useState(null)
  const [attention,    setAttention]    = useState(null)
  const [studyRecs,    setStudyRecs]    = useState(null)
  const [perseverance, setPerseverance] = useState(null)
  const [trainShort,   setTrainShort]   = useState(null)
  const [trainLong,    setTrainLong]    = useState(null)
  const [trainTasks,   setTrainTasks]   = useState(null)
  const [trainStreak,  setTrainStreak]  = useState(null)
  const [trainRank,    setTrainRank]    = useState(null)
  const [trainDiff,    setTrainDiff]    = useState(null)
  const [trainTypes,   setTrainTypes]   = useState(null)
  const [badgesCount,  setBadgesCount]  = useState(null)
  const [badgesLevels, setBadgesLevels] = useState(null)
  const [badgesRarest, setBadgesRarest] = useState(null)
  const [partialCredit,setPartialCredit]= useState(null)
  const [firstVideoStarter, setFirstVideoStarter] = useState(null)
  const [schoolRank,   setSchoolRank]   = useState(null)
  const [regionRank,   setRegionRank]   = useState(null)
  const [classRank,    setClassRank]    = useState(null)
  const [homeworkTasks, setHomeworkTasks] = useState(null)
  const [lessonTasks,   setLessonTasks]   = useState(null)
  const [loading,      setLoading]      = useState(false)

  const resetData = useCallback(() => {
    setProgress(null); setVideos(null); setPoints(null); setConspects(null)
    setMedia(null); setChrono(null); setSolved(null); setAttendance(null)
    setRare(null); setRarest(null); setAttention(null)
    setStudyRecs(null); setPerseverance(null)
    setTrainShort(null); setTrainLong(null); setTrainTasks(null)
    setTrainStreak(null); setTrainRank(null); setTrainDiff(null); setTrainTypes(null)
    setBadgesCount(null); setBadgesLevels(null); setBadgesRarest(null); setPartialCredit(null)
    setFirstVideoStarter(null); setSchoolRank(null); setRegionRank(null); setClassRank(null)
    setHomeworkTasks(null); setLessonTasks(null)
  }, [])

  useEffect(() => {
    resetData()
    setLoading(true)
    const q = typeof tab === 'number' ? tab : null

    // Прогресс по всем четвертям — всегда (для 3D сетки)
    Promise.allSettled([
      fetchQuarterProgress(1), fetchQuarterProgress(2),
      fetchQuarterProgress(3), fetchQuarterProgress(4),
    ]).then((res) =>
      setQuarterProgress(res.map((r, i) =>
        r.status === 'fulfilled' ? r.value.progress : MOCK.quarterProgress[i]
      ))
    )

    const mk = q ? MOCK.quarter[q] : MOCK.year

    const calls = q
      ? [
          fetchQuarterProgress(q).then((d) => setProgress(d.progress)).catch(() => setProgress(mk.progress)),
          fetchVideosQuarter(q).then(setVideos).catch(() => setVideos(mk.videos)),
          fetchPointsQuarter(q).then(setPoints).catch(() => setPoints(mk.points)),
          fetchConspectsQuarter(q).then(setConspects).catch(() => setConspects(mk.conspects)),
          fetchMediaQuarter(q).then(setMedia).catch(() => setMedia(mk.media)),
          fetchChronotypeQuarter(q).then(setChrono).catch(() => setChrono(mk.chrono)),
          fetchSolvedLessonsQuarter(q).then(setSolved).catch(() => setSolved(mk.solved)),
          fetchAttendanceQuarter(q).then(setAttendance).catch(() => setAttendance(mk.attendance)),
          fetchRareLessonsQuarter(q).then(setRare).catch(() => setRare(mk.rare)),
          fetchRarestLessonQuarter(q).then(setRarest).catch(() => setRarest(mk.rarest)),
          fetchVideoAttentionQuarter(q).then(setAttention).catch(() => setAttention(mk.attention)),
          fetchStudyRecsQuarter(q).then(setStudyRecs).catch(() => setStudyRecs(mk.studyRecs)),
          fetchTrainingShortestQuarter(q).then(setTrainShort).catch(() => setTrainShort(mk.trainShort)),
          fetchTrainingLongestQuarter(q).then(setTrainLong).catch(() => setTrainLong(mk.trainLong)),
          fetchTrainingSolvedTasksQuarter(q).then(setTrainTasks).catch(() => setTrainTasks(mk.trainTasks)),
          fetchTrainingPerfectStreakQuarter(q).then(setTrainStreak).catch(() => setTrainStreak(mk.trainStreak)),
          fetchTrainingStarterRankQuarter(q).then(setTrainRank).catch(() => setTrainRank(mk.trainRank)),
          fetchTrainingDifficultyStatsQuarter(q).then(setTrainDiff).catch(() => setTrainDiff(mk.trainDiff)),
          fetchAwardBadgesCountQuarter(q).then(setBadgesCount).catch(() => setBadgesCount(mk.badgesCount)),
          fetchAwardBadgesLevelsQuarter(q).then(setBadgesLevels).catch(() => setBadgesLevels(mk.badgesLevels)),
          fetchFirstVideoStarterQuarter(q).then(setFirstVideoStarter).catch(() => setFirstVideoStarter(mk.firstVideoStarter)),
          fetchSchoolVideoRankQuarter(q).then(setSchoolRank).catch(() => setSchoolRank(mk.schoolRank)),
          fetchRegionPointsRankQuarter(q).then(setRegionRank).catch(() => setRegionRank(mk.regionRank)),
          fetchClassTextDepthRankQuarter(q).then(setClassRank).catch(() => setClassRank(mk.classRank)),
          fetchHomeworkTasksQuarter(q).then(setHomeworkTasks).catch(() => setHomeworkTasks(mk.homeworkTasks)),
          fetchLessonTasksQuarter(q).then(setLessonTasks).catch(() => setLessonTasks(mk.lessonTasks)),
        ]
      : [
          fetchCourseProgress().then((d) => setProgress(d.progress)).catch(() => setProgress(mk.progress)),
          fetchVideosYear().then(setVideos).catch(() => setVideos(mk.videos)),
          fetchPointsYear().then(setPoints).catch(() => setPoints(mk.points)),
          fetchConspectsYear().then(setConspects).catch(() => setConspects(mk.conspects)),
          fetchMediaYear().then(setMedia).catch(() => setMedia(mk.media)),
          fetchChronotypeYear().then(setChrono).catch(() => setChrono(mk.chrono)),
          fetchSolvedLessonsYear().then(setSolved).catch(() => setSolved(mk.solved)),
          fetchAttendanceYear().then(setAttendance).catch(() => setAttendance(mk.attendance)),
          fetchRareLessonsYear().then(setRare).catch(() => setRare(mk.rare)),
          fetchRarestLessonYear().then(setRarest).catch(() => setRarest(mk.rarest)),
          fetchVideoAttentionYear().then(setAttention).catch(() => setAttention(mk.attention)),
          fetchStudyRecsYear().then(setStudyRecs).catch(() => setStudyRecs(mk.studyRecs)),
          fetchPerseveranceYear().then(setPerseverance).catch(() => setPerseverance(mk.perseverance)),
          fetchTrainingShortestYear().then(setTrainShort).catch(() => setTrainShort(mk.trainShort)),
          fetchTrainingLongestYear().then(setTrainLong).catch(() => setTrainLong(mk.trainLong)),
          fetchTrainingSolvedTasksYear().then(setTrainTasks).catch(() => setTrainTasks(mk.trainTasks)),
          fetchTrainingPerfectStreakYear().then(setTrainStreak).catch(() => setTrainStreak(mk.trainStreak)),
          fetchTrainingStarterRankYear().then(setTrainRank).catch(() => setTrainRank(mk.trainRank)),
          fetchTrainingDifficultyStatsYear().then(setTrainDiff).catch(() => setTrainDiff(mk.trainDiff)),
          fetchTrainingTypeProfileYear().then(setTrainTypes).catch(() => setTrainTypes(mk.trainTypes)),
          fetchAwardBadgesCountYear().then(setBadgesCount).catch(() => setBadgesCount(mk.badgesCount)),
          fetchAwardBadgesLevelsYear().then(setBadgesLevels).catch(() => setBadgesLevels(mk.badgesLevels)),
          fetchAwardBadgesRarestYear().then(setBadgesRarest).catch(() => setBadgesRarest(mk.badgesRarest)),
          fetchPartialCreditShareYear().then(setPartialCredit).catch(() => setPartialCredit(mk.partialCredit)),
          fetchFirstVideoStarterYear().then(setFirstVideoStarter).catch(() => setFirstVideoStarter(mk.firstVideoStarter)),
          fetchSchoolVideoRankYear().then(setSchoolRank).catch(() => setSchoolRank(mk.schoolRank)),
          fetchRegionPointsRankYear().then(setRegionRank).catch(() => setRegionRank(mk.regionRank)),
          fetchClassTextDepthRankYear().then(setClassRank).catch(() => setClassRank(mk.classRank)),
          fetchHomeworkTasksYear().then(setHomeworkTasks).catch(() => setHomeworkTasks(mk.homeworkTasks)),
          fetchLessonTasksYear().then(setLessonTasks).catch(() => setLessonTasks(mk.lessonTasks)),
        ]

    Promise.allSettled(calls).finally(() => setLoading(false))
  }, [tab, resetData])

  // ── Share ──────────────────────────────────────────────────────────────
  const shareCardRef = useRef(null)
  const [copied, setCopied] = useState(false)

  function handleCopyLink() {
    navigator.clipboard.writeText(window.location.href).then(() => {
      setCopied(true)
      setTimeout(() => setCopied(false), 2000)
    })
  }

  async function handleDownload() {
    if (!shareCardRef.current) return
    const canvas = await html2canvas(shareCardRef.current, {
      backgroundColor: '#0d1230', scale: 2, useCORS: true,
    })
    const link = document.createElement('a')
    link.download = 'мой-учебный-год.png'
    link.href = canvas.toDataURL('image/png')
    link.click()
  }

  // ── Производные ────────────────────────────────────────────────────────
  const mediaPieData = media
    ? [
        { name: MEDIA_LABELS.live_lesson,        value: media.live_lesson_count,        pct: media.live_lesson_percentage },
        { name: MEDIA_LABELS.recorded_lesson,     value: media.recorded_lesson_count,    pct: media.recorded_lesson_percentage },
        { name: MEDIA_LABELS.prerecorded_lesson,  value: media.prerecorded_lesson_count, pct: media.prerecorded_lesson_percentage },
      ].filter((d) => d.value > 0)
    : []

  const rareLessonsList = rare ? Object.values(rare.rare_lessons) : []
  const attentionCurve  = attention ? makeAttentionCurve(attention.peak_position_percentage) : []

  const tabLabel = tab === 'year' ? 'за год' : `за ${tab} четверть`

  return (
    <>
    <StarField />
    <div className="report">

      {/* ══ НАВИГАЦИЯ ПО ЧЕТВЕРТЯМ ══════════════════════════════════════════ */}
      <nav className="tab-nav">
        <div className="tab-nav-inner">
          <span className="tab-nav-label">Четверть</span>
          <div className="tab-list">
            {TABS.map((t) => (
              <button
                key={t.id}
                className={`tab-btn${tab === t.id ? ' active' : ''}`}
                onClick={() => setTab(t.id)}
              >
                {t.label}
              </button>
            ))}
          </div>
        </div>
      </nav>

      {/* ══ HERO ════════════════════════════════════════════════════════════ */}
      <section className="hero">
        <div className="hero-glow" />
        <div className="hero-year">Цифриум · Итоговый отчёт · 2024–2025</div>
        <h1 className="hero-title">Твой учебный год</h1>
        <p className="hero-subtitle">{STUDENT.name}</p>
        <p className="hero-course">{STUDENT.course}</p>

        <div className="hero-stats">
          <div className="hero-stat">
            <div className="num">{points ? points.earned_count : '…'}</div>
            <div className="lbl">Баллов</div>
          </div>
          <div className="hero-stat">
            <div className="num">{videos ? videos.watched_count : '…'}</div>
            <div className="lbl">Видео</div>
          </div>
          <div className="hero-stat">
            <div className="num">{solved ? solved.full_solved_lessons_count : '…'}</div>
            <div className="lbl">Уроков решено</div>
          </div>
          <div className="hero-stat">
            <div className="num">{conspects ? conspects.learned_count : '…'}</div>
            <div className="lbl">Конспектов</div>
          </div>
        </div>

        {progress !== null && (
          <div className="hero-course-progress">
            <div className="hcp-label">
              {tab === 'year' ? 'Прогресс по курсу' : `${tab} четверть`}
            </div>
            <div className="hcp-bar-wrap">
              <div className="hcp-bar" style={{ width: `${progress}%` }} />
            </div>
            <div className="hcp-val">{progress}%</div>
          </div>
        )}

        <div className="hero-bottom-row">
          <span className="hero-scroll-hint">Листай вниз</span>
          <a href="#share-section" className="hero-share-btn">Поделиться →</a>
        </div>
      </section>

      <div className="divider" />

      {/* ══ 3D МОДЕЛИ ═══════════════════════════════════════════════════════ */}
      <section className="chapter">
        <div className="chapter-label">Твой путь</div>
        <h2 className="chapter-heading">
          {tab === 'year'
            ? <>Четыре <span className="highlight">этапа</span> учебного года</>
            : <><span className="highlight">{tab} четверть</span> — твой прогресс</>}
        </h2>
        <p className="chapter-desc">Крути модель мышью, смотри как ты рос.</p>

        {tab === 'year' ? (
          <div className="quarters-grid">
            {QUARTER_MODELS.map((model, i) => {
              const pct = quarterProgress[i]
              return (
                <div
                  key={i}
                  className="quarter-card"
                  style={{ cursor: 'pointer' }}
                  onClick={() => setTab(i + 1)}
                >
                  <div className="quarter-model-wrap">
                    <ModelViewer url={model} />
                  </div>
                  <div className="quarter-info">
                    <div className="quarter-label">{i + 1} четверть</div>
                    {pct !== null ? (
                      <>
                        <div className="quarter-progress-bar-wrap">
                          <div className="quarter-progress-bar" style={{ width: `${pct}%` }} />
                        </div>
                        <div className="quarter-pct">{pct}%</div>
                      </>
                    ) : (
                      <div className="quarter-loading">загрузка…</div>
                    )}
                  </div>
                </div>
              )
            })}
          </div>
        ) : (
          <div className="single-model-wrap">
            <ModelViewer url={QUARTER_MODELS[tab - 1]} />
            {(videos || points || conspects) && (
              <div className="tribar-wrap">
                <div className="tribar">
                  <div
                    className="tribar-segment"
                    data-tip={videos ? `Видео · ${videos.watched_count} / ${videos.total_count} (${Math.round(videos.watched_count / videos.total_count * 100)}%)` : 'Видео'}
                  >
                    <div className="tribar-segment-inner">
                      <div className="tribar-fill blue" style={{ width: videos ? `${Math.round(videos.watched_count / videos.total_count * 100)}%` : '0%' }} />
                    </div>
                  </div>
                  <div
                    className="tribar-segment"
                    data-tip={points ? `Баллы · ${points.earned_count} / ${points.total_count} (${Math.round(points.earned_count / points.total_count * 100)}%)` : 'Баллы'}
                  >
                    <div className="tribar-segment-inner">
                      <div className="tribar-fill purple" style={{ width: points ? `${Math.round(points.earned_count / points.total_count * 100)}%` : '0%' }} />
                    </div>
                  </div>
                  <div
                    className="tribar-segment"
                    data-tip={conspects ? `Конспекты · ${conspects.learned_count} / ${conspects.total_count} (${Math.round(conspects.learned_count / conspects.total_count * 100)}%)` : 'Конспекты'}
                  >
                    <div className="tribar-segment-inner">
                      <div className="tribar-fill green" style={{ width: conspects ? `${Math.round(conspects.learned_count / conspects.total_count * 100)}%` : '0%' }} />
                    </div>
                  </div>
                </div>
                <div className="tribar-labels">
                  <span style={{ color: '#4a9fff' }}>Видео</span>
                  <span style={{ color: '#9b5bff' }}>Баллы</span>
                  <span style={{ color: '#2dd4a0' }}>Конспекты</span>
                </div>
              </div>
            )}
          </div>
        )}

        {/* ── Компактные карточки метрик под моделью ── */}
        {(videos || points || conspects) && (
          <div className="model-stat-cards">
            {videos && (
              <div className="model-stat-card">
                <div className="model-stat-val" style={{ color: '#4a9fff' }}>
                  {videos.watched_count}<span className="model-stat-sep">/</span>{videos.total_count}
                </div>
                <div className="model-stat-lbl">Видео</div>
                <div className="model-stat-bar-wrap">
                  <div className="model-stat-bar" style={{ width: `${Math.round(videos.watched_count / videos.total_count * 100)}%`, background: 'linear-gradient(90deg,#4a9fff,#36c2cf)' }} />
                </div>
              </div>
            )}
            {points && (
              <div className="model-stat-card">
                <div className="model-stat-val" style={{ color: '#b06bff' }}>
                  {points.earned_count}<span className="model-stat-sep">/</span>{points.total_count}
                </div>
                <div className="model-stat-lbl">Баллы</div>
                <div className="model-stat-bar-wrap">
                  <div className="model-stat-bar" style={{ width: `${Math.round(points.earned_count / points.total_count * 100)}%`, background: 'linear-gradient(90deg,#9b5bff,#c47bff)' }} />
                </div>
              </div>
            )}
            {conspects && (
              <div className="model-stat-card">
                <div className="model-stat-val" style={{ color: '#2dd4a0' }}>
                  {conspects.learned_count}<span className="model-stat-sep">/</span>{conspects.total_count}
                </div>
                <div className="model-stat-lbl">Конспекты</div>
                <div className="model-stat-bar-wrap">
                  <div className="model-stat-bar" style={{ width: `${Math.round(conspects.learned_count / conspects.total_count * 100)}%`, background: 'linear-gradient(90deg,#2dd4a0,#4ade80)' }} />
                </div>
              </div>
            )}
          </div>
        )}
      </section>

      <div className="divider" />

      {/* ══ РЕЙТИНГИ ════════════════════════════════════════════════════════ */}
      {(schoolRank || regionRank || classRank) && (
        <section className="chapter">
          <div className="chapter-label">Твоё место</div>
          <h2 className="chapter-heading">
            Как ты <span className="highlight">выглядишь среди других</span>
          </h2>
          <p className="chapter-desc">Твоя позиция в школе, регионе и классе.</p>
          <div className="rank-cards-grid">
            {schoolRank && (
              <div className="rank-card rank-card--blue">
                <div className="rank-card-icon">🏫</div>
                <div className="rank-card-value">{schoolRank.display_value}</div>
                <div className="rank-card-label">место в школе</div>
                <div className="rank-card-sub">{schoolRank.school_name}</div>
                <div className="rank-card-bar-wrap">
                  <div className="rank-card-bar" style={{ width: `${Math.round(schoolRank.watched_lessons_count / schoolRank.total_video_lessons_count * 100)}%`, background: 'linear-gradient(90deg,#4a9fff,#36c2cf)' }} />
                </div>
                <div className="rank-card-text">{schoolRank.display_text}</div>
              </div>
            )}
            {regionRank && (
              <div className="rank-card rank-card--purple">
                <div className="rank-card-icon">🌍</div>
                <div className="rank-card-value">{regionRank.display_mode === 'top' ? `Топ ${regionRank.display_value}%` : regionRank.display_value}</div>
                <div className="rank-card-label">{regionRank.display_mode === 'top' ? 'в регионе' : 'место в регионе'}</div>
                <div className="rank-card-sub">{regionRank.region_name}</div>
                <div className="rank-card-bar-wrap">
                  <div className="rank-card-bar" style={{ width: `${Math.round(regionRank.earned_points / regionRank.total_points * 100)}%`, background: 'linear-gradient(90deg,#9b5bff,#c47bff)' }} />
                </div>
                <div className="rank-card-text">{regionRank.display_text}</div>
              </div>
            )}
            {classRank && (
              <div className="rank-card rank-card--teal">
                <div className="rank-card-icon">📚</div>
                <div className="rank-card-value">{classRank.display_value}</div>
                <div className="rank-card-label">место в классе</div>
                <div className="rank-card-sub">{classRank.class_name}</div>
                <div className="rank-card-bar-wrap">
                  <div className="rank-card-bar" style={{ width: `${Math.round(classRank.learned_count / classRank.total_materials_count * 100)}%`, background: 'linear-gradient(90deg,#2dd4a0,#4ade80)' }} />
                </div>
                <div className="rank-card-text">{classRank.display_text}</div>
              </div>
            )}
          </div>
        </section>
      )}

      <div className="divider" />

      {/* ══ ХРОНОТИП — ТАБЛИЦА (перенесён после 3D) ═══════════════════════ */}
      <section className="chapter">
        <div className="chapter-label">Твой ритм</div>
        <h2 className="chapter-heading">
          Твой <span className="highlight">космический хронотип</span>
        </h2>
        <p className="chapter-desc">
          В какое время суток ты чаще всего учишься — и что это говорит о тебе.
        </p>

        <div className="chrono-table">
          <div className="chrono-header">
            <span>Время</span>
            <span>Тип</span>
            <span>Позывной</span>
            <span>Инсайт</span>
          </div>
          {CHRONOTYPE_ROWS.filter((row) => !chrono || chrono.type === row.type).map((row) => {
            const isActive = chrono?.type === row.type
            return (
              <div key={row.type} className={`chrono-row${isActive ? ' chrono-active' : ''}`}>
                <span className="chrono-time">{row.time}</span>
                <span className="chrono-type">
                  <span className="chrono-icon">{row.icon}</span>
                  {row.type}
                </span>
                <span className="chrono-name">{row.report_name}</span>
                <span className="chrono-insight">
                  {isActive && chrono?.insight
                    ? chrono.insight
                    : <span className="chrono-dash">—</span>}
                </span>
              </div>
            )
          })}
        </div>

        {chrono && (
          <div className="chrono-meta">
            Пиковый час: <strong>{chrono.peak_time}</strong>
            &nbsp;·&nbsp; Действий в пике: <strong>{chrono.peak_actions_count}</strong>
            &nbsp;·&nbsp; Всего: <strong>{chrono.total_actions_count}</strong>
          </div>
        )}
      </section>

      <div className="divider" />

      {/* ══ ГЛАВА УРОКИ ══════════════════════════════════════════════════════ */}
      <section className="chapter">
        <div className="chapter-label">Уроки</div>
        <h2 className="chapter-heading">
          Твои <span className="highlight">уроки</span> {tabLabel}
        </h2>
        <p className="chapter-desc">Решённые уроки и где ты оказался сильнее других.</p>

        <div className="metrics-grid" style={{ gridTemplateColumns: attendance?.is_user_visited_all_lessons ? '1fr 1fr' : '1fr' }}>
          <div className="metric-card accent-orange">
            <div className="metric-label">Уроков решено полностью</div>
            <div className="metric-value orange" style={{ fontSize: 36 }}>
              {solved ? solved.full_solved_lessons_count : '…'}
            </div>
            <div className="metric-unit">уроков с выполненными задачами</div>
          </div>

          {attendance?.is_user_visited_all_lessons && (
            <div className="metric-card" style={{ borderColor: 'rgba(62,207,178,0.3)', background: 'linear-gradient(135deg,rgba(62,207,178,0.08),rgba(62,207,178,0.02))' }}>
              <div className="metric-label">Достижение</div>
              <div style={{ fontSize: 36, margin: '8px 0' }}>⭐</div>
              <div className="metric-unit" style={{ fontWeight: 700, color: 'var(--accent-teal)', fontSize: 15 }}>Без прогулов</div>
              <div className="metric-unit">Ты посетил все уроки</div>
            </div>
          )}
        </div>

        {rareLessonsList.length > 0 && (
          <>
            <h3 className="subsection-heading" style={{ marginTop: 28, marginBottom: 12 }}>
              Уроки, где ты <span className="highlight">оказался сильнее</span>
            </h3>
            <p className="chapter-desc" style={{ marginBottom: 16 }}>
              Ты решил эти уроки — а большинство учеников не справились.
            </p>
            <div className="rare-lessons-table">
              <div className="rare-lessons-header">
                <span>Урок (ТОП-3)</span>
                <span>Статус</span>
                <span>Глобальная сложность</span>
              </div>
              {rareLessonsList.map((lesson, i) => (
                <div key={i} className="rare-lesson-row">
                  <span className="rare-lesson-title">{lesson.title}</span>
                  <span className="rare-lesson-badge">Решён ✅</span>
                  <span className="rare-lesson-pct">
                    {lesson.not_solved_percentage}% не справились
                  </span>
                </div>
              ))}
            </div>
          </>
        )}

        {/* Самый редкий урок — внутри главы Уроки */}
        {rarest && (
          <>
            <h3 className="subsection-heading" style={{ marginTop: 28, marginBottom: 8 }}>
              {rarest.report_name}
            </h3>
            <p className="chapter-desc" style={{ marginBottom: 16 }}>
              Этот урок посетили лишь единицы. Ты — один из них.
            </p>
            <div className="rarest-card">
              <div className="rarest-title">{rarest.title}</div>
              <div className="rarest-stat">
                Посетили:{' '}
                <strong>{rarest.visited_users_count} из {rarest.total_users_count}</strong>{' '}
                учеников — <strong>{rarest.visited_percentage}%</strong>
              </div>
              <p className="rarest-body">{rarest.insight}</p>
            </div>
          </>
        )}
      </section>

      <div className="divider" />

      {/* ══ МЕДИАКОНТЕНТ ════════════════════════════════════════════════════ */}
      <section className="chapter">
        <div className="chapter-label">Медиаконтент</div>
        <h2 className="chapter-heading">
          Какой контент ты <span className="highlight-teal">смотрел</span>
        </h2>
        <p className="chapter-desc">
          Соотношение прямых эфиров, записей и предзаписанных уроков.
        </p>

        <div className="chart-box">
          {media && mediaPieData.length > 0 ? (
            <>
              <div className="metric-label" style={{ marginBottom: 12 }}>
                Всего просмотрено: {media.total_watched_videos_cnt} видео
              </div>
              <ResponsiveContainer width="100%" height={340}>
                <PieChart margin={{ top: 24, right: 24, bottom: 8, left: 24 }}>
                  <Pie
                    data={mediaPieData}
                    cx="50%"
                    cy="50%"
                    innerRadius={72}
                    outerRadius={112}
                    paddingAngle={3}
                    dataKey="value"
                    label={({ pct }) => `${pct}%`}
                    labelLine={true}
                  >
                    {mediaPieData.map((_, idx) => (
                      <Cell key={idx} fill={MEDIA_COLORS[idx % MEDIA_COLORS.length]} />
                    ))}
                  </Pie>
                  <Tooltip formatter={(v, name) => [`${v} видео`, name]} />
                  <Legend />
                </PieChart>
              </ResponsiveContainer>
            </>
          ) : (
            <div className="chart-placeholder">
              <span className="chart-icon">🍩</span>
              <span>{loading ? 'Загрузка…' : 'Нет данных'}</span>
            </div>
          )}
        </div>

        {/* Первый старт просмотра видео */}
        {firstVideoStarter?.is_user_first_video_starter && (
          <div className="insight-card wide" style={{ marginTop: 20, background: 'linear-gradient(135deg,rgba(255,200,50,0.08),rgba(255,160,0,0.04))', borderColor: 'rgba(255,200,50,0.25)' }}>
            <div className="insight-emoji">⭐</div>
            <div className="insight-title" style={{ background: 'linear-gradient(135deg,#ffd700,#ffaa00)', WebkitBackgroundClip: 'text', WebkitTextFillColor: 'transparent', backgroundClip: 'text' }}>
              Первый старт
            </div>
            <div className="insight-body">
              Ты самый первый приступил к просмотру видео в этом периоде — раньше всех в своём потоке.
            </div>
          </div>
        )}

        {/* Профиль внимания — внутри Медиаконтента */}
        {attention ? (
          <>
            <h3 className="subsection-heading" style={{ marginTop: 28, marginBottom: 8 }}>
              Как ты <span className="highlight-warm">смотришь видео</span>
            </h3>
            <p className="chapter-desc" style={{ marginBottom: 16 }}>
              По сегментам видео определяем, какое время просмотра самое частое.
            </p>
            <div className="chart-box">
              <div className="metric-label" style={{ marginBottom: 16 }}>
                Частота просмотра по таймлайну видео
              </div>
              <ResponsiveContainer width="100%" height={220}>
                <LineChart data={attentionCurve} margin={{ top: 4, right: 16, bottom: 20, left: -20 }}>
                  <defs>
                    <linearGradient id="attentionGrad2" x1="0" y1="0" x2="1" y2="0">
                      <stop offset="0%"   stopColor="#6b8fff" />
                      <stop offset="100%" stopColor="#b06bff" />
                    </linearGradient>
                  </defs>
                  <CartesianGrid strokeDasharray="3 3" stroke="rgba(255,255,255,0.05)" />
                  <XAxis dataKey="pos" tick={{ fill: '#7a8299', fontSize: 12 }} tickLine={false} axisLine={false}
                    label={{ value: 'Таймлайн видео', position: 'insideBottom', offset: -12, fill: '#4a5470', fontSize: 12 }} />
                  <YAxis tick={{ fill: '#7a8299', fontSize: 12 }} tickLine={false} axisLine={false}
                    label={{ value: 'Частота', angle: -90, position: 'insideLeft', offset: 12, fill: '#4a5470', fontSize: 12 }} />
                  <Tooltip contentStyle={{ background: '#111520', border: '1px solid #1e2a45', borderRadius: 10 }}
                    labelStyle={{ color: '#7a8299' }} itemStyle={{ color: '#6b8fff' }}
                    formatter={(v) => [v, 'Частота просмотра']} />
                  <Line type="monotone" dataKey="freq" stroke="url(#attentionGrad2)" strokeWidth={3} dot={false} activeDot={{ r: 5, fill: '#6b8fff' }} />
                </LineChart>
              </ResponsiveContainer>
            </div>
            <div className="insight-card" style={{ marginTop: 12 }}>
              <div className="insight-emoji">🎬</div>
              <div className="insight-title">{attention.report_name}</div>
              <div style={{ color: 'var(--text-muted)', marginBottom: 8, fontSize: 13 }}>
                Роль: <strong style={{ color: '#fff' }}>{attention.role}</strong>
                &nbsp;·&nbsp;
                Пик на: <strong style={{ color: '#fff' }}>{attention.peak_position_percentage}%</strong> видео
              </div>
              <div className="insight-body">{attention.insight}</div>
            </div>
          </>
        ) : null}
      </section>

      <div className="divider" />

      {/* ══ ЗАДАНИЯ ═════════════════════════════════════════════════════════ */}
      <section className="chapter">
        <div className="chapter-label">Задания</div>
        <h2 className="chapter-heading">
          Твой <span className="highlight">учебный фокус</span> {tabLabel}
        </h2>
        <p className="chapter-desc">На что сделать упор, чтобы поднять средний балл.</p>

        {(homeworkTasks || lessonTasks) && (
          <div className="tasks-metric" style={{ marginBottom: 24 }}>
            <div className="tasks-metric-label">Решено задач</div>
            <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'center', gap: 16, margin: '8px 0' }}>
              {homeworkTasks && (
                <div style={{ textAlign: 'center' }}>
                  <div className="tasks-metric-value" style={{ color: 'var(--accent-blue)' }}>
                    {homeworkTasks.solved_homework_tasks_count}
                  </div>
                  <div style={{ fontSize: 11, color: 'var(--text-muted)', marginTop: 2 }}>Homework</div>
                </div>
              )}
              {homeworkTasks && lessonTasks && (
                <div style={{ fontSize: 32, color: 'var(--border)', fontWeight: 300, lineHeight: 1 }}>/</div>
              )}
              {lessonTasks && (
                <div style={{ textAlign: 'center' }}>
                  <div className="tasks-metric-value" style={{ color: 'var(--accent-teal)' }}>
                    {lessonTasks.solved_lesson_tasks_count}
                  </div>
                  <div style={{ fontSize: 11, color: 'var(--text-muted)', marginTop: 2 }}>Lesson</div>
                </div>
              )}
            </div>
            <div className="tasks-metric-unit">задач решено</div>
          </div>
        )}

        {studyRecs && (
          <>
            <div className="tasks-grid" style={{ gridTemplateColumns: '1fr 1fr', marginBottom: 20 }}>
              <div className="tasks-metric">
                <div className="tasks-metric-label">Текущий средний балл</div>
                <div className="tasks-metric-value" style={{ color: 'var(--accent-blue)' }}>
                  {studyRecs.current_average_score}
                </div>
                <div className="tasks-metric-unit">из 5.0</div>
              </div>
              <div className="tasks-metric">
                <div className="tasks-metric-label">Цель при правильном фокусе</div>
                <div className="tasks-metric-value" style={{ color: 'var(--accent-teal)' }}>
                  {studyRecs.target_average_score}
                </div>
                <div className="tasks-metric-unit">
                  фокус: <strong style={{ color: '#fff' }}>{studyRecs.recommended_focus_title}</strong>
                </div>
              </div>
            </div>

            <div className="study-recs-table" style={{ marginBottom: 20 }}>
              <div className="study-recs-row study-recs-head">
                <span>Параметр</span>
                <span>Твой уровень</span>
                <span>Влияние на балл</span>
              </div>
              {['video', 'conspects', 'materials'].map((key) => {
                const item = studyRecs[key]
                return (
                  <div key={key} className="study-recs-row">
                    <span className="rare-lesson-title">{item.title}</span>
                    <span className="study-level-cell">
                      <div className="study-bar-track">
                        <div className="study-bar-fill" style={{ width: `${item.level_percentage}%` }} />
                      </div>
                      <span className="study-level-pct">{item.level_percentage}%</span>
                    </span>
                    <span style={{ color: 'var(--text-muted)', fontSize: 13, whiteSpace: 'nowrap' }}>
                      {item.influence}
                    </span>
                  </div>
                )
              })}
            </div>

            <div className="insight-card wide">
              <div className="insight-emoji">🤖</div>
              <div className="insight-title">AI-анализ</div>
              <div className="insight-body" style={{ fontSize: 15, lineHeight: 1.7 }}>{studyRecs.ai_insight}</div>
            </div>
          </>
        )}

        {tab === 'year' && perseverance && (
          <>
            <div className="tasks-metric" style={{ marginTop: 16, marginBottom: 16 }}>
              <div className="tasks-metric-label">Упорство в задачах</div>
              <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'center', gap: 16, margin: '8px 0' }}>
                <div style={{ textAlign: 'center' }}>
                  <div className="tasks-metric-value" style={{ color: 'var(--accent-teal)' }}>
                    +{perseverance.extra_points_from_retries}
                  </div>
                  <div style={{ fontSize: 11, color: 'var(--text-muted)', marginTop: 2 }}>доп. баллов</div>
                </div>
                <div style={{ fontSize: 32, color: 'var(--border)', fontWeight: 300, lineHeight: 1 }}>/</div>
                <div style={{ textAlign: 'center' }}>
                  <div className="tasks-metric-value" style={{ color: 'var(--accent-blue)' }}>
                    {perseverance.repeated_tasks_count}
                  </div>
                  <div style={{ fontSize: 11, color: 'var(--text-muted)', marginTop: 2 }}>повторных попыток</div>
                </div>
              </div>
              <div className="tasks-metric-unit">за повторные решения задач</div>
            </div>
            <div className="insight-card wide">
              <div className="insight-emoji">🔁</div>
              <div className="insight-title">{perseverance.report_name}</div>
              <div className="insight-body">{perseverance.insight}</div>
            </div>
          </>
        )}

        {tab === 'year' && partialCredit && (
          <div className="insight-card wide" style={{ marginTop: 16 }}>
            <div className="insight-emoji">🪙</div>
            <div className="insight-title">{partialCredit.report_name}</div>
            <div style={{ color: 'var(--text-muted)', fontSize: 14, marginBottom: 10 }}>
              За частичные ответы получено{' '}
              <strong style={{ color: 'var(--accent-teal)', fontSize: 20 }}>
                {partialCredit.partial_points_percentage}%
              </strong>{' '}
              всех баллов ({partialCredit.partial_points} из {partialCredit.total_points})
            </div>
            <div className="insight-body">{partialCredit.insight}</div>
          </div>
        )}
      </section>



      <div className="divider" />

      {/* ══ ГЛАВА 8: ТРЕНИНГИ ═══════════════════════════════════════════════ */}
      <section className="chapter">
        <div className="chapter-label">Тренинги</div>
        <h2 className="chapter-heading">
          Твои <span className="highlight">тренинги</span> {tabLabel}
        </h2>
        <p className="chapter-desc">Скорость, точность и серии без ошибок.</p>

        <div className="tasks-grid">
          <div className="tasks-metric">
            <div className="tasks-metric-label">Самый короткий</div>
            <div className="tasks-metric-value" style={{ color: 'var(--accent-teal)', fontSize: 28 }}>
              {trainShort ? trainShort.duration_human : '—'}
            </div>
            <div className="tasks-metric-unit" style={{ fontSize: 11 }}>
              {trainShort ? trainShort.training_name : ''}
            </div>
          </div>
          <div className="tasks-metric">
            <div className="tasks-metric-label">Самый длинный</div>
            <div className="tasks-metric-value" style={{ color: 'var(--accent-blue)', fontSize: 28 }}>
              {trainLong ? trainLong.duration_human : '—'}
            </div>
            <div className="tasks-metric-unit" style={{ fontSize: 11 }}>
              {trainLong ? trainLong.training_name : ''}
            </div>
          </div>
          <div className="tasks-metric">
            <div className="tasks-metric-label">Задач в тренингах</div>
            <div className="tasks-metric-value" style={{ color: 'var(--accent-purple)' }}>
              {trainTasks ? trainTasks.solved_tasks_count : '—'}
            </div>
            <div className="tasks-metric-unit">
              в {trainTasks ? trainTasks.trainings_count : '—'} тренингах
            </div>
          </div>
        </div>

        {trainStreak && (
          <div className="insight-card" style={{ marginTop: 16 }}>
            <div className="insight-emoji">🎯</div>
            <div className="insight-title">{trainStreak.report_name}</div>
            <div className="insight-body">{trainStreak.insight}</div>
          </div>
        )}

        {trainRank && (trainRank.is_user_first_starter || trainRank.is_user_top3_starter) && (
          <div className="insight-card" style={{ marginTop: 12 }}>
            <div className="insight-emoji">🚀</div>
            <div className="insight-title">
              {trainRank.is_user_first_starter ? 'Первый начал тренинг!' : `Топ-3 по старту (${trainRank.user_rank} место)`}
            </div>
            <div className="insight-body">
              В тренинге «{trainRank.training_name}» ты стартовал раньше других.
            </div>
          </div>
        )}

        {trainDiff && (
          <div style={{ marginTop: 16 }}>
            <div className="metric-label" style={{ marginBottom: 10 }}>Сложность тренингов</div>
            <div style={{ display: 'flex', gap: 12 }}>
              {[['Лёгкие', trainDiff.difficulty_stats.easy_count, 'var(--accent-teal)'],
                ['Средние', trainDiff.difficulty_stats.medium_count, 'var(--accent-blue)'],
                ['Сложные', trainDiff.difficulty_stats.hard_count, 'var(--accent-purple)']].map(([label, val, color]) => (
                <div key={label} style={{ background: 'var(--bg-card)', border: '1px solid var(--border)', borderRadius: 12, padding: '12px 16px', flex: 1, textAlign: 'center' }}>
                  <div style={{ fontSize: 28, fontWeight: 700, color }}>{val}</div>
                  <div style={{ fontSize: 12, color: 'var(--text-muted)' }}>{label}</div>
                </div>
              ))}
            </div>
          </div>
        )}

        {tab === 'year' && trainTypes && (
          <>
            <div className="chart-box" style={{ marginTop: 16 }}>
              <div className="metric-label" style={{ marginBottom: 8 }}>Профиль тренингов</div>
              <div style={{ display: 'flex', alignItems: 'center', gap: 24, flexWrap: 'wrap' }}>
                <ResponsiveContainer width={200} height={200}>
                  <PieChart>
                    <Pie
                      data={[
                        { name: 'Курсовые', value: trainTypes.lesson_training_percentage || 0, color: 'var(--accent-blue)' },
                        { name: 'Независимые', value: trainTypes.regular_training_percentage || 0, color: 'var(--accent-purple)' },
                        { name: 'Олимпиадные', value: trainTypes.olympiad_training_percentage || 0, color: 'var(--accent-orange)' },
                      ].filter(d => d.value > 0)}
                      dataKey="value"
                      cx="50%"
                      cy="50%"
                      outerRadius={80}
                      innerRadius={36}
                      paddingAngle={2}
                      label={({ name, value }) => `${value}%`}
                      labelLine={false}
                    >
                      {[
                        { color: 'var(--accent-blue)' },
                        { color: 'var(--accent-purple)' },
                        { color: 'var(--accent-orange)' },
                      ].map((entry, idx) => (
                        <Cell key={idx} fill={entry.color} />
                      ))}
                    </Pie>
                    <Tooltip
                      contentStyle={{ background: '#111520', border: '1px solid #1e2a45', borderRadius: 10 }}
                      formatter={(v, name) => [`${v}%`, name]}
                    />
                  </PieChart>
                </ResponsiveContainer>
                <div style={{ display: 'flex', flexDirection: 'column', gap: 10 }}>
                  {[
                    ['Курсовые (Lesson)', trainTypes.lesson_training_percentage, trainTypes.lesson_training_count, 'var(--accent-blue)'],
                    ['Независимые (Regular)', trainTypes.regular_training_percentage, trainTypes.regular_training_count, 'var(--accent-purple)'],
                    ['Олимпиадные', trainTypes.olympiad_training_percentage, trainTypes.olympiad_training_count, 'var(--accent-orange)'],
                  ].map(([label, pct, cnt, color]) => (
                    <div key={label} style={{ display: 'flex', alignItems: 'center', gap: 10, fontSize: 14 }}>
                      <div style={{ width: 12, height: 12, borderRadius: '50%', background: color, flexShrink: 0 }} />
                      <span style={{ color: 'var(--text-muted)' }}>{label}:</span>
                      <strong style={{ color }}>{pct}%</strong>
                      <span style={{ color: 'var(--text-muted)', fontSize: 12 }}>({cnt} шт.)</span>
                    </div>
                  ))}
                </div>
              </div>
            </div>

            {trainTypes.has_olympiad_training && (
              <div className="insight-card wide" style={{ marginTop: 12 }}>
                <div className="insight-emoji">🏆</div>
                <div className="insight-title">{trainTypes.report_name}</div>
                <div className="insight-body">{trainTypes.insight}</div>
              </div>
            )}
          </>
        )}
      </section>

      <div className="divider" />

      {/* ══ ГЛАВА 9: ДОСТИЖЕНИЯ ═════════════════════════════════════════════ */}
      <section className="chapter">
        <div className="chapter-label">Достижения</div>
        <h2 className="chapter-heading">
          Твои <span className="highlight-warm">достижения</span> {tabLabel}
        </h2>
        <p className="chapter-desc">Награды, уровни и самые редкие бейджи.</p>

        <div className="tasks-grid" style={{ gridTemplateColumns: '1fr 1fr', marginBottom: 20 }}>
          <div className="tasks-metric">
            <div className="tasks-metric-label">Всего достижений</div>
            <div className="tasks-metric-value" style={{ color: 'var(--accent-orange)' }}>
              {badgesCount ? badgesCount.award_badges_count : '—'}
            </div>
            <div className="tasks-metric-unit">наград получено</div>
          </div>
          <div className="tasks-metric">
            <div className="tasks-metric-label">Специальных наград</div>
            <div className="tasks-metric-value" style={{ color: 'var(--accent-pink)' }}>
              {badgesLevels ? badgesLevels.award_badges_levels.special_count : '—'}
            </div>
            <div className="tasks-metric-unit">редкая категория</div>
          </div>
        </div>

        {badgesLevels && (
          <div className="chart-box" style={{ marginBottom: 20 }}>
            <div className="metric-label" style={{ marginBottom: 12 }}>Уровни наград</div>
            <ResponsiveContainer width="100%" height={160}>
              <BarChart
                data={[1,2,3,4,5].map((lvl) => ({
                  level: `Уровень ${lvl}`,
                  count: badgesLevels.award_badges_levels[`level_${lvl}_count`] || 0,
                }))}
                margin={{ top: 4, right: 8, bottom: 4, left: -20 }}
              >
                <CartesianGrid strokeDasharray="3 3" stroke="rgba(255,255,255,0.05)" />
                <XAxis dataKey="level" tick={{ fill: '#7a8299', fontSize: 11 }} tickLine={false} axisLine={false} />
                <YAxis tick={{ fill: '#7a8299', fontSize: 11 }} tickLine={false} axisLine={false} allowDecimals={false} />
                <Tooltip
                  contentStyle={{ background: '#111520', border: '1px solid #1e2a45', borderRadius: 10 }}
                  formatter={(v) => [v, 'Наград']}
                />
                <Bar dataKey="count" fill="var(--accent-orange)" radius={[6,6,0,0]} />
              </BarChart>
            </ResponsiveContainer>
          </div>
        )}

        {tab === 'year' && badgesRarest && badgesRarest.rarest_award_badges?.length > 0 && (
          <>
            <div className="metric-label" style={{ marginBottom: 14 }}>Самые редкие награды</div>
            <div style={{ display: 'flex', gap: 16, flexWrap: 'wrap' }}>
              {badgesRarest.rarest_award_badges.map((badge) => (
                <div key={badge.badge_id} style={{ background: 'var(--bg-card)', border: '1px solid var(--border)', borderRadius: 16, padding: '18px 20px', display: 'flex', flexDirection: 'column', alignItems: 'center', gap: 8, minWidth: 130 }}>
                  <img src={badge.image_url} alt={badge.title} style={{ width: 52, height: 52, objectFit: 'contain' }} />
                  <div style={{ fontSize: 13, fontWeight: 600, textAlign: 'center', color: 'var(--text)' }}>{badge.title}</div>
                  <div style={{ fontSize: 11, color: 'var(--accent-orange)', fontWeight: 600 }}>
                    {badge.owners_percentage}% учеников
                  </div>
                </div>
              ))}
            </div>
          </>
        )}

      </section>

      <div className="divider" />

      {/* ══ SHARE CARD ══════════════════════════════════════════════════════ */}
      <section className="share-section-wrap" id="share-section">
        <div className="chapter-label" style={{ marginBottom: 12 }}>Поделиться</div>
        <h2 className="chapter-heading" style={{ marginBottom: 8 }}>
          Покажи свои <span className="highlight">результаты</span>
        </h2>
        <p className="chapter-desc">Карточка с твоими главными достижениями — готова к публикации.</p>

        <div className="share-card" ref={shareCardRef}>
          <div className="share-card-glow" />
          <div className="share-card-content">

            {/* Шапка */}
            <div className="share-header">
              <div className="share-logo">Цифриум</div>
              <div className="share-user-pill">
                <span className="share-user-icon">👤</span>
                {STUDENT.name}
              </div>
            </div>

            {/* Прогресс */}
            <div className="share-progress-block">
              <div className="share-progress-pct">{progress !== null ? `${progress}%` : '—'}</div>
              <div className="share-progress-label">
                {tab === 'year' ? 'Прогресс за год' : `Прогресс — ${tab} четверть`}
              </div>
              <div className="share-progress-bar-wrap">
                <div className="share-progress-bar-fill" style={{ width: `${progress ?? 0}%` }} />
              </div>
            </div>

            <div className="share-divider" />

            {/* 3 метрики */}
            <div className="share-metrics">
              <div className="share-metric-item">
                <div className="share-metric-val">{points ? points.earned_count : '—'}</div>
                <div className="share-metric-lbl">Баллов</div>
              </div>
              <div className="share-metric-item">
                <div className="share-metric-val">{videos ? videos.watched_count : '—'}</div>
                <div className="share-metric-lbl">Видео</div>
              </div>
              <div className="share-metric-item">
                <div className="share-metric-val">{solved ? solved.full_solved_lessons_count : '—'}</div>
                <div className="share-metric-lbl">Уроков решено</div>
              </div>
              <div className="share-metric-item">
                <div className="share-metric-val">{badgesCount ? badgesCount.award_badges_count : '—'}</div>
                <div className="share-metric-lbl">Достижений</div>
              </div>
            </div>

            <div className="share-divider" />

            {/* Бейджи */}
            {tab === 'year' && badgesRarest?.rarest_award_badges?.length > 0 && (
              <div className="share-badges-row">
                {badgesRarest.rarest_award_badges.slice(0, 3).map((b) => (
                  <img key={b.badge_id} src={b.image_url} alt={b.title} className="share-badge-img" />
                ))}
              </div>
            )}

            <div className="share-phrase">
              Учебный год — это не просто цифры.{chrono ? ` Ты ${chrono.report_name}.` : ''}
            </div>
          </div>
        </div>

        <div className="share-actions" style={{ marginTop: 20 }}>
          <button className="btn-primary" onClick={handleDownload}>Скачать карточку</button>
          <button className="btn-outline" onClick={handleCopyLink}>
            {copied ? 'Скопировано!' : 'Скопировать ссылку'}
          </button>
        </div>
      </section>

    </div>
    </>
  )
}
