#include "SchoolRankingsService.h"

#include <algorithm>
#include <cmath>
#include <sstream>

#include "constants/progress_constants.h"

namespace yearreporter::services
{
namespace
{
using Lesson = CourseMetricsService::CourseLesson;
using QuarterSlice = CourseMetricsService::QuarterSlice;

std::string buildDisplayText(const std::string &mode, int value)
{
    if (mode == "place")
    {
        return "Ты " + std::to_string(value) + " по своей школе по просмотренным занятиям";
    }
    if (mode == "top")
    {
        return "Ты в топ-" + std::to_string(value) + " по своей школе по просмотренным занятиям";
    }
    if (mode == "share")
    {
        return "Столько же просмотренных занятий, как у " + std::to_string(value) +
               "% учеников твоей школы";
    }
    return "";
}
}  // namespace

int SchoolRankingsService::roundPercent(int value, int total)
{
    if (total <= 0)
    {
        return 0;
    }

    return static_cast<int>(std::lround(static_cast<double>(value) * 100.0 /
                                        static_cast<double>(total)));
}

std::string SchoolRankingsService::buildLessonIdList(const std::vector<Lesson> &lessons)
{
    std::ostringstream out;
    bool first = true;
    for (const auto &lesson : lessons)
    {
        if (!lesson.hasVideo)
        {
            continue;
        }

        if (!first)
        {
            out << ", ";
        }
        out << lesson.lessonId;
        first = false;
    }
    return out.str();
}

std::optional<std::vector<Lesson>> SchoolRankingsService::loadQuarterLessons(
    const drogon::orm::DbClientPtr &dbClient,
    int quarter,
    int courseId,
    ApiError &error) const
{
    if (quarter < 1 || quarter > yearreporter::constants::progress::kQuarterCount)
    {
        error = {drogon::k400BadRequest,
                 "Параметр 'quarter' должен быть числом от 1 до 4"};
        return std::nullopt;
    }

    CourseMetricsService courseService;
    const auto lessons = courseService.loadCourseLessons(dbClient, courseId, error);
    if (!lessons.has_value())
    {
        return std::nullopt;
    }

    const QuarterSlice slice = CourseMetricsService::quarterSliceFor(*lessons, quarter);
    return std::vector<Lesson>(lessons->begin() + static_cast<std::ptrdiff_t>(slice.begin),
                               lessons->begin() + static_cast<std::ptrdiff_t>(slice.end));
}

std::optional<std::vector<Lesson>> SchoolRankingsService::loadYearLessons(
    const drogon::orm::DbClientPtr &dbClient,
    int courseId,
    ApiError &error) const
{
    CourseMetricsService courseService;
    return courseService.loadCourseLessons(dbClient, courseId, error);
}

std::optional<std::string> SchoolRankingsService::loadUserSchool(
    const drogon::orm::DbClientPtr &dbClient,
    int userId,
    int courseId,
    ApiError &error) const
{
    try
    {
        const auto result = dbClient->execSqlSync(
            "select coalesce(nullif(s.\"Школа\", ''), nullif(cs.\"Школа\", '')) as school_name "
            "from public.user_courses uc "
            "left join public.students_of_interest s on s.id = uc.user_id "
            "left join public.courses_stats cs on cs.user_id = uc.user_id and cs.\"Курс\" = uc.course_id "
            "where uc.user_id = $1 and uc.course_id = $2 "
            "limit 1",
            userId,
            courseId);

        if (result.empty() || result.front()["school_name"].isNull())
        {
            error = {drogon::k404NotFound, "Для пользователя не найдена школа"};
            return std::nullopt;
        }

        const auto schoolName = result.front()["school_name"].as<std::string>();
        if (schoolName.empty())
        {
            error = {drogon::k404NotFound, "Для пользователя не найдена школа"};
            return std::nullopt;
        }

        return schoolName;
    }
    catch (const std::exception &e)
    {
        error = {drogon::k500InternalServerError,
                 "Ошибка загрузки школы пользователя: " + std::string(e.what())};
        return std::nullopt;
    }
}

std::optional<std::vector<SchoolRankingsService::SchoolUserVideoCount>>
SchoolRankingsService::loadSchoolVideoCounts(const drogon::orm::DbClientPtr &dbClient,
                                             int courseId,
                                             const std::string &schoolName,
                                             const std::vector<Lesson> &lessons,
                                             ApiError &error) const
{
    const auto lessonIdList = buildLessonIdList(lessons);
    if (lessonIdList.empty())
    {
        error = {drogon::k404NotFound, "Для выбранного периода не найдены видеоуроки"};
        return std::nullopt;
    }

    try
    {
        const auto result = dbClient->execSqlSync(
            "with school_users as ("
            "    select distinct uc.user_id "
            "    from public.user_courses uc "
            "    left join public.students_of_interest s on s.id = uc.user_id "
            "    left join public.courses_stats cs on cs.user_id = uc.user_id and cs.\"Курс\" = uc.course_id "
            "    where uc.course_id = $1 "
            "      and coalesce(nullif(s.\"Школа\", ''), nullif(cs.\"Школа\", '')) = $2"
            "), lesson_watch as ("
            "    select ul.user_id, ul.lesson_id, "
            "           bool_or(coalesce(ul.video_visited, false) or coalesce(ul.video_viewed, false)) as watched "
            "    from public.user_lessons ul "
            "    join school_users su on su.user_id = ul.user_id "
            "    where ul.lesson_id in (" + lessonIdList + ") "
            "    group by ul.user_id, ul.lesson_id"
            ") "
            "select su.user_id, "
            "       count(*) filter (where coalesce(lw.watched, false))::int as watched_lessons_count "
            "from school_users su "
            "left join lesson_watch lw on lw.user_id = su.user_id "
            "group by su.user_id",
            courseId,
            schoolName);

        std::vector<SchoolUserVideoCount> counts;
        counts.reserve(result.size());
        for (const auto &row : result)
        {
            counts.push_back(SchoolUserVideoCount{
                .userId = row["user_id"].as<int>(),
                .watchedLessonsCount = row["watched_lessons_count"].as<int>()});
        }

        if (counts.empty())
        {
            error = {drogon::k404NotFound, "Для школы не найдены пользователи курса"};
            return std::nullopt;
        }

        return counts;
    }
    catch (const std::exception &e)
    {
        error = {drogon::k500InternalServerError,
                 "Ошибка расчёта ранга по школе: " + std::string(e.what())};
        return std::nullopt;
    }
}

SchoolVideoRankResult SchoolRankingsService::buildResult(const std::string &schoolName,
                                                         const std::vector<Lesson> &lessons,
                                                         const std::vector<SchoolUserVideoCount> &counts,
                                                         int userId,
                                                         ApiError &error)
{
    SchoolVideoRankResult result;
    result.schoolName = schoolName;
    for (const auto &lesson : lessons)
    {
        if (lesson.hasVideo)
        {
            ++result.totalVideoLessonsCount;
        }
    }

    auto ordered = counts;
    std::sort(ordered.begin(),
              ordered.end(),
              [](const SchoolUserVideoCount &lhs, const SchoolUserVideoCount &rhs)
              {
                  if (lhs.watchedLessonsCount != rhs.watchedLessonsCount)
                  {
                      return lhs.watchedLessonsCount > rhs.watchedLessonsCount;
                  }
                  return lhs.userId < rhs.userId;
              });

    result.schoolUsersCount = static_cast<int>(ordered.size());

    int currentRank = 0;
    int previousCount = -1;
    for (size_t index = 0; index < ordered.size(); ++index)
    {
        if (ordered[index].watchedLessonsCount != previousCount)
        {
            currentRank = static_cast<int>(index) + 1;
            previousCount = ordered[index].watchedLessonsCount;
        }

        if (ordered[index].userId == userId)
        {
            result.userRank = currentRank;
            result.watchedLessonsCount = ordered[index].watchedLessonsCount;
            break;
        }
    }

    if (result.userRank == 0)
    {
        error = {drogon::k404NotFound, "Пользователь не найден среди учеников своей школы"};
        return {};
    }

    result.sameValueUsersCount = static_cast<int>(std::count_if(
        ordered.begin(),
        ordered.end(),
        [&](const SchoolUserVideoCount &item)
        { return item.watchedLessonsCount == result.watchedLessonsCount; }));

    if (result.userRank <= 10)
    {
        result.displayMode = "place";
        result.displayValue = result.userRank;
    }
    else if (result.userRank <= 100)
    {
        result.displayMode = "top";
        result.displayValue = static_cast<int>(std::ceil(result.userRank / 10.0) * 10.0);
        if (result.displayValue > 100)
        {
            result.displayValue = 100;
        }
    }
    else
    {
        const auto sameValueShare = roundPercent(result.sameValueUsersCount, result.schoolUsersCount);
        if (sameValueShare > 0 && sameValueShare <= 70)
        {
            result.displayMode = "share";
            result.displayValue = sameValueShare;
        }
        else
        {
            result.displayMode = "hidden";
            result.displayValue = 0;
        }
    }

    result.displayText = buildDisplayText(result.displayMode, result.displayValue);
    return result;
}

std::optional<SchoolVideoRankResult> SchoolRankingsService::computeVideoRankQuarter(
    const drogon::orm::DbClientPtr &dbClient,
    int quarter,
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

    const auto lessons = loadQuarterLessons(dbClient, quarter, courseId, error);
    if (!lessons.has_value())
    {
        return std::nullopt;
    }

    const auto schoolName = loadUserSchool(dbClient, userId, courseId, error);
    if (!schoolName.has_value())
    {
        return std::nullopt;
    }

    const auto counts = loadSchoolVideoCounts(dbClient, courseId, *schoolName, *lessons, error);
    if (!counts.has_value())
    {
        return std::nullopt;
    }

    auto result = buildResult(*schoolName, *lessons, *counts, userId, error);
    if (!error.reason.empty())
    {
        return std::nullopt;
    }
    return result;
}

std::optional<SchoolVideoRankResult> SchoolRankingsService::computeVideoRankYear(
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

    const auto lessons = loadYearLessons(dbClient, courseId, error);
    if (!lessons.has_value())
    {
        return std::nullopt;
    }

    const auto schoolName = loadUserSchool(dbClient, userId, courseId, error);
    if (!schoolName.has_value())
    {
        return std::nullopt;
    }

    const auto counts = loadSchoolVideoCounts(dbClient, courseId, *schoolName, *lessons, error);
    if (!counts.has_value())
    {
        return std::nullopt;
    }

    auto result = buildResult(*schoolName, *lessons, *counts, userId, error);
    if (!error.reason.empty())
    {
        return std::nullopt;
    }
    return result;
}
}  // namespace yearreporter::services
