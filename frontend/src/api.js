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
