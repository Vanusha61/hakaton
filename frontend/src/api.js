const USER_ID = 665767
const COURSE_ID = 755

export async function fetchQuarterProgress(quarter) {
  const r = await fetch(
    `/api/v1/progress/quarter?quarter=${quarter}&user_id=${USER_ID}&course_id=${COURSE_ID}`
  )
  if (!r.ok) throw new Error('API error')
  return r.json()
}

export async function fetchCourseProgress() {
  const r = await fetch(
    `/api/v1/progress/course?user_id=${USER_ID}&course_id=${COURSE_ID}`
  )
  if (!r.ok) throw new Error('API error')
  return r.json()
}
