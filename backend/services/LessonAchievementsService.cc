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

bool isVisitedLesson(const CourseMetricsService::UserLessonState &state)
{
    return state.translationVisited || state.videoWatched;
}

std::string makeExclusiveInsight(const std::string &title,
                                 const std::string &periodPhrase,
                                 int visitedUsersCount,
                                 int totalUsersCount)
{
    return "Пока другие отдыхали, ты изучал " + title + ". Это занятие оказалось самым "
           "эксклюзивным " +
           periodPhrase + " — лишь " + std::to_string(visitedUsersCount) + " из " +
           std::to_string(totalUsersCount) +
           " учеников добрались до этой темы. Твоя жажда знаний помогает ракете "
           "прокладывать маршруты там, где другие боятся лететь.";
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

std::optional<RarestVisitedLessonResult> LessonAchievementsService::computeRarestVisitedLesson(
    const drogon::orm::DbClientPtr &dbClient,
    int courseId,
    const std::vector<CourseMetricsService::CourseLesson> &lessons,
    const std::unordered_map<int, CourseMetricsService::UserLessonState> &stateByLessonId,
    const std::string &periodPhrase,
    ApiError &error) const
{
    try
    {
        std::vector<int> visitedLessonIds;
        visitedLessonIds.reserve(lessons.size());

        for (const auto &lesson : lessons)
        {
            if (!lesson.lessonNumber.has_value())
            {
                continue;
            }

            const auto stateIt = stateByLessonId.find(lesson.lessonId);
            if (stateIt == stateByLessonId.end() || !isVisitedLesson(stateIt->second))
            {
                continue;
            }

            visitedLessonIds.push_back(lesson.lessonId);
        }

        if (visitedLessonIds.empty())
        {
            error = {drogon::k404NotFound,
                     "Для пользователя не найдено посещённых уроков в выбранном периоде"};
            return std::nullopt;
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
            error = {drogon::k404NotFound, "Для курса не найдены пользователи"};
            return std::nullopt;
        }

        const auto visitedUsersResult = dbClient->execSqlSync(
            "select ul.lesson_id, "
            "count(distinct ul.user_id)::int as visited_users "
            "from public.user_lessons ul "
            "join public.lessons l on l.id = ul.lesson_id and l.course_id = $1 "
            "where coalesce(ul.translation_visited, false) = true "
            "   or coalesce(ul.video_visited, false) = true "
            "   or coalesce(ul.video_viewed, false) = true "
            "group by ul.lesson_id",
            courseId);

        std::unordered_map<int, int> visitedUsersByLessonId;
        for (const auto &row : visitedUsersResult)
        {
            visitedUsersByLessonId.emplace(row["lesson_id"].as<int>(), row["visited_users"].as<int>());
        }

        const auto lessonOrderKey = [&](int lessonId)
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

        std::sort(visitedLessonIds.begin(),
                  visitedLessonIds.end(),
                  [&](int lhs, int rhs)
                  {
                      const auto lhsVisited = visitedUsersByLessonId[lhs];
                      const auto rhsVisited = visitedUsersByLessonId[rhs];
                      if (lhsVisited != rhsVisited)
                      {
                          return lhsVisited < rhsVisited;
                      }
                      return lessonOrderKey(lhs) < lessonOrderKey(rhs);
                  });

        const auto winnerId = visitedLessonIds.front();
        const auto lessonIt = std::find_if(
            lessons.begin(),
            lessons.end(),
            [&](const auto &lesson) { return lesson.lessonId == winnerId; });
        if (lessonIt == lessons.end())
        {
            error = {drogon::k404NotFound, "Не удалось определить урок для storytelling-блока"};
            return std::nullopt;
        }

        const auto visitedUsersCount = visitedUsersByLessonId[winnerId];
        const auto visitedPercentage = CourseMetricsService::roundCount(
            (static_cast<double>(visitedUsersCount) * 100.0) / static_cast<double>(totalUsers));

        return RarestVisitedLessonResult{
            .title = makeLessonTitle(*lessonIt),
            .visitedPercentage = visitedPercentage,
            .visitedUsersCount = visitedUsersCount,
            .totalUsersCount = totalUsers,
            .reportName = "Ты посетил самый редкий урок",
            .insight = makeExclusiveInsight(makeLessonTitle(*lessonIt),
                                           periodPhrase,
                                           visitedUsersCount,
                                           totalUsers)};
    }
    catch (const std::exception &e)
    {
        error = {drogon::k500InternalServerError,
                 "Ошибка расчёта редкости посещённого урока: " + std::string(e.what())};
        return std::nullopt;
    }
}

std::optional<RarestVisitedLessonResult> LessonAchievementsService::computeRarestVisitedLessonQuarter(
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

    return computeRarestVisitedLesson(
        dbClient, courseId, lessons, context->stateByLessonId, "в этой четверти", error);
}

std::optional<RarestVisitedLessonResult> LessonAchievementsService::computeRarestVisitedLessonYear(
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

    return computeRarestVisitedLesson(
        dbClient, courseId, context->lessons, context->stateByLessonId, "в этом году", error);
}
}  // namespace yearreporter::services
