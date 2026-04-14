import { useState, useEffect, useRef, useCallback } from 'react'
import html2canvas from 'html2canvas'
import {
  LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip,
  ResponsiveContainer, PieChart, Pie, Cell, Legend,
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

const TABS = [
  { id: 1, label: '1' },
  { id: 2, label: '2' },
  { id: 3, label: '3' },
  { id: 4, label: '4' },
  { id: 'year', label: 'Год' },
]

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
  const [attention,  setAttention]  = useState(null)
  const [loading,    setLoading]    = useState(false)

  const resetData = useCallback(() => {
    setProgress(null); setVideos(null); setPoints(null); setConspects(null)
    setMedia(null); setChrono(null); setSolved(null); setAttendance(null)
    setRare(null); setRarest(null); setAttention(null)
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
      setQuarterProgress(res.map((r) => r.status === 'fulfilled' ? r.value.progress : null))
    )

    const calls = q
      ? [
          fetchQuarterProgress(q).then((d) => setProgress(d.progress)),
          fetchVideosQuarter(q).then(setVideos),
          fetchPointsQuarter(q).then(setPoints),
          fetchConspectsQuarter(q).then(setConspects),
          fetchMediaQuarter(q).then(setMedia),
          fetchChronotypeQuarter(q).then(setChrono),
          fetchSolvedLessonsQuarter(q).then(setSolved),
          fetchAttendanceQuarter(q).then(setAttendance),
          fetchRareLessonsQuarter(q).then(setRare),
          fetchRarestLessonQuarter(q).then(setRarest),
          fetchVideoAttentionQuarter(q).then(setAttention),
        ]
      : [
          fetchCourseProgress().then((d) => setProgress(d.progress)),
          fetchVideosYear().then(setVideos),
          fetchPointsYear().then(setPoints),
          fetchConspectsYear().then(setConspects),
          fetchMediaYear().then(setMedia),
          fetchChronotypeYear().then(setChrono),
          fetchSolvedLessonsYear().then(setSolved),
          fetchAttendanceYear().then(setAttendance),
          fetchRareLessonsYear().then(setRare),
          fetchRarestLessonYear().then(setRarest),
          fetchVideoAttentionYear().then(setAttention),
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

        <div className="hero-scroll">Листай вниз</div>
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
            {quarterProgress[tab - 1] !== null && (
              <div className="single-model-progress">
                <div className="quarter-progress-bar-wrap" style={{ flex: 1 }}>
                  <div
                    className="quarter-progress-bar"
                    style={{ width: `${quarterProgress[tab - 1]}%` }}
                  />
                </div>
                <span className="single-model-pct">{quarterProgress[tab - 1]}%</span>
              </div>
            )}
          </div>
        )}
      </section>

      <div className="divider" />

      {/* ══ ГЛАВА 1: ПРОГРЕСС ═══════════════════════════════════════════════ */}
      <section className="chapter">
        <div className="chapter-label">Глава 1</div>
        <h2 className="chapter-heading">
          Твой <span className="highlight">прогресс</span> {tabLabel}
        </h2>
        <p className="chapter-desc">Видео, баллы, конспекты и полностью решённые уроки.</p>

        <div className="metrics-grid">
          <div className="metric-card accent-blue">
            <div className="metric-label">Видео просмотрено</div>
            <div className="metric-value blue">
              {videos ? `${videos.watched_count} / ${videos.total_count}` : '…'}
            </div>
            <div className="metric-unit">уроков</div>
            {videos && (
              <div className="quarter-progress-bar-wrap" style={{ marginTop: 10 }}>
                <div
                  className="quarter-progress-bar"
                  style={{ width: `${Math.round(videos.watched_count / videos.total_count * 100)}%` }}
                />
              </div>
            )}
          </div>

          <div className="metric-card accent-purple">
            <div className="metric-label">Баллов набрано</div>
            <div className="metric-value purple">
              {points ? `${points.earned_count} / ${points.total_count}` : '…'}
            </div>
            <div className="metric-unit">из максимума</div>
            {points && (
              <div className="quarter-progress-bar-wrap" style={{ marginTop: 10 }}>
                <div
                  className="quarter-progress-bar"
                  style={{
                    width: `${Math.round(points.earned_count / points.total_count * 100)}%`,
                    background: 'linear-gradient(90deg,#b06bff,#ff6b9d)',
                  }}
                />
              </div>
            )}
          </div>

          <div className="metric-card accent-teal">
            <div className="metric-label">Конспектов изучено</div>
            <div className="metric-value teal">
              {conspects ? `${conspects.learned_count} / ${conspects.total_count}` : '…'}
            </div>
            <div className="metric-unit">материалов</div>
            {conspects && (
              <div className="quarter-progress-bar-wrap" style={{ marginTop: 10 }}>
                <div
                  className="quarter-progress-bar teal"
                  style={{ width: `${Math.round(conspects.learned_count / conspects.total_count * 100)}%` }}
                />
              </div>
            )}
          </div>

          <div className="metric-card accent-orange">
            <div className="metric-label">Уроков решено полностью</div>
            <div className="metric-value orange" style={{ fontSize: 36 }}>
              {solved ? solved.full_solved_lessons_count : '…'}
            </div>
            <div className="metric-unit">уроков с выполненными задачами</div>
          </div>
        </div>

        {attendance?.is_user_visited_all_lessons && (
          <div className="achievements-row" style={{ marginTop: 16 }}>
            <div className="achievement-badge">
              <div className="badge-icon">✅</div>
              <div>
                <div className="badge-title">Без прогулов</div>
                <div className="badge-desc">Посетил все уроки</div>
              </div>
            </div>
          </div>
        )}
      </section>

      <div className="divider" />

      {/* ══ ГЛАВА 2: МЕДИАКОНТЕНТ ═══════════════════════════════════════════ */}
      <section className="chapter">
        <div className="chapter-label">Глава 2</div>
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
              <ResponsiveContainer width="100%" height={280}>
                <PieChart>
                  <Pie
                    data={mediaPieData}
                    cx="50%"
                    cy="50%"
                    innerRadius={72}
                    outerRadius={112}
                    paddingAngle={3}
                    dataKey="value"
                    label={({ pct }) => `${pct}%`}
                    labelLine={false}
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
      </section>

      <div className="divider" />

      {/* ══ ГЛАВА 3: ХРОНОТИП — ТАБЛИЦА ════════════════════════════════════ */}
      <section className="chapter">
        <div className="chapter-label">Глава 3</div>
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
            <span>Название для отчёта</span>
            <span>Инсайт</span>
          </div>
          {CHRONOTYPE_ROWS.map((row) => {
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

      {/* ══ ГЛАВА 4: ТОП-3 СЛОЖНЫХ УРОКА ═══════════════════════════════════ */}
      <section className="chapter">
        <div className="chapter-label">Глава 4</div>
        <h2 className="chapter-heading">
          Уроки, где ты <span className="highlight">оказался сильнее</span>
        </h2>
        <p className="chapter-desc">
          Ты решил эти уроки — а большинство учеников не справились.
        </p>

        {rareLessonsList.length > 0 ? (
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
        ) : (
          <div className="chart-box">
            <div className="chart-placeholder">
              <span className="chart-icon">🏆</span>
              <span>{loading ? 'Загрузка…' : 'Нет данных'}</span>
            </div>
          </div>
        )}
      </section>

      <div className="divider" />

      {/* ══ ГЛАВА 5: САМЫЙ РЕДКИЙ УРОК ══════════════════════════════════════ */}
      <section className="chapter">
        <div className="chapter-label">Глава 5</div>
        <h2 className="chapter-heading">
          {rarest
            ? rarest.report_name
            : <>Ты посетил <span className="highlight">самый редкий урок</span></>}
        </h2>
        <p className="chapter-desc">Этот урок посетили лишь единицы. Ты — один из них.</p>

        {rarest ? (
          <div className="insight-card wide">
            <div className="insight-emoji" style={{ fontSize: 44 }}>🌟</div>
            <div className="insight-title">{rarest.title}</div>
            <div style={{ color: 'var(--text-muted)', marginBottom: 10, fontSize: 14 }}>
              Посетили:{' '}
              <strong style={{ color: '#fff' }}>
                {rarest.visited_users_count} из {rarest.total_users_count}
              </strong>{' '}
              учеников ({rarest.visited_percentage}%)
            </div>
            <div className="insight-body">{rarest.insight}</div>
          </div>
        ) : (
          <div className="chart-box">
            <div className="chart-placeholder">
              <span className="chart-icon">🌟</span>
              <span>{loading ? 'Загрузка…' : 'Нет данных'}</span>
            </div>
          </div>
        )}
      </section>

      <div className="divider" />

      {/* ══ ГЛАВА 6: ПРОФИЛЬ ВНИМАНИЯ — ЛИНЕЙНЫЙ ГРАФИК ════════════════════ */}
      <section className="chapter">
        <div className="chapter-label">Глава 6</div>
        <h2 className="chapter-heading">
          Как ты <span className="highlight-warm">смотришь видео</span>
        </h2>
        <p className="chapter-desc">
          По сегментам видео определяем, какое время просмотра самое частое.
          AI интерпретирует результат.
        </p>

        {attention ? (
          <>
            <div className="chart-box">
              <div className="metric-label" style={{ marginBottom: 16 }}>
                Частота просмотра по таймлайну видео
              </div>
              <ResponsiveContainer width="100%" height={220}>
                <LineChart data={attentionCurve} margin={{ top: 4, right: 16, bottom: 20, left: -20 }}>
                  <defs>
                    <linearGradient id="attentionGrad" x1="0" y1="0" x2="1" y2="0">
                      <stop offset="0%"   stopColor="#6b8fff" />
                      <stop offset="100%" stopColor="#b06bff" />
                    </linearGradient>
                  </defs>
                  <CartesianGrid strokeDasharray="3 3" stroke="rgba(255,255,255,0.05)" />
                  <XAxis
                    dataKey="pos"
                    tick={{ fill: '#7a8299', fontSize: 12 }}
                    tickLine={false}
                    axisLine={false}
                    label={{
                      value: 'Таймлайн видео',
                      position: 'insideBottom',
                      offset: -12,
                      fill: '#4a5470',
                      fontSize: 12,
                    }}
                  />
                  <YAxis
                    tick={{ fill: '#7a8299', fontSize: 12 }}
                    tickLine={false}
                    axisLine={false}
                    label={{
                      value: 'Частота',
                      angle: -90,
                      position: 'insideLeft',
                      offset: 12,
                      fill: '#4a5470',
                      fontSize: 12,
                    }}
                  />
                  <Tooltip
                    contentStyle={{ background: '#111520', border: '1px solid #1e2a45', borderRadius: 10 }}
                    labelStyle={{ color: '#7a8299' }}
                    itemStyle={{ color: '#6b8fff' }}
                    formatter={(v) => [v, 'Частота просмотра']}
                  />
                  <Line
                    type="monotone"
                    dataKey="freq"
                    stroke="url(#attentionGrad)"
                    strokeWidth={3}
                    dot={false}
                    activeDot={{ r: 5, fill: '#6b8fff' }}
                  />
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
        ) : (
          <div className="chart-box">
            <div className="chart-placeholder">
              <span className="chart-icon">📈</span>
              <span>{loading ? 'Загрузка…' : 'Нет данных'}</span>
            </div>
          </div>
        )}
      </section>

      <div className="divider" />

      {/* ══ SHARE CARD ══════════════════════════════════════════════════════ */}
      <section className="share-section-wrap">
        <div className="chapter-label" style={{ marginBottom: 12 }}>Поделиться</div>
        <h2 className="chapter-heading" style={{ marginBottom: 8 }}>
          Покажи свои <span className="highlight">результаты</span>
        </h2>
        <p className="chapter-desc">Карточка с твоими главными достижениями — готова к публикации.</p>

        <div className="share-card" ref={shareCardRef}>
          <div className="share-card-glow" />
          <div className="share-card-content">
            <div className="share-logo">Цифриум</div>
            <div className="share-name">{STUDENT.name}</div>
            <div className="share-course">{STUDENT.course}</div>

            <div className="share-numbers">
              {points && (
                <div className="share-num-item">
                  <div className="share-num-val">{points.earned_count}</div>
                  <div className="share-num-lbl">Баллов</div>
                </div>
              )}
              {videos && (
                <div className="share-num-item">
                  <div className="share-num-val">{videos.watched_count}</div>
                  <div className="share-num-lbl">Видео</div>
                </div>
              )}
              {solved && (
                <div className="share-num-item">
                  <div className="share-num-val">{solved.full_solved_lessons_count}</div>
                  <div className="share-num-lbl">Уроков решено</div>
                </div>
              )}
              {progress !== null && (
                <div className="share-num-item">
                  <div className="share-num-val">{progress}%</div>
                  <div className="share-num-lbl">Прогресс</div>
                </div>
              )}
            </div>

            <div className="share-phrase">
              Год обучения — это не просто цифры.
              {solved ? ` Это ${solved.full_solved_lessons_count} уроков роста.` : ''}
            </div>

            <div className="share-actions">
              <button className="btn-primary" onClick={handleDownload}>Скачать карточку</button>
              <button className="btn-outline" onClick={handleCopyLink}>
                {copied ? 'Скопировано!' : 'Скопировать ссылку'}
              </button>
            </div>
          </div>
        </div>
      </section>

    </div>
  )
}
