import { useState, useEffect, useRef } from 'react'
import html2canvas from 'html2canvas'
import ModelViewer from './components/ModelViewer'
import { fetchQuarterProgress, fetchCourseProgress } from './api'
import './App.css'

// ── MOCK DATA (статика — заменится на API /report/{user_id}) ─────────────────
const MOCK = {
  student: { name: 'Иван Петров', course: 'Английский язык · 2024–2025' },

  hero: {
    totalDays: 94,
    totalTasks: 312,
    avgScore: 84,
    totalPoints: 4208,
  },

  funnel: [
    { label: 'Открыто уроков',   value: 48, pct: 100, cls: 'b1' },
    { label: 'Видео открыто',    value: 41, pct: 85,  cls: 'b2' },
    { label: 'Видео досмотрено', value: 35, pct: 73,  cls: 'b3' },
    { label: 'Задачи решены',    value: 37, pct: 77,  cls: 'b4' },
  ],

  activity: {
    videoEngagement: 85,
    completionRate: 73,
    practiceRate: 77,
    fullCycle: 28,
  },

  insights: [
    {
      emoji: '🚀',
      title: 'Отличный старт',
      body: 'Ты решил 312 задач за учебный год. Это больше, чем у большинства учеников на курсе.',
      tag: 'Достижение',
      tagCls: 'tag-blue',
    },
    {
      emoji: '🎬',
      title: 'Видео — твой формат',
      body: '85% уроков ты открывал с просмотра видео. Это говорит о высокой вовлечённости в материал.',
      tag: 'Поведение',
      tagCls: 'tag-teal',
    },
    {
      emoji: '⚡',
      title: 'Практика сразу',
      body: 'В 77% уроков ты переходил к задачам после видео. Это признак активного, а не пассивного обучения.',
      tag: 'Паттерн',
      tagCls: 'tag-orange',
    },
    {
      emoji: '🔁',
      title: 'Полный цикл',
      body: 'В 28 уроках ты прошёл полный путь: видео → перевод → задачи. Именно так формируется навык.',
      tag: 'Глубина',
      tagCls: 'tag-purple',
      wide: true,
    },
  ],

  achievements: [
    { icon: '🏆', title: 'Топ-вовлечённость', desc: '85% уроков с просмотром видео' },
    { icon: '🔥', title: '94 активных дня',   desc: 'Регулярность на протяжении года' },
    { icon: '💡', title: '4 208 баллов',       desc: 'Общий результат за курс' },
    { icon: '🎯', title: 'Средний балл 84',    desc: 'Стабильно высокое качество' },
  ],

  shareCard: {
    phrase: 'Год обучения — это не просто цифры. Это 94 дня роста.',
    stats: [
      { val: '84',    lbl: 'Средний балл' },
      { val: '312',   lbl: 'Задач решено' },
      { val: '94',    lbl: 'Активных дней' },
      { val: '4 208', lbl: 'Баллов' },
    ],
  },
}

const QUARTERS = [
  { label: '1 четверть', model: '/models/stage1.glb' },
  { label: '2 четверть', model: '/models/stage2.glb' },
  { label: '3 четверть', model: '/models/stage3.glb' },
  { label: '4 четверть', model: '/models/stage4.glb' },
]
// ────────────────────────────────────────────────────────────────────────────

export default function App() {
  const { student, hero, funnel, activity, insights, achievements, shareCard } = MOCK

  // ── API state ──────────────────────────────────────────────────────────────
  const [quarterProgress, setQuarterProgress] = useState([null, null, null, null])
  const [courseProgress, setCourseProgress]   = useState(null)

  useEffect(() => {
    // Загружаем прогресс по всем четвертям параллельно
    Promise.allSettled([
      fetchQuarterProgress(1),
      fetchQuarterProgress(2),
      fetchQuarterProgress(3),
      fetchQuarterProgress(4),
    ]).then((results) => {
      setQuarterProgress(
        results.map((r) => (r.status === 'fulfilled' ? r.value.progress : null))
      )
    })

    fetchCourseProgress()
      .then((data) => setCourseProgress(data.progress))
      .catch(() => {})
  }, [])

  // ── Share ──────────────────────────────────────────────────────────────────
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
      backgroundColor: '#0d1230',
      scale: 2,
      useCORS: true,
    })
    const link = document.createElement('a')
    link.download = 'мой-учебный-год.png'
    link.href = canvas.toDataURL('image/png')
    link.click()
  }
  // ──────────────────────────────────────────────────────────────────────────

  return (
    <div className="report">

      {/* ── HERO ─────────────────────────────────────────────────── */}
      <section className="hero">
        <div className="hero-glow" />
        <div className="hero-year">Цифриум · Итоговый отчёт · 2024–2025</div>
        <h1 className="hero-title">Твой учебный год</h1>
        <p className="hero-subtitle">{student.name}</p>
        <p className="hero-course">{student.course}</p>
        <div className="hero-stats">
          <div className="hero-stat">
            <div className="num">{hero.avgScore}</div>
            <div className="lbl">Средний балл</div>
          </div>
          <div className="hero-stat">
            <div className="num">{hero.totalTasks}</div>
            <div className="lbl">Задач решено</div>
          </div>
          <div className="hero-stat">
            <div className="num">{hero.totalDays}</div>
            <div className="lbl">Активных дней</div>
          </div>
          <div className="hero-stat">
            <div className="num">{hero.totalPoints.toLocaleString('ru')}</div>
            <div className="lbl">Баллов набрано</div>
          </div>
        </div>
        {courseProgress !== null && (
          <div className="hero-course-progress">
            <div className="hcp-label">Прогресс по курсу</div>
            <div className="hcp-bar-wrap">
              <div className="hcp-bar" style={{ width: `${courseProgress}%` }} />
            </div>
            <div className="hcp-val">{courseProgress}%</div>
          </div>
        )}
        <div className="hero-scroll">Листай вниз</div>
      </section>

      <div className="divider" />

      {/* ── 3D: ПУТЬ ПО ЧЕТВЕРТЯМ ────────────────────────────────── */}
      <section className="chapter">
        <div className="chapter-label">Твой путь</div>
        <h2 className="chapter-heading">
          Четыре <span className="highlight">этапа</span> учебного года
        </h2>
        <p className="chapter-desc">
          Каждая четверть — отдельный этап. Крути модель мышью, смотри как ты рос.
        </p>

        <div className="quarters-grid">
          {QUARTERS.map((q, i) => {
            const pct = quarterProgress[i]
            return (
              <div key={q.label} className="quarter-card">
                <div className="quarter-model-wrap">
                  <ModelViewer url={q.model} />
                </div>
                <div className="quarter-info">
                  <div className="quarter-label">{q.label}</div>
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
      </section>

      <div className="divider" />

      {/* ── ГЛАВА 1: АКТИВНОСТЬ ──────────────────────────────────── */}
      <section className="chapter">
        <div className="chapter-label">Глава 1</div>
        <h2 className="chapter-heading">
          Ты учился <span className="highlight">{hero.totalDays} дней</span>
        </h2>
        <p className="chapter-desc">
          Регулярность — главный предиктор результата. Посмотри, как выглядит твой путь в цифрах.
        </p>

        <div className="metrics-grid">
          <div className="metric-card accent-blue">
            <div className="metric-label">Активных дней</div>
            <div className="metric-value blue">{hero.totalDays}</div>
            <div className="metric-unit">за учебный год</div>
          </div>
          <div className="metric-card accent-purple">
            <div className="metric-label">Всего баллов</div>
            <div className="metric-value purple">{hero.totalPoints.toLocaleString('ru')}</div>
            <div className="metric-unit">wk points</div>
          </div>
          <div className="metric-card accent-teal large">
            <div className="metric-icon">📅</div>
            <div>
              <div className="metric-label">Последняя активность</div>
              <div className="metric-value teal" style={{ fontSize: 28 }}>31 мая 2025</div>
              <div className="metric-unit">завершил курс вовремя</div>
            </div>
          </div>
        </div>

        <div className="chart-box" style={{ marginTop: 20 }}>
          <div className="metric-label" style={{ marginBottom: 12 }}>Активность по месяцам</div>
          <div className="chart-placeholder">
            <span className="chart-icon">📈</span>
            <span>Столбчатый график — подключим Recharts</span>
          </div>
        </div>
      </section>

      <div className="divider" />

      {/* ── ГЛАВА 2: ПРОГРЕСС ────────────────────────────────────── */}
      <section className="chapter">
        <div className="chapter-label">Глава 2</div>
        <h2 className="chapter-heading">
          Решил <span className="highlight-warm">{hero.totalTasks} задачи</span> со средним баллом {hero.avgScore}
        </h2>
        <p className="chapter-desc">
          Не просто количество, а качество. Посмотри, как ты шёл по урокам.
        </p>

        <div className="metrics-grid cols-3" style={{ marginBottom: 20 }}>
          <div className="metric-card accent-orange">
            <div className="metric-label">Средний балл</div>
            <div className="metric-value orange">{hero.avgScore}</div>
            <div className="metric-unit">из 100</div>
          </div>
          <div className="metric-card">
            <div className="metric-label">Решено задач</div>
            <div className="metric-value blue">{hero.totalTasks}</div>
            <div className="metric-unit">заданий</div>
          </div>
          <div className="metric-card">
            <div className="metric-label">Процент решённых</div>
            <div className="metric-value teal">76%</div>
            <div className="metric-unit">от открытых</div>
          </div>
        </div>

        <div className="charts-row">
          <div className="chart-box">
            <div className="metric-label" style={{ marginBottom: 12 }}>Балл по урокам</div>
            <div className="chart-placeholder">
              <span className="chart-icon">📊</span>
              <span>Линейный график (Recharts)</span>
            </div>
          </div>
          <div className="chart-box">
            <div className="metric-label" style={{ marginBottom: 12 }}>Типы заданий</div>
            <div className="chart-placeholder">
              <span className="chart-icon">🍩</span>
              <span>Donut-диаграмма (Recharts)</span>
            </div>
          </div>
        </div>
      </section>

      <div className="divider" />

      {/* ── ГЛАВА 3: ВОРОНКА ОБУЧЕНИЯ ────────────────────────────── */}
      <section className="chapter">
        <div className="chapter-label">Глава 3</div>
        <h2 className="chapter-heading">
          Как ты <span className="highlight-teal">проходил уроки</span>
        </h2>
        <p className="chapter-desc">
          Воронка показывает, как ты двигался от открытия урока до решения задач. Чем глубже — тем лучше.
        </p>

        <div className="chart-box">
          <div className="funnel">
            {funnel.map((step) => (
              <div key={step.label} className="funnel-step">
                <div className="funnel-label">{step.label}</div>
                <div className="funnel-bar-wrap">
                  <div
                    className={`funnel-bar ${step.cls}`}
                    style={{ width: `${step.pct}%` }}
                  >
                    {step.value}
                  </div>
                </div>
                <div className="funnel-val">{step.pct}%</div>
              </div>
            ))}
          </div>
        </div>

        <div style={{ marginTop: 20 }}>
          <div className="metric-label" style={{ marginBottom: 16 }}>Поведенческие паттерны</div>
          <div className="progress-list">
            <div className="progress-item">
              <div className="p-label">Вовлечённость в видео</div>
              <div className="progress-bar-wrap">
                <div className="progress-bar" style={{ width: `${activity.videoEngagement}%` }} />
              </div>
              <div className="p-val">{activity.videoEngagement}%</div>
            </div>
            <div className="progress-item">
              <div className="p-label">Досматриваемость видео</div>
              <div className="progress-bar-wrap">
                <div className="progress-bar teal" style={{ width: `${activity.completionRate}%` }} />
              </div>
              <div className="p-val">{activity.completionRate}%</div>
            </div>
            <div className="progress-item">
              <div className="p-label">Переход в практику</div>
              <div className="progress-bar-wrap">
                <div className="progress-bar warm" style={{ width: `${activity.practiceRate}%` }} />
              </div>
              <div className="p-val">{activity.practiceRate}%</div>
            </div>
          </div>
        </div>
      </section>

      <div className="divider" />

      {/* ── ГЛАВА 4: ИНСАЙТЫ ─────────────────────────────────────── */}
      <section className="chapter">
        <div className="chapter-label">Глава 4</div>
        <h2 className="chapter-heading">
          Что это <span className="highlight">говорит о тебе</span>
        </h2>
        <p className="chapter-desc">
          Данные — это не только цифры. Вот что мы увидели в твоём учебном году.
        </p>

        <div className="insights-grid">
          {insights.map((ins) => (
            <div key={ins.title} className={`insight-card${ins.wide ? ' wide' : ''}`}>
              <div className="insight-emoji">{ins.emoji}</div>
              <div className="insight-title">{ins.title}</div>
              <div className="insight-body">{ins.body}</div>
              <span className={`insight-tag ${ins.tagCls}`}>{ins.tag}</span>
            </div>
          ))}
        </div>
      </section>

      <div className="divider" />

      {/* ── ГЛАВА 5: ДОСТИЖЕНИЯ ──────────────────────────────────── */}
      <section className="chapter">
        <div className="chapter-label">Глава 5</div>
        <h2 className="chapter-heading">
          Твои <span className="highlight">достижения</span> за год
        </h2>
        <p className="chapter-desc">
          Вехи, которые ты заработал своим трудом.
        </p>

        <div className="achievements-row">
          {achievements.map((a) => (
            <div key={a.title} className="achievement-badge">
              <div className="badge-icon">{a.icon}</div>
              <div>
                <div className="badge-title">{a.title}</div>
                <div className="badge-desc">{a.desc}</div>
              </div>
            </div>
          ))}
        </div>
      </section>

      <div className="divider" />

      {/* ── SHARE CARD ───────────────────────────────────────────── */}
      <section className="share-section-wrap">
        <div className="chapter-label" style={{ marginBottom: 12 }}>Поделиться</div>
        <h2 className="chapter-heading" style={{ marginBottom: 8 }}>
          Покажи свои <span className="highlight">результаты</span>
        </h2>
        <p className="chapter-desc">
          Карточка с твоими главными достижениями — готова к публикации.
        </p>

        <div className="share-card" ref={shareCardRef}>
          <div className="share-card-glow" />
          <div className="share-card-content">
            <div className="share-logo">Цифриум</div>
            <div className="share-name">{student.name}</div>
            <div className="share-course">{student.course}</div>

            <div className="share-numbers">
              {shareCard.stats.map((s) => (
                <div key={s.lbl} className="share-num-item">
                  <div className="share-num-val">{s.val}</div>
                  <div className="share-num-lbl">{s.lbl}</div>
                </div>
              ))}
              {courseProgress !== null && (
                <div className="share-num-item">
                  <div className="share-num-val">{courseProgress}%</div>
                  <div className="share-num-lbl">Курс пройден</div>
                </div>
              )}
            </div>

            <div className="share-phrase">{shareCard.phrase}</div>

            <div className="share-actions">
              <button className="btn-primary" onClick={handleDownload}>
                Скачать карточку
              </button>
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
