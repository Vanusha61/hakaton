const USER_ID = 665767
const COURSE_ID = 755

const BASE = '/api/v1'

async function get(path) {
  const r = await fetch(path)
  if (!r.ok) throw new Error('API error')
  return r.json()
}

const q = (params) =>
  Object.entries(params)
    .map(([k, v]) => `${k}=${v}`)
    .join('&')

const uid = { user_id: USER_ID, course_id: COURSE_ID }

// ── Progress ──────────────────────────────────────────────────────────────────
export const fetchQuarterProgress = (quarter) =>
  get(`${BASE}/progress/quarter?${q({ quarter, ...uid })}`)

export const fetchCourseProgress = () =>
  get(`${BASE}/progress/course?${q(uid)}`)

// ── Videos ───────────────────────────────────────────────────────────────────
export const fetchVideosQuarter = (quarter) =>
  get(`${BASE}/videos/quarter?${q({ quarter, ...uid })}`)

export const fetchVideosYear = () =>
  get(`${BASE}/videos/year?${q(uid)}`)

// ── Points ───────────────────────────────────────────────────────────────────
export const fetchPointsQuarter = (quarter) =>
  get(`${BASE}/points/quarter?${q({ quarter, ...uid })}`)

export const fetchPointsYear = () =>
  get(`${BASE}/points/year?${q(uid)}`)

// ── Conspects ────────────────────────────────────────────────────────────────
export const fetchConspectsQuarter = (quarter) =>
  get(`${BASE}/conspects/quarter?${q({ quarter, ...uid })}`)

export const fetchConspectsYear = () =>
  get(`${BASE}/conspects/year?${q(uid)}`)

// ── Media ────────────────────────────────────────────────────────────────────
export const fetchMediaQuarter = (quarter) =>
  get(`${BASE}/media/quarter?${q({ quarter, ...uid })}`)

export const fetchMediaYear = () =>
  get(`${BASE}/media/year?${q(uid)}`)

// ── Chronotype ───────────────────────────────────────────────────────────────
export const fetchChronotypeQuarter = (quarter) =>
  get(`${BASE}/chronotype/quarter?${q({ quarter, ...uid })}`)

export const fetchChronotypeYear = () =>
  get(`${BASE}/chronotype/year?${q(uid)}`)

// ── Solved lessons ────────────────────────────────────────────────────────────
export const fetchSolvedLessonsQuarter = (quarter) =>
  get(`${BASE}/solved_lessons/quarter?${q({ quarter, ...uid })}`)

export const fetchSolvedLessonsYear = () =>
  get(`${BASE}/solved_lessons/year?${q(uid)}`)

// ── Attendance ───────────────────────────────────────────────────────────────
export const fetchAttendanceQuarter = (quarter) =>
  get(`${BASE}/attendance/quarter?${q({ quarter, ...uid })}`)

export const fetchAttendanceYear = () =>
  get(`${BASE}/attendance/year?${q(uid)}`)

// ── Rare lessons (топ-3 сложных) ─────────────────────────────────────────────
export const fetchRareLessonsQuarter = (quarter) =>
  get(`${BASE}/rare_lessons/quarter?${q({ quarter, ...uid })}`)

export const fetchRareLessonsYear = () =>
  get(`${BASE}/rare_lessons/year?${q(uid)}`)

// ── Rarest visited lesson ────────────────────────────────────────────────────
export const fetchRarestLessonQuarter = (quarter) =>
  get(`${BASE}/rarest_visited_lesson/quarter?${q({ quarter, ...uid })}`)

export const fetchRarestLessonYear = () =>
  get(`${BASE}/rarest_visited_lesson/year?${q(uid)}`)

// ── First video starter ───────────────────────────────────────────────────────
export const fetchFirstVideoStarterQuarter = (quarter) =>
  get(`${BASE}/first_video_starter/quarter?${q({ quarter, ...uid })}`)

export const fetchFirstVideoStarterYear = () =>
  get(`${BASE}/first_video_starter/year?${q(uid)}`)

// ── Video attention ───────────────────────────────────────────────────────────
export const fetchVideoAttentionQuarter = (quarter) =>
  get(`${BASE}/video_attention/quarter?${q({ quarter, ...uid })}`)

export const fetchVideoAttentionYear = () =>
  get(`${BASE}/video_attention/year?${q(uid)}`)

// ── Training insights ─────────────────────────────────────────────────────
export const fetchTrainingShortestQuarter = (quarter) =>
  get(`${BASE}/training_shortest/quarter?${q({ quarter, ...uid })}`)
export const fetchTrainingShortestYear = () =>
  get(`${BASE}/training_shortest/year?${q(uid)}`)

export const fetchTrainingLongestQuarter = (quarter) =>
  get(`${BASE}/training_longest/quarter?${q({ quarter, ...uid })}`)
export const fetchTrainingLongestYear = () =>
  get(`${BASE}/training_longest/year?${q(uid)}`)

export const fetchTrainingSolvedTasksQuarter = (quarter) =>
  get(`${BASE}/training_solved_tasks/quarter?${q({ quarter, ...uid })}`)
export const fetchTrainingSolvedTasksYear = () =>
  get(`${BASE}/training_solved_tasks/year?${q(uid)}`)

export const fetchTrainingPerfectStreakQuarter = (quarter) =>
  get(`${BASE}/training_perfect_streak/quarter?${q({ quarter, ...uid })}`)
export const fetchTrainingPerfectStreakYear = () =>
  get(`${BASE}/training_perfect_streak/year?${q(uid)}`)

export const fetchTrainingStarterRankQuarter = (quarter) =>
  get(`${BASE}/training_starter_rank/quarter?${q({ quarter, ...uid })}`)
export const fetchTrainingStarterRankYear = () =>
  get(`${BASE}/training_starter_rank/year?${q(uid)}`)

export const fetchTrainingDifficultyStatsQuarter = (quarter) =>
  get(`${BASE}/training_difficulty_stats/quarter?${q({ quarter, ...uid })}`)
export const fetchTrainingDifficultyStatsYear = () =>
  get(`${BASE}/training_difficulty_stats/year?${q(uid)}`)

export const fetchTrainingTypeProfileYear = () =>
  get(`${BASE}/training_type_profile/year?${q(uid)}`)

// ── Award badges ──────────────────────────────────────────────────────────
export const fetchAwardBadgesCountQuarter = (quarter) =>
  get(`${BASE}/award_badges_count/quarter?${q({ quarter, ...uid })}`)
export const fetchAwardBadgesCountYear = () =>
  get(`${BASE}/award_badges_count/year?${q(uid)}`)

export const fetchAwardBadgesLevelsQuarter = (quarter) =>
  get(`${BASE}/award_badges_levels/quarter?${q({ quarter, ...uid })}`)
export const fetchAwardBadgesLevelsYear = () =>
  get(`${BASE}/award_badges_levels/year?${q(uid)}`)

export const fetchAwardBadgesRarestYear = () =>
  get(`${BASE}/award_badges_rarest/year?${q(uid)}`)

// ── Partial credit share (year only) ─────────────────────────────────────
export const fetchPartialCreditShareYear = () =>
  get(`${BASE}/partial_credit_share/year?${q(uid)}`)

// ── Study recommendations ─────────────────────────────────────────────────
export const fetchStudyRecsQuarter = (quarter) =>
  get(`${BASE}/study_recommendations/quarter?${q({ quarter, ...uid })}`)

export const fetchStudyRecsYear = () =>
  get(`${BASE}/study_recommendations/year?${q(uid)}`)

// ── Perseverance (только year) ────────────────────────────────────────────
export const fetchPerseveranceYear = () =>
  get(`${BASE}/perseverance/year?${q(uid)}`)

// ── Task counts ───────────────────────────────────────────────────────────
export const fetchHomeworkTasksQuarter = (quarter) =>
  get(`${BASE}/homework_tasks/quarter?${q({ quarter, ...uid })}`)
export const fetchHomeworkTasksYear = () =>
  get(`${BASE}/homework_tasks/year?${q(uid)}`)

export const fetchLessonTasksQuarter = (quarter) =>
  get(`${BASE}/lesson_tasks/quarter?${q({ quarter, ...uid })}`)
export const fetchLessonTasksYear = () =>
  get(`${BASE}/lesson_tasks/year?${q(uid)}`)

// ── Social rankings ───────────────────────────────────────────────────────
export const fetchSchoolVideoRankQuarter = (quarter) =>
  get(`${BASE}/school_video_rank/quarter?${q({ quarter, ...uid })}`)
export const fetchSchoolVideoRankYear = () =>
  get(`${BASE}/school_video_rank/year?${q(uid)}`)

export const fetchRegionPointsRankQuarter = (quarter) =>
  get(`${BASE}/region_points_rank/quarter?${q({ quarter, ...uid })}`)
export const fetchRegionPointsRankYear = () =>
  get(`${BASE}/region_points_rank/year?${q(uid)}`)

export const fetchClassTextDepthRankQuarter = (quarter) =>
  get(`${BASE}/class_text_depth_rank/quarter?${q({ quarter, ...uid })}`)
export const fetchClassTextDepthRankYear = () =>
  get(`${BASE}/class_text_depth_rank/year?${q(uid)}`)
