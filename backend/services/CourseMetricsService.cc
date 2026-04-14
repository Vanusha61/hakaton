#include "CourseMetricsService.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "constants/progress_constants.h"

namespace yearreporter::services
{
using namespace yearreporter::constants::progress;

int CourseMetricsService::roundCount(double value)
{
    return static_cast<int>(std::lround(value));
}

std::optional<CourseMetricsService::UserCourse> CourseMetricsService::findUserCourse(
    const drogon::orm::DbClientPtr &dbClient,
    int userId,
    int courseId,
    ApiError &error) const
{
    try
    {
        const auto result = dbClient->execSqlSync(
            "select user_id, course_id, wk_points, wk_max_points, wk_solved_task_count "
            "from public.user_courses "
            "where user_id = $1 and course_id = $2 "
            "limit 1",
            userId,
            courseId);

        if (result.empty())
        {
            error = {drogon::k404NotFound, "Курс или пользователь не найдены в базе"};
            return std::nullopt;
        }

        const auto &row = result.front();
        return UserCourse{
            row["user_id"].as<int>(),
            row["course_id"].as<int>(),
            row["wk_points"].as<double>(),
            row["wk_max_points"].as<double>(),
            row["wk_solved_task_count"].as<int>()};
    }
    catch (const std::exception &e)
    {
        error = {drogon::k500InternalServerError,
                 "Ошибка чтения данных курса: " + std::string(e.what())};
        return std::nullopt;
    }
}

std::optional<int> CourseMetricsService::findMatchingLessonTrack(const drogon::orm::DbClientPtr &dbClient,
                                                                 const UserCourse &course,
                                                                 ApiError &error) const
{
    try
    {
        const auto result = dbClient->execSqlSync(
            "select users_course_id, "
            "sum(coalesce(wk_points, 0)) as lesson_points, "
            "sum(coalesce(wk_solved_task_count, 0)) as lesson_solved "
            "from public.user_lessons "
            "where user_id = $1 "
            "group by users_course_id",
            course.userId);

        if (result.empty())
        {
            error = {drogon::k404NotFound, "Для пользователя не найдены данные по урокам"};
            return std::nullopt;
        }

        double bestScore = std::numeric_limits<double>::max();
        std::optional<int> bestId;

        for (const auto &row : result)
        {
            const auto lessonPoints = row["lesson_points"].as<double>();
            const auto lessonSolved = row["lesson_solved"].as<int>();
            const auto exactPoints = std::fabs(lessonPoints - course.earnedPoints) <= kPointEpsilon;
            const auto exactSolved = lessonSolved == course.solvedTaskCount;

            if (exactPoints && exactSolved)
            {
                return row["users_course_id"].as<int>();
            }

            const auto score = std::fabs(lessonPoints - course.earnedPoints) +
                               std::fabs(static_cast<double>(lessonSolved - course.solvedTaskCount)) *
                                   100.0;
            if (score < bestScore)
            {
                bestScore = score;
                bestId = row["users_course_id"].as<int>();
            }
        }

        if (!bestId.has_value())
        {
            error = {drogon::k404NotFound,
                     "Не удалось определить траекторию уроков пользователя"};
            return std::nullopt;
        }

        return bestId;
    }
    catch (const std::exception &e)
    {
        error = {drogon::k500InternalServerError,
                 "Ошибка чтения уроков курса: " + std::string(e.what())};
        return std::nullopt;
    }
}

std::optional<std::vector<CourseMetricsService::CourseLesson>> CourseMetricsService::loadCourseLessons(
    const drogon::orm::DbClientPtr &dbClient,
    int courseId,
    ApiError &error) const
{
    try
    {
        const auto result = dbClient->execSqlSync(
            "select l.id, l.lesson_number, coalesce(l.wk_max_points, 0) as wk_max_points, "
            "coalesce(l.wk_task_count, 0) as wk_task_count, "
            "coalesce(l.task_expected, false) as task_expected, "
            "coalesce(l.wk_video_duration, 0) as wk_video_duration, "
            "exists ("
            "    select 1 from public.wk_users_courses_actions a "
            "    where a.action = 'visit_preparation_material' and a.lesson_id = l.id"
            ") as has_preparation_material "
            "from public.lessons l "
            "where l.course_id = $1 "
            "order by l.id",
            courseId);

        std::vector<CourseLesson> lessons;
        lessons.reserve(result.size());
        for (const auto &row : result)
        {
            CourseLesson lesson;
            lesson.lessonId = row["id"].as<int>();
            if (!row["lesson_number"].isNull())
            {
                lesson.lessonNumber = row["lesson_number"].as<int>();
            }
            lesson.maxPoints = row["wk_max_points"].as<double>();
            lesson.maxTaskCount = row["wk_task_count"].as<int>();
            lesson.taskExpected = row["task_expected"].as<bool>();
            lesson.hasVideo = row["wk_video_duration"].as<double>() > 0.0;
            lesson.hasPreparationMaterial = row["has_preparation_material"].as<bool>();
            lessons.push_back(lesson);
        }
        return lessons;
    }
    catch (const std::exception &e)
    {
        error = {drogon::k500InternalServerError,
                 "Ошибка загрузки уроков курса: " + std::string(e.what())};
        return std::nullopt;
    }
}

std::optional<std::vector<CourseMetricsService::UserLessonState>> CourseMetricsService::loadUserLessonStates(
    const drogon::orm::DbClientPtr &dbClient,
    int userId,
    int usersCourseId,
    ApiError &error) const
{
    try
    {
        const auto result = dbClient->execSqlSync(
            "select lesson_id, coalesce(wk_points, 0) as wk_points, "
            "coalesce(wk_solved_task_count, 0) as wk_solved_task_count, "
            "coalesce(solved, false) as solved, "
            "coalesce(video_visited, false) or coalesce(video_viewed, false) as video_watched, "
            "coalesce(translation_visited, false) as translation_visited "
            "from public.user_lessons "
            "where user_id = $1 and users_course_id = $2",
            userId,
            usersCourseId);

        std::vector<UserLessonState> states;
        states.reserve(result.size());
        for (const auto &row : result)
        {
            states.push_back(UserLessonState{
                row["lesson_id"].as<int>(),
                row["wk_points"].as<double>(),
                row["wk_solved_task_count"].as<int>(),
                row["solved"].as<bool>(),
                row["video_watched"].as<bool>(),
                row["translation_visited"].as<bool>()});
        }
        return states;
    }
    catch (const std::exception &e)
    {
        error = {drogon::k500InternalServerError,
                 "Ошибка загрузки пользовательских уроков: " + std::string(e.what())};
        return std::nullopt;
    }
}

std::optional<std::unordered_set<int>> CourseMetricsService::loadCoursePreparationMaterials(
    const drogon::orm::DbClientPtr &dbClient,
    int courseId,
    ApiError &error) const
{
    try
    {
        const auto result = dbClient->execSqlSync(
            "select distinct a.lesson_id "
            "from public.wk_users_courses_actions a "
            "join public.lessons l on l.id = a.lesson_id "
            "where a.action = 'visit_preparation_material' and l.course_id = $1 "
            "and a.lesson_id is not null",
            courseId);

        std::unordered_set<int> ids;
        for (const auto &row : result)
        {
            ids.insert(row["lesson_id"].as<int>());
        }
        return ids;
    }
    catch (const std::exception &e)
    {
        error = {drogon::k500InternalServerError,
                 "Ошибка загрузки вспомогательных материалов: " + std::string(e.what())};
        return std::nullopt;
    }
}

std::optional<std::unordered_set<int>> CourseMetricsService::loadVisitedPreparationMaterials(
    const drogon::orm::DbClientPtr &dbClient,
    int usersCourseId,
    ApiError &error) const
{
    try
    {
        const auto result = dbClient->execSqlSync(
            "select distinct lesson_id "
            "from public.wk_users_courses_actions "
            "where action = 'visit_preparation_material' and users_course_id = $1 "
            "and lesson_id is not null",
            usersCourseId);

        std::unordered_set<int> ids;
        for (const auto &row : result)
        {
            ids.insert(row["lesson_id"].as<int>());
        }
        return ids;
    }
    catch (const std::exception &e)
    {
        error = {drogon::k500InternalServerError,
                 "Ошибка загрузки посещённых материалов: " + std::string(e.what())};
        return std::nullopt;
    }
}

CourseMetricsService::QuarterSlice CourseMetricsService::quarterSliceFor(
    const std::vector<CourseLesson> &lessons,
    int quarter)
{
    const auto totalLessons = lessons.size();
    const auto baseSize = totalLessons / kQuarterCount;
    const auto remainder = totalLessons % kQuarterCount;

    size_t begin = 0;
    for (int idx = 1; idx < quarter; ++idx)
    {
        begin += baseSize + (idx <= static_cast<int>(remainder) ? 1 : 0);
    }

    const auto currentSize = baseSize + (quarter <= static_cast<int>(remainder) ? 1 : 0);
    return QuarterSlice{begin, begin + currentSize};
}

std::optional<CountMetrics> CourseMetricsService::computeVideoQuarter(
    const drogon::orm::DbClientPtr &dbClient,
    int quarter,
    int userId,
    int courseId,
    ApiError &error) const
{
    if (quarter < 1 || quarter > kQuarterCount)
    {
        error = {drogon::k400BadRequest,
                 "Параметр 'quarter' должен быть числом от 1 до 4"};
        return std::nullopt;
    }

    const auto course = findUserCourse(dbClient, userId, courseId, error);
    if (!course.has_value())
    {
        return std::nullopt;
    }
    const auto trackId = findMatchingLessonTrack(dbClient, *course, error);
    if (!trackId.has_value())
    {
        return std::nullopt;
    }
    const auto lessons = loadCourseLessons(dbClient, courseId, error);
    if (!lessons.has_value())
    {
        return std::nullopt;
    }
    const auto userStates = loadUserLessonStates(dbClient, userId, *trackId, error);
    if (!userStates.has_value())
    {
        return std::nullopt;
    }

    std::unordered_map<int, UserLessonState> stateByLessonId;
    for (const auto &state : *userStates)
    {
        stateByLessonId[state.lessonId] = state;
    }

    const auto slice = quarterSliceFor(*lessons, quarter);
    CountMetrics metrics;
    for (size_t index = slice.begin; index < slice.end; ++index)
    {
        const auto &lesson = (*lessons)[index];
        if (!lesson.hasVideo)
        {
            continue;
        }
        ++metrics.totalCount;
        const auto state = stateByLessonId.find(lesson.lessonId);
        if (state != stateByLessonId.end() && state->second.videoWatched)
        {
            ++metrics.valueCount;
        }
    }
    return metrics;
}

std::optional<CountMetrics> CourseMetricsService::computeVideoYear(const drogon::orm::DbClientPtr &dbClient,
                                                                   int userId,
                                                                   int courseId,
                                                                   ApiError &error) const
{
    const auto course = findUserCourse(dbClient, userId, courseId, error);
    if (!course.has_value())
    {
        return std::nullopt;
    }
    const auto trackId = findMatchingLessonTrack(dbClient, *course, error);
    if (!trackId.has_value())
    {
        return std::nullopt;
    }
    const auto lessons = loadCourseLessons(dbClient, courseId, error);
    if (!lessons.has_value())
    {
        return std::nullopt;
    }
    const auto userStates = loadUserLessonStates(dbClient, userId, *trackId, error);
    if (!userStates.has_value())
    {
        return std::nullopt;
    }

    std::unordered_map<int, UserLessonState> stateByLessonId;
    for (const auto &state : *userStates)
    {
        stateByLessonId[state.lessonId] = state;
    }

    CountMetrics metrics;
    for (const auto &lesson : *lessons)
    {
        if (!lesson.hasVideo)
        {
            continue;
        }
        ++metrics.totalCount;
        const auto state = stateByLessonId.find(lesson.lessonId);
        if (state != stateByLessonId.end() && state->second.videoWatched)
        {
            ++metrics.valueCount;
        }
    }
    return metrics;
}

std::optional<CountMetrics> CourseMetricsService::computePointsQuarter(
    const drogon::orm::DbClientPtr &dbClient,
    int quarter,
    int userId,
    int courseId,
    ApiError &error) const
{
    if (quarter < 1 || quarter > kQuarterCount)
    {
        error = {drogon::k400BadRequest,
                 "Параметр 'quarter' должен быть числом от 1 до 4"};
        return std::nullopt;
    }

    const auto course = findUserCourse(dbClient, userId, courseId, error);
    if (!course.has_value())
    {
        return std::nullopt;
    }
    const auto trackId = findMatchingLessonTrack(dbClient, *course, error);
    if (!trackId.has_value())
    {
        return std::nullopt;
    }
    const auto lessons = loadCourseLessons(dbClient, courseId, error);
    if (!lessons.has_value())
    {
        return std::nullopt;
    }
    const auto userStates = loadUserLessonStates(dbClient, userId, *trackId, error);
    if (!userStates.has_value())
    {
        return std::nullopt;
    }

    std::unordered_map<int, UserLessonState> stateByLessonId;
    for (const auto &state : *userStates)
    {
        stateByLessonId[state.lessonId] = state;
    }

    const auto slice = quarterSliceFor(*lessons, quarter);
    double earned = 0.0;
    double total = 0.0;
    for (size_t index = slice.begin; index < slice.end; ++index)
    {
        const auto &lesson = (*lessons)[index];
        total += lesson.maxPoints;
        const auto state = stateByLessonId.find(lesson.lessonId);
        if (state != stateByLessonId.end())
        {
            earned += state->second.earnedPoints;
        }
    }

    return CountMetrics{roundCount(earned), roundCount(total)};
}

std::optional<CountMetrics> CourseMetricsService::computePointsYear(const drogon::orm::DbClientPtr &dbClient,
                                                                    int userId,
                                                                    int courseId,
                                                                    ApiError &error) const
{
    const auto course = findUserCourse(dbClient, userId, courseId, error);
    if (!course.has_value())
    {
        return std::nullopt;
    }
    const auto trackId = findMatchingLessonTrack(dbClient, *course, error);
    if (!trackId.has_value())
    {
        return std::nullopt;
    }
    const auto lessons = loadCourseLessons(dbClient, courseId, error);
    if (!lessons.has_value())
    {
        return std::nullopt;
    }
    const auto userStates = loadUserLessonStates(dbClient, userId, *trackId, error);
    if (!userStates.has_value())
    {
        return std::nullopt;
    }

    std::unordered_map<int, UserLessonState> stateByLessonId;
    for (const auto &state : *userStates)
    {
        stateByLessonId[state.lessonId] = state;
    }

    double earned = 0.0;
    double total = 0.0;
    for (const auto &lesson : *lessons)
    {
        total += lesson.maxPoints;
        const auto state = stateByLessonId.find(lesson.lessonId);
        if (state != stateByLessonId.end())
        {
            earned += state->second.earnedPoints;
        }
    }

    return CountMetrics{roundCount(earned), roundCount(total)};
}

std::optional<CountMetrics> CourseMetricsService::computeConspectsQuarter(
    const drogon::orm::DbClientPtr &dbClient,
    int quarter,
    int userId,
    int courseId,
    ApiError &error) const
{
    if (quarter < 1 || quarter > kQuarterCount)
    {
        error = {drogon::k400BadRequest,
                 "Параметр 'quarter' должен быть числом от 1 до 4"};
        return std::nullopt;
    }

    const auto course = findUserCourse(dbClient, userId, courseId, error);
    if (!course.has_value())
    {
        return std::nullopt;
    }
    const auto trackId = findMatchingLessonTrack(dbClient, *course, error);
    if (!trackId.has_value())
    {
        return std::nullopt;
    }
    const auto lessons = loadCourseLessons(dbClient, courseId, error);
    if (!lessons.has_value())
    {
        return std::nullopt;
    }
    const auto userStates = loadUserLessonStates(dbClient, userId, *trackId, error);
    if (!userStates.has_value())
    {
        return std::nullopt;
    }
    const auto visitedPrep = loadVisitedPreparationMaterials(dbClient, *trackId, error);
    if (!visitedPrep.has_value())
    {
        return std::nullopt;
    }

    std::unordered_map<int, UserLessonState> stateByLessonId;
    for (const auto &state : *userStates)
    {
        stateByLessonId[state.lessonId] = state;
    }

    const auto slice = quarterSliceFor(*lessons, quarter);
    CountMetrics metrics;
    for (size_t index = slice.begin; index < slice.end; ++index)
    {
        const auto &lesson = (*lessons)[index];
        if (lesson.lessonNumber.has_value())
        {
            ++metrics.totalCount;
            const auto state = stateByLessonId.find(lesson.lessonId);
            if (state != stateByLessonId.end() && state->second.translationVisited)
            {
                ++metrics.valueCount;
            }
        }

        if (lesson.hasPreparationMaterial)
        {
            ++metrics.totalCount;
            if (visitedPrep->contains(lesson.lessonId))
            {
                ++metrics.valueCount;
            }
        }
    }
    return metrics;
}

std::optional<CountMetrics> CourseMetricsService::computeConspectsYear(
    const drogon::orm::DbClientPtr &dbClient,
    int userId,
    int courseId,
    ApiError &error) const
{
    const auto course = findUserCourse(dbClient, userId, courseId, error);
    if (!course.has_value())
    {
        return std::nullopt;
    }
    const auto trackId = findMatchingLessonTrack(dbClient, *course, error);
    if (!trackId.has_value())
    {
        return std::nullopt;
    }
    const auto lessons = loadCourseLessons(dbClient, courseId, error);
    if (!lessons.has_value())
    {
        return std::nullopt;
    }
    const auto userStates = loadUserLessonStates(dbClient, userId, *trackId, error);
    if (!userStates.has_value())
    {
        return std::nullopt;
    }
    const auto visitedPrep = loadVisitedPreparationMaterials(dbClient, *trackId, error);
    if (!visitedPrep.has_value())
    {
        return std::nullopt;
    }

    std::unordered_map<int, UserLessonState> stateByLessonId;
    for (const auto &state : *userStates)
    {
        stateByLessonId[state.lessonId] = state;
    }

    CountMetrics metrics;
    for (const auto &lesson : *lessons)
    {
        if (lesson.lessonNumber.has_value())
        {
            ++metrics.totalCount;
            const auto state = stateByLessonId.find(lesson.lessonId);
            if (state != stateByLessonId.end() && state->second.translationVisited)
            {
                ++metrics.valueCount;
            }
        }

        if (lesson.hasPreparationMaterial)
        {
            ++metrics.totalCount;
            if (visitedPrep->contains(lesson.lessonId))
            {
                ++metrics.valueCount;
            }
        }
    }
    return metrics;
}
}  // namespace yearreporter::services
