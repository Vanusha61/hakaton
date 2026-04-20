#include "TaskStatsService.h"

#include <cmath>

#include "constants/progress_constants.h"

namespace yearreporter::services
{
std::optional<TaskStatsService::Context> TaskStatsService::loadContext(
    const drogon::orm::DbClientPtr &dbClient,
    int userId,
    int courseId,
    ApiError &error) const
{
    CourseMetricsService courseService;
    const auto course = courseService.findUserCourse(dbClient, userId, courseId, error);
    if (!course.has_value())
    {
        return std::nullopt;
    }

    const auto usersCourseId = courseService.findMatchingLessonTrack(dbClient, *course, error);
    if (!usersCourseId.has_value())
    {
        return std::nullopt;
    }

    const auto lessons = courseService.loadCourseLessons(dbClient, courseId, error);
    if (!lessons.has_value())
    {
        return std::nullopt;
    }

    const auto userStates = courseService.loadUserLessonStates(dbClient, userId, *usersCourseId, error);
    if (!userStates.has_value())
    {
        return std::nullopt;
    }

    try
    {
        const auto periodResult = dbClient->execSqlSync(
            "select "
            "    created_at as started_at, "
            "    coalesce(nullif(access_finished_at, ''), nullif(updated_at, ''), created_at) as finished_at "
            "from public.user_courses "
            "where user_id = $1 and course_id = $2 "
            "limit 1",
            userId,
            courseId);

        if (periodResult.empty())
        {
            error = {drogon::k404NotFound, "Курс пользователя не найден"};
            return std::nullopt;
        }

        Context context;
        context.userId = userId;
        context.courseId = courseId;
        context.usersCourseId = *usersCourseId;
        context.startedAt = periodResult.front()["started_at"].as<std::string>();
        context.finishedAt = periodResult.front()["finished_at"].as<std::string>();
        context.lessons = *lessons;

        for (const auto &state : *userStates)
        {
            context.stateByLessonId.emplace(state.lessonId, state);
        }

        return context;
    }
    catch (const std::exception &e)
    {
        error = {drogon::k500InternalServerError,
                 "Ошибка загрузки временного контекста курса: " + std::string(e.what())};
        return std::nullopt;
    }
}

std::vector<CourseMetricsService::CourseLesson> TaskStatsService::sliceLessons(
    const std::vector<CourseMetricsService::CourseLesson> &lessons,
    int quarter,
    ApiError &error)
{
    if (quarter < 1 || quarter > yearreporter::constants::progress::kQuarterCount)
    {
        error = {drogon::k400BadRequest,
                 "Параметр 'quarter' должен быть числом от 1 до 4"};
        return {};
    }

    const auto slice = CourseMetricsService::quarterSliceFor(lessons, quarter);
    return {lessons.begin() + static_cast<std::ptrdiff_t>(slice.begin),
            lessons.begin() + static_cast<std::ptrdiff_t>(slice.end)};
}

std::optional<TaskStatsService::HomeworkAggregate> TaskStatsService::loadHomeworkYear(
    const drogon::orm::DbClientPtr &dbClient,
    int userId,
    ApiError &error) const
{
    try
    {
        const auto result = dbClient->execSqlSync(
            "select "
            "    coalesce(sum(coalesce(solved_tasks_count, 0)), 0)::int as solved_tasks_count, "
            "    coalesce(sum(coalesce(earned_points, 0)), 0) as earned_points "
            "from public.user_trainings "
            "where user_id = $1 "
            "  and type = 'UserTrainings::RegularTraining'",
            userId);

        HomeworkAggregate aggregate;
        if (!result.empty())
        {
            aggregate.solvedTasksCount = result.front()["solved_tasks_count"].as<int>();
            aggregate.earnedPoints = result.front()["earned_points"].as<double>();
        }
        return aggregate;
    }
    catch (const std::exception &e)
    {
        error = {drogon::k500InternalServerError,
                 "Ошибка загрузки homework-метрик за год: " + std::string(e.what())};
        return std::nullopt;
    }
}

std::optional<TaskStatsService::HomeworkAggregate> TaskStatsService::loadHomeworkQuarter(
    const drogon::orm::DbClientPtr &dbClient,
    int userId,
    int quarter,
    const std::string &startedAt,
    const std::string &finishedAt,
    ApiError &error) const
{
    if (quarter < 1 || quarter > yearreporter::constants::progress::kQuarterCount)
    {
        error = {drogon::k400BadRequest,
                 "Параметр 'quarter' должен быть числом от 1 до 4"};
        return std::nullopt;
    }

    try
    {
        const auto result = dbClient->execSqlSync(
            "with bounds as ("
            "    select "
            "        $3::timestamptz as started_at, "
            "        $4::timestamptz as finished_at"
            "), quarter_bounds as ("
            "    select "
            "        case $2::int "
            "            when 1 then started_at "
            "            when 2 then started_at + (finished_at - started_at) / 4 "
            "            when 3 then started_at + (finished_at - started_at) / 2 "
            "            else started_at + ((finished_at - started_at) * 3) / 4 "
            "        end as quarter_start, "
            "        case $2::int "
            "            when 1 then started_at + (finished_at - started_at) / 4 "
            "            when 2 then started_at + (finished_at - started_at) / 2 "
            "            when 3 then started_at + ((finished_at - started_at) * 3) / 4 "
            "            else finished_at "
            "        end as quarter_end "
            "    from bounds"
            ") "
            "select "
            "    coalesce(sum(coalesce(ut.solved_tasks_count, 0)), 0)::int as solved_tasks_count, "
            "    coalesce(sum(coalesce(ut.earned_points, 0)), 0) as earned_points "
            "from public.user_trainings ut "
            "cross join quarter_bounds qb "
            "where ut.user_id = $1 "
            "  and ut.type = 'UserTrainings::RegularTraining' "
            "  and ut.started_at::timestamptz >= qb.quarter_start "
            "  and (( $2 < 4 and ut.started_at::timestamptz < qb.quarter_end ) "
            "       or ( $2::int = 4 and ut.started_at::timestamptz <= qb.quarter_end ))",
            userId,
            quarter,
            startedAt,
            finishedAt);

        HomeworkAggregate aggregate;
        if (!result.empty())
        {
            aggregate.solvedTasksCount = result.front()["solved_tasks_count"].as<int>();
            aggregate.earnedPoints = result.front()["earned_points"].as<double>();
        }
        return aggregate;
    }
    catch (const std::exception &e)
    {
        error = {drogon::k500InternalServerError,
                 "Ошибка загрузки homework-метрик по четверти: " + std::string(e.what())};
        return std::nullopt;
    }
}

double TaskStatsService::roundToTwoDigits(double value)
{
    return std::round(value * 100.0) / 100.0;
}

AverageScoreResult TaskStatsService::buildAverageScoreResult(int solvedTasksCount, double earnedPoints)
{
    AverageScoreResult result;
    result.solvedTasksCount = solvedTasksCount;
    result.earnedPoints = roundToTwoDigits(earnedPoints);
    result.averageScore =
        solvedTasksCount > 0 ? roundToTwoDigits(earnedPoints / static_cast<double>(solvedTasksCount)) : 0.0;
    return result;
}

std::optional<SolvedTasksCountResult> TaskStatsService::computeHomeworkSolvedTasksYear(
    const drogon::orm::DbClientPtr &dbClient,
    int userId,
    int courseId,
    ApiError &error) const
{
    const auto context = loadContext(dbClient, userId, courseId, error);
    if (!context.has_value())
    {
        return std::nullopt;
    }

    const auto homework = loadHomeworkYear(dbClient, userId, error);
    if (!homework.has_value())
    {
        return std::nullopt;
    }

    return SolvedTasksCountResult{.solvedTasksCount = homework->solvedTasksCount};
}

std::optional<SolvedTasksCountResult> TaskStatsService::computeHomeworkSolvedTasksQuarter(
    const drogon::orm::DbClientPtr &dbClient,
    int quarter,
    int userId,
    int courseId,
    ApiError &error) const
{
    const auto context = loadContext(dbClient, userId, courseId, error);
    if (!context.has_value())
    {
        return std::nullopt;
    }

    const auto homework =
        loadHomeworkQuarter(dbClient, userId, quarter, context->startedAt, context->finishedAt, error);
    if (!homework.has_value())
    {
        return std::nullopt;
    }

    return SolvedTasksCountResult{.solvedTasksCount = homework->solvedTasksCount};
}

std::optional<SolvedTasksCountResult> TaskStatsService::computeLessonSolvedTasksYear(
    const drogon::orm::DbClientPtr &dbClient,
    int userId,
    int courseId,
    ApiError &error) const
{
    const auto context = loadContext(dbClient, userId, courseId, error);
    if (!context.has_value())
    {
        return std::nullopt;
    }

    int solvedTasksCount = 0;
    for (const auto &lesson : context->lessons)
    {
        const auto stateIt = context->stateByLessonId.find(lesson.lessonId);
        if (stateIt == context->stateByLessonId.end())
        {
            continue;
        }
        solvedTasksCount += stateIt->second.solvedTasks;
    }

    return SolvedTasksCountResult{.solvedTasksCount = solvedTasksCount};
}

std::optional<SolvedTasksCountResult> TaskStatsService::computeLessonSolvedTasksQuarter(
    const drogon::orm::DbClientPtr &dbClient,
    int quarter,
    int userId,
    int courseId,
    ApiError &error) const
{
    const auto context = loadContext(dbClient, userId, courseId, error);
    if (!context.has_value())
    {
        return std::nullopt;
    }

    const auto lessons = sliceLessons(context->lessons, quarter, error);
    if (!error.reason.empty())
    {
        return std::nullopt;
    }

    int solvedTasksCount = 0;
    for (const auto &lesson : lessons)
    {
        const auto stateIt = context->stateByLessonId.find(lesson.lessonId);
        if (stateIt == context->stateByLessonId.end())
        {
            continue;
        }
        solvedTasksCount += stateIt->second.solvedTasks;
    }

    return SolvedTasksCountResult{.solvedTasksCount = solvedTasksCount};
}

std::optional<AverageScoreResult> TaskStatsService::computeAverageScoreYear(
    const drogon::orm::DbClientPtr &dbClient,
    int userId,
    int courseId,
    ApiError &error) const
{
    const auto context = loadContext(dbClient, userId, courseId, error);
    if (!context.has_value())
    {
        return std::nullopt;
    }

    const auto homework = loadHomeworkYear(dbClient, userId, error);
    if (!homework.has_value())
    {
        return std::nullopt;
    }

    int lessonSolvedTasksCount = 0;
    double lessonEarnedPoints = 0.0;
    for (const auto &lesson : context->lessons)
    {
        const auto stateIt = context->stateByLessonId.find(lesson.lessonId);
        if (stateIt == context->stateByLessonId.end())
        {
            continue;
        }
        lessonSolvedTasksCount += stateIt->second.solvedTasks;
        lessonEarnedPoints += stateIt->second.earnedPoints;
    }

    return buildAverageScoreResult(
        lessonSolvedTasksCount + homework->solvedTasksCount,
        lessonEarnedPoints + homework->earnedPoints);
}

std::optional<AverageScoreResult> TaskStatsService::computeAverageScoreQuarter(
    const drogon::orm::DbClientPtr &dbClient,
    int quarter,
    int userId,
    int courseId,
    ApiError &error) const
{
    const auto context = loadContext(dbClient, userId, courseId, error);
    if (!context.has_value())
    {
        return std::nullopt;
    }

    const auto lessons = sliceLessons(context->lessons, quarter, error);
    if (!error.reason.empty())
    {
        return std::nullopt;
    }

    const auto homework =
        loadHomeworkQuarter(dbClient, userId, quarter, context->startedAt, context->finishedAt, error);
    if (!homework.has_value())
    {
        return std::nullopt;
    }

    int lessonSolvedTasksCount = 0;
    double lessonEarnedPoints = 0.0;
    for (const auto &lesson : lessons)
    {
        const auto stateIt = context->stateByLessonId.find(lesson.lessonId);
        if (stateIt == context->stateByLessonId.end())
        {
            continue;
        }
        lessonSolvedTasksCount += stateIt->second.solvedTasks;
        lessonEarnedPoints += stateIt->second.earnedPoints;
    }

    return buildAverageScoreResult(
        lessonSolvedTasksCount + homework->solvedTasksCount,
        lessonEarnedPoints + homework->earnedPoints);
}
}  // namespace yearreporter::services
