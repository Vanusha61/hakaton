\set ON_ERROR_STOP on

DROP TABLE IF EXISTS public.lessons;
CREATE TABLE public.lessons (
    "Column1" integer,
    id integer,
    course_id integer,
    conspect_expected boolean,
    task_expected boolean,
    lesson_number integer,
    wk_max_points real,
    wk_task_count integer,
    wk_survival_training_expected boolean,
    wk_scratch_playground_enabled boolean,
    wk_attendance_tracking_enabled boolean,
    wk_video_duration real
);

DROP TABLE IF EXISTS public.wk_users_courses_actions;
CREATE TABLE public.wk_users_courses_actions (
    "Column1" integer,
    user_id integer,
    users_course_id integer,
    action character varying(64),
    created_at timestamp,
    updated_at timestamp,
    lesson_id integer
);
