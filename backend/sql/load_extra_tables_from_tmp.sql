\set ON_ERROR_STOP on

TRUNCATE TABLE public.lessons;
TRUNCATE TABLE public.wk_users_courses_actions;

DROP TABLE IF EXISTS tmp_lessons_import;
CREATE TEMP TABLE tmp_lessons_import (
    "Column1" text,
    id text,
    course_id text,
    conspect_expected text,
    task_expected text,
    lesson_number text,
    wk_max_points text,
    wk_task_count text,
    wk_survival_training_expected text,
    wk_scratch_playground_enabled text,
    wk_attendance_tracking_enabled text,
    wk_video_duration text
);

\copy tmp_lessons_import FROM '/tmp/lessons.csv' WITH (FORMAT csv, HEADER true, NULL '')

INSERT INTO public.lessons (
    "Column1",
    id,
    course_id,
    conspect_expected,
    task_expected,
    lesson_number,
    wk_max_points,
    wk_task_count,
    wk_survival_training_expected,
    wk_scratch_playground_enabled,
    wk_attendance_tracking_enabled,
    wk_video_duration
)
SELECT
    NULLIF("Column1", '')::integer,
    NULLIF(id, '')::integer,
    NULLIF(course_id, '')::integer,
    NULLIF(conspect_expected, '')::boolean,
    NULLIF(task_expected, '')::boolean,
    NULLIF(replace(lesson_number, '.0', ''), '')::integer,
    NULLIF(wk_max_points, '')::real,
    NULLIF(replace(wk_task_count, '.0', ''), '')::integer,
    NULLIF(wk_survival_training_expected, '')::boolean,
    NULLIF(wk_scratch_playground_enabled, '')::boolean,
    NULLIF(wk_attendance_tracking_enabled, '')::boolean,
    NULLIF(wk_video_duration, '')::real
FROM tmp_lessons_import;

DROP TABLE IF EXISTS tmp_wk_users_courses_actions_import;
CREATE TEMP TABLE tmp_wk_users_courses_actions_import (
    "Column1" text,
    user_id text,
    users_course_id text,
    action text,
    created_at text,
    updated_at text,
    lesson_id text
);

\copy tmp_wk_users_courses_actions_import FROM '/tmp/wk_users_courses_actions.csv' WITH (FORMAT csv, HEADER true, NULL '')

INSERT INTO public.wk_users_courses_actions (
    "Column1",
    user_id,
    users_course_id,
    action,
    created_at,
    updated_at,
    lesson_id
)
SELECT
    NULLIF("Column1", '')::integer,
    NULLIF(user_id, '')::integer,
    NULLIF(users_course_id, '')::integer,
    NULLIF(action, '')::character varying(64),
    NULLIF(created_at, '')::timestamp,
    NULLIF(updated_at, '')::timestamp,
    NULLIF(replace(lesson_id, ',', ''), '')::integer
FROM tmp_wk_users_courses_actions_import;
