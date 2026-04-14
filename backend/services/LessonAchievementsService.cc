#include "LessonAchievementsService.h"

#include <algorithm>
#include <limits>

#include "constants/progress_constants.h"

namespace yearreporter::services
{
namespace
{
struct RareLessonCandidate
{
    int lessonId{0};
    int solvedUsers{0};
    int notSolvedPercentage{0};
};

bool isTaskLesson(const CourseMetricsService::CourseLesson &lesson)
{
    return lesson.taskExpected || lesson.maxTaskCount > 0;
}
}  // namespace

std::optional<LessonAchievementsService::MetricsContext> LessonAchievementsService::loadContext(
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

    MetricsContext context;
    context.lessons = *lessons;
    context.userStates = *userStates;

    for (const auto &lesson : context.lessons)
    {
        context.lessonById.emplace(lesson.lessonId, lesson);
    }
    for (const auto &state : context.userStates)
    {
        context.stateByLessonId.emplace(state.lessonId, state);
    }

    return context;
}

std::vector<CourseMetricsService::CourseLesson> LessonAchievementsService::sliceLessons(
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

std::string LessonAchievementsService::makeLessonTitle(
    const CourseMetricsService::CourseLesson &lesson)
{
    if (lesson.lessonNumber.has_value())
    {
        return "Урок " + std::to_string(*lesson.lessonNumber);
    }
    return "Урок #" + std::to_string(lesson.lessonId);
}

std::optional<int> LessonAchievementsService::computeFullSolvedLessonsQuarter(
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

    int solvedLessonsCount = 0;
    for (const auto &lesson : lessons)
    {
        if (!isTaskLesson(lesson))
        {
            continue;
        }

        const auto stateIt = context->stateByLessonId.find(lesson.lessonId);
        if (stateIt != context->stateByLessonId.end() && stateIt->second.solved)
        {
            ++solvedLessonsCount;
        }
    }

    return solvedLessonsCount;
}

std::optional<int> LessonAchievementsService::computeFullSolvedLessonsYear(
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

    int solvedLessonsCount = 0;
    for (const auto &lesson : context->lessons)
    {
        if (!isTaskLesson(lesson))
        {
            continue;
        }

        const auto stateIt = context->stateByLessonId.find(lesson.lessonId);
        if (stateIt != context->stateByLessonId.end() && stateIt->second.solved)
        {
            ++solvedLessonsCount;
        }
    }

    return solvedLessonsCount;
}

std::optional<bool> LessonAchievementsService::computeVisitedAllLessonsQuarter(
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

    for (const auto &lesson : lessons)
    {
        if (!context->stateByLessonId.contains(lesson.lessonId))
        {
            return false;
        }
    }

    return true;
}

std::optional<bool> LessonAchievementsService::computeVisitedAllLessonsYear(
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

    for (const auto &lesson : context->lessons)
    {
        if (!context->stateByLessonId.contains(lesson.lessonId))
        {
            return false;
        }
    }

    return true;
}

std::optional<RareLessonsResult> LessonAchievementsService::computeRareLessons(
    const drogon::orm::DbClientPtr &dbClient,
    int courseId,
    const std::vector<CourseMetricsService::CourseLesson> &lessons,
    const std::unordered_map<int, CourseMetricsService::UserLessonState> &stateByLessonId,
    ApiError &error) const
{
    try
    {
        std::vector<RareLessonCandidate> candidates;
        candidates.reserve(lessons.size());

        for (const auto &lesson : lessons)
        {
            if (!isTaskLesson(lesson))
            {
                continue;
            }

            const auto stateIt = stateByLessonId.find(lesson.lessonId);
            if (stateIt == stateByLessonId.end() || !stateIt->second.solved)
            {
                continue;
            }

            candidates.push_back(RareLessonCandidate{.lessonId = lesson.lessonId});
        }

        RareLessonsResult result;
        result.items.resize(3);

        if (candidates.empty())
        {
            return result;
        }

        const auto totalUsersResult = dbClient->execSqlSync(
            "select count(distinct user_id)::int as total_users "
            "from public.user_courses "
            "where course_id = $1",
            courseId);

        const auto totalUsers =
            totalUsersResult.empty() ? 0 : totalUsersResult.front()["total_users"].as<int>();

        if (totalUsers <= 0)
        {
            return result;
        }

        const auto solvedUsersResult = dbClient->execSqlSync(
            "select ul.lesson_id, "
            "count(distinct ul.user_id) filter (where coalesce(ul.solved, false) = true)::int as solved_users "
            "from public.user_lessons ul "
            "join public.user_courses uc on uc.user_id = ul.user_id and uc.course_id = $1 "
            "join public.lessons l on l.id = ul.lesson_id and l.course_id = $1 "
            "group by ul.lesson_id",
            courseId);

        std::unordered_map<int, int> solvedUsersByLessonId;
        for (const auto &row : solvedUsersResult)
        {
            solvedUsersByLessonId.emplace(row["lesson_id"].as<int>(), row["solved_users"].as<int>());
        }

        for (auto &candidate : candidates)
        {
            candidate.solvedUsers = solvedUsersByLessonId[candidate.lessonId];
            candidate.notSolvedPercentage = CourseMetricsService::roundCount(
                (static_cast<double>(totalUsers - candidate.solvedUsers) * 100.0) /
                static_cast<double>(totalUsers));
        }

        const auto lessonOrderKey =
            [&](int lessonId)
        {
            const auto lessonIt = std::find_if(
                lessons.begin(),
                lessons.end(),
                [&](const auto &lesson) { return lesson.lessonId == lessonId; });

            if (lessonIt == lessons.end())
            {
                return std::pair<int, int>{std::numeric_limits<int>::max(), lessonId};
            }

            const auto lessonNumber = lessonIt->lessonNumber.has_value()
                                          ? *lessonIt->lessonNumber
                                          : std::numeric_limits<int>::max();
            return std::pair<int, int>{lessonNumber, lessonId};
        };

        std::sort(candidates.begin(),
                  candidates.end(),
                  [&](const RareLessonCandidate &lhs,
                      const RareLessonCandidate &rhs)
                  {
                      if (lhs.solvedUsers != rhs.solvedUsers)
                      {
                          return lhs.solvedUsers < rhs.solvedUsers;
                      }
                      return lessonOrderKey(lhs.lessonId) < lessonOrderKey(rhs.lessonId);
                  });

        const auto topCount = std::min<size_t>(3, candidates.size());
        for (size_t index = 0; index < topCount; ++index)
        {
            const auto lessonIt = std::find_if(
                lessons.begin(),
                lessons.end(),
                [&](const auto &lesson) { return lesson.lessonId == candidates[index].lessonId; });
            if (lessonIt == lessons.end())
            {
                continue;
            }

            result.items[index] = RareLesson{
                .title = makeLessonTitle(*lessonIt),
                .notSolvedPercentage = candidates[index].notSolvedPercentage};
        }

        return result;
    }
    catch (const std::exception &e)
    {
        error = {drogon::k500InternalServerError,
                 "Ошибка расчёта топ-3 сложных уроков: " + std::string(e.what())};
        return std::nullopt;
    }
}

std::optional<RareLessonsResult> LessonAchievementsService::computeRareLessonsQuarter(
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

    return computeRareLessons(dbClient, courseId, lessons, context->stateByLessonId, error);
}

std::optional<RareLessonsResult> LessonAchievementsService::computeRareLessonsYear(
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

    return computeRareLessons(dbClient, courseId, context->lessons, context->stateByLessonId, error);
}
}  // namespace yearreporter::services
