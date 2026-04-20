#include "SocialRankingsService.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <unordered_map>

#include "constants/progress_constants.h"

namespace yearreporter::services
{
namespace
{
using Lesson = CourseMetricsService::CourseLesson;
using QuarterSlice = CourseMetricsService::QuarterSlice;

std::string buildRegionPointsDisplayText(const std::string &mode, int value)
{
    if (mode == "place")
    {
        return "Ты " + std::to_string(value) + " в своем регионе по баллам";
    }
    if (mode == "top")
    {
        return "Ты входишь в топ " + std::to_string(value) + " в своем регионе по баллам";
    }
    if (mode == "share")
    {
        return "Ты в " + std::to_string(value) + "% в своем регионе по баллам";
    }
    return "";
}

std::string buildClassTextDisplayText(const std::string &mode, int value)
{
    if (mode == "place")
    {
        return "Ты " + std::to_string(value) + " в классе по глубине изучения текстов";
    }
    if (mode == "top")
    {
        return "Ты в топ-" + std::to_string(value) + " в классе по глубине изучения текстов";
    }
    if (mode == "share")
    {
        return "Ты в " + std::to_string(value) + "% в классе по глубине изучения текстов";
    }
    return "";
}
}  // namespace

int SocialRankingsService::roundPercent(int value, int total)
{
    if (total <= 0)
    {
        return 0;
    }

    return static_cast<int>(std::lround(static_cast<double>(value) * 100.0 /
                                        static_cast<double>(total)));
}

double SocialRankingsService::roundToTwoDigits(double value)
{
    return std::round(value * 100.0) / 100.0;
}

std::string SocialRankingsService::formatFixed2(double value)
{
    std::ostringstream out;
    out.setf(std::ios::fixed);
    out << std::setprecision(2) << value;
    return out.str();
}

std::string SocialRankingsService::buildLessonIdList(const std::vector<Lesson> &lessons)
{
    std::ostringstream out;
    bool first = true;
    for (const auto &lesson : lessons)
    {
        if (!first)
        {
            out << ", ";
        }
        out << lesson.lessonId;
        first = false;
    }
    return out.str();
}

std::optional<std::vector<Lesson>> SocialRankingsService::loadQuarterLessons(
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

std::optional<std::vector<Lesson>> SocialRankingsService::loadYearLessons(
    const drogon::orm::DbClientPtr &dbClient,
    int courseId,
    ApiError &error) const
{
    CourseMetricsService courseService;
    return courseService.loadCourseLessons(dbClient, courseId, error);
}

std::optional<std::string> SocialRankingsService::loadUserRegion(
    const drogon::orm::DbClientPtr &dbClient,
    int userId,
    int courseId,
    ApiError &error) const
{
    try
    {
        const auto result = dbClient->execSqlSync(
            "select coalesce(nullif(s.\"Регион\", ''), nullif(cs.\"Регион\", '')) as region_name "
            "from public.user_courses uc "
            "left join public.students_of_interest s on s.id = uc.user_id "
            "left join public.courses_stats cs on cs.user_id = uc.user_id and cs.\"Курс\" = uc.course_id "
            "where uc.user_id = $1 and uc.course_id = $2 "
            "limit 1",
            userId,
            courseId);

        if (result.empty() || result.front()["region_name"].isNull())
        {
            error = {drogon::k404NotFound, "Для пользователя не найден регион"};
            return std::nullopt;
        }

        const auto regionName = result.front()["region_name"].as<std::string>();
        if (regionName.empty())
        {
            error = {drogon::k404NotFound, "Для пользователя не найден регион"};
            return std::nullopt;
        }

        return regionName;
    }
    catch (const std::exception &e)
    {
        error = {drogon::k500InternalServerError,
                 "Ошибка загрузки региона пользователя: " + std::string(e.what())};
        return std::nullopt;
    }
}

std::optional<std::string> SocialRankingsService::loadUserClass(
    const drogon::orm::DbClientPtr &dbClient,
    int userId,
    int courseId,
    ApiError &error) const
{
    try
    {
        const auto result = dbClient->execSqlSync(
            "select coalesce(nullif(cs.\"Класс\", ''), nullif(uwd.\"Класс\", '')) as class_name "
            "from public.user_courses uc "
            "left join public.courses_stats cs on cs.user_id = uc.user_id and cs.\"Курс\" = uc.course_id "
            "left join public.user_watched_depth uwd on uwd.user_id = uc.user_id "
            "where uc.user_id = $1 and uc.course_id = $2 "
            "limit 1",
            userId,
            courseId);

        if (result.empty() || result.front()["class_name"].isNull())
        {
            error = {drogon::k404NotFound, "Для пользователя не найден класс"};
            return std::nullopt;
        }

        const auto className = result.front()["class_name"].as<std::string>();
        if (className.empty())
        {
            error = {drogon::k404NotFound, "Для пользователя не найден класс"};
            return std::nullopt;
        }

        return className;
    }
    catch (const std::exception &e)
    {
        error = {drogon::k500InternalServerError,
                 "Ошибка загрузки класса пользователя: " + std::string(e.what())};
        return std::nullopt;
    }
}

std::optional<std::vector<SocialRankingsService::UserDoubleMetric>>
SocialRankingsService::loadRegionPointsMetrics(const drogon::orm::DbClientPtr &dbClient,
                                               int courseId,
                                               const std::string &regionName,
                                               const std::vector<Lesson> &lessons,
                                               ApiError &error) const
{
    const auto lessonIdList = buildLessonIdList(lessons);
    if (lessonIdList.empty())
    {
        error = {drogon::k404NotFound, "Для выбранного периода не найдены уроки"};
        return std::nullopt;
    }

    try
    {
        const auto result = dbClient->execSqlSync(
            "with region_users as ("
            "    select distinct uc.user_id "
            "    from public.user_courses uc "
            "    left join public.students_of_interest s on s.id = uc.user_id "
            "    left join public.courses_stats cs on cs.user_id = uc.user_id and cs.\"Курс\" = uc.course_id "
            "    where uc.course_id = $1 "
            "      and coalesce(nullif(s.\"Регион\", ''), nullif(cs.\"Регион\", '')) = $2"
            "), per_lesson as ("
            "    select ul.user_id, ul.lesson_id, max(coalesce(ul.wk_points, 0)) as lesson_points "
            "    from public.user_lessons ul "
            "    join region_users ru on ru.user_id = ul.user_id "
            "    where ul.lesson_id in (" + lessonIdList + ") "
            "    group by ul.user_id, ul.lesson_id"
            ") "
            "select ru.user_id, "
            "       coalesce(round(sum(coalesce(pl.lesson_points, 0))::numeric, 2), 0)::float8 as metric_value "
            "from region_users ru "
            "left join per_lesson pl on pl.user_id = ru.user_id "
            "group by ru.user_id",
            courseId,
            regionName);

        std::vector<UserDoubleMetric> metrics;
        metrics.reserve(result.size());
        for (const auto &row : result)
        {
            metrics.push_back(UserDoubleMetric{
                .userId = row["user_id"].as<int>(),
                .value = row["metric_value"].as<double>()});
        }

        if (metrics.empty())
        {
            error = {drogon::k404NotFound, "Для региона не найдены пользователи курса"};
            return std::nullopt;
        }

        return metrics;
    }
    catch (const std::exception &e)
    {
        error = {drogon::k500InternalServerError,
                 "Ошибка расчёта ранга по региону: " + std::string(e.what())};
        return std::nullopt;
    }
}

std::optional<std::vector<SocialRankingsService::UserIntMetric>>
SocialRankingsService::loadClassTextMetrics(const drogon::orm::DbClientPtr &dbClient,
                                            int courseId,
                                            const std::string &className,
                                            const std::vector<Lesson> &lessons,
                                            ApiError &error) const
{
    const auto lessonIdList = buildLessonIdList(lessons);
    if (lessonIdList.empty())
    {
        error = {drogon::k404NotFound, "Для выбранного периода не найдены уроки"};
        return std::nullopt;
    }

    try
    {
        const auto classUsersResult = dbClient->execSqlSync(
            "select distinct uc.user_id "
            "from public.user_courses uc "
            "left join public.courses_stats cs on cs.user_id = uc.user_id and cs.\"Курс\" = uc.course_id "
            "left join public.user_watched_depth uwd on uwd.user_id = uc.user_id "
            "where uc.course_id = $1 "
            "  and coalesce(nullif(cs.\"Класс\", ''), nullif(uwd.\"Класс\", '')) = $2",
            courseId,
            className);

        if (classUsersResult.empty())
        {
            error = {drogon::k404NotFound, "Для класса не найдены пользователи курса"};
            return std::nullopt;
        }

        std::unordered_map<int, UserIntMetric> metricsByUserId;
        for (const auto &row : classUsersResult)
        {
            const auto userId = row["user_id"].as<int>();
            metricsByUserId.emplace(userId, UserIntMetric{.userId = userId, .value = 0});
        }

        std::unordered_map<int, Lesson> lessonById;
        for (const auto &lesson : lessons)
        {
            lessonById.emplace(lesson.lessonId, lesson);
        }

        const auto conspectsResult = dbClient->execSqlSync(
            "select ul.user_id, ul.lesson_id, "
            "       bool_or(coalesce(ul.translation_visited, false)) as conspect_done "
            "from public.user_lessons ul "
            "where ul.user_id in ("
            "    select distinct uc.user_id "
            "    from public.user_courses uc "
            "    left join public.courses_stats cs on cs.user_id = uc.user_id and cs.\"Курс\" = uc.course_id "
            "    left join public.user_watched_depth uwd on uwd.user_id = uc.user_id "
            "    where uc.course_id = $1 "
            "      and coalesce(nullif(cs.\"Класс\", ''), nullif(uwd.\"Класс\", '')) = $2"
            ") "
            "and ul.lesson_id in (" + lessonIdList + ") "
            "group by ul.user_id, ul.lesson_id",
            courseId,
            className);

        for (const auto &row : conspectsResult)
        {
            if (!row["conspect_done"].as<bool>())
            {
                continue;
            }

            const auto lessonId = row["lesson_id"].as<int>();
            const auto lessonIt = lessonById.find(lessonId);
            if (lessonIt == lessonById.end() || !lessonIt->second.lessonNumber.has_value())
            {
                continue;
            }

            metricsByUserId[row["user_id"].as<int>()].value += 1;
        }

        const auto materialsResult = dbClient->execSqlSync(
            "select a.user_id, a.lesson_id "
            "from public.wk_users_courses_actions a "
            "where a.action = 'visit_preparation_material' "
            "  and a.user_id in ("
            "      select distinct uc.user_id "
            "      from public.user_courses uc "
            "      left join public.courses_stats cs on cs.user_id = uc.user_id and cs.\"Курс\" = uc.course_id "
            "      left join public.user_watched_depth uwd on uwd.user_id = uc.user_id "
            "      where uc.course_id = $1 "
            "        and coalesce(nullif(cs.\"Класс\", ''), nullif(uwd.\"Класс\", '')) = $2"
            "  ) "
            "  and a.lesson_id in (" + lessonIdList + ") "
            "group by a.user_id, a.lesson_id",
            courseId,
            className);

        for (const auto &row : materialsResult)
        {
            const auto lessonId = row["lesson_id"].as<int>();
            const auto lessonIt = lessonById.find(lessonId);
            if (lessonIt == lessonById.end() || !lessonIt->second.hasPreparationMaterial)
            {
                continue;
            }

            metricsByUserId[row["user_id"].as<int>()].value += 1;
        }

        std::vector<UserIntMetric> metrics;
        metrics.reserve(metricsByUserId.size());
        for (const auto &[_, metric] : metricsByUserId)
        {
            metrics.push_back(metric);
        }
        return metrics;
    }
    catch (const std::exception &e)
    {
        error = {drogon::k500InternalServerError,
                 "Ошибка расчёта ранга по классу: " + std::string(e.what())};
        return std::nullopt;
    }
}

std::optional<RegionPointsRankResult> SocialRankingsService::computeRegionPointsRankQuarter(
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

    const auto regionName = loadUserRegion(dbClient, userId, courseId, error);
    if (!regionName.has_value())
    {
        return std::nullopt;
    }

    const auto metrics = loadRegionPointsMetrics(dbClient, courseId, *regionName, *lessons, error);
    if (!metrics.has_value())
    {
        return std::nullopt;
    }

    RegionPointsRankResult result;
    result.regionName = *regionName;
    for (const auto &lesson : *lessons)
    {
        result.totalPoints += lesson.maxPoints;
    }
    result.totalPoints = roundToTwoDigits(result.totalPoints);
    result.totalPointsDisplay = formatFixed2(result.totalPoints);

    auto ordered = *metrics;
    std::sort(ordered.begin(),
              ordered.end(),
              [](const UserDoubleMetric &lhs, const UserDoubleMetric &rhs)
              {
                  if (lhs.value != rhs.value)
                  {
                      return lhs.value > rhs.value;
                  }
                  return lhs.userId < rhs.userId;
              });

    result.regionUsersCount = static_cast<int>(ordered.size());
    int currentRank = 0;
    double previousValue = -1.0;
    for (size_t index = 0; index < ordered.size(); ++index)
    {
        if (std::fabs(ordered[index].value - previousValue) > 1e-9)
        {
            currentRank = static_cast<int>(index) + 1;
            previousValue = ordered[index].value;
        }

        if (ordered[index].userId == userId)
        {
            result.userRank = currentRank;
            result.earnedPoints = roundToTwoDigits(ordered[index].value);
            result.earnedPointsDisplay = formatFixed2(result.earnedPoints);
            break;
        }
    }

    if (result.userRank == 0)
    {
        error = {drogon::k404NotFound, "Пользователь не найден среди учеников своего региона"};
        return std::nullopt;
    }

    result.sameValueUsersCount = static_cast<int>(std::count_if(
        ordered.begin(),
        ordered.end(),
        [&](const UserDoubleMetric &item) { return std::fabs(item.value - result.earnedPoints) <= 1e-9; }));

    if (result.userRank <= 10)
    {
        result.displayMode = "place";
        result.displayValue = result.userRank;
    }
    else if (result.userRank <= 100)
    {
        result.displayMode = "top";
        result.displayValue = std::min(100, static_cast<int>(std::ceil(result.userRank / 10.0) * 10.0));
    }
    else
    {
        const auto sameValueShare = roundPercent(result.sameValueUsersCount, result.regionUsersCount);
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

    result.displayText = buildRegionPointsDisplayText(result.displayMode, result.displayValue);
    return result;
}

std::optional<RegionPointsRankResult> SocialRankingsService::computeRegionPointsRankYear(
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

    const auto regionName = loadUserRegion(dbClient, userId, courseId, error);
    if (!regionName.has_value())
    {
        return std::nullopt;
    }

    const auto metrics = loadRegionPointsMetrics(dbClient, courseId, *regionName, *lessons, error);
    if (!metrics.has_value())
    {
        return std::nullopt;
    }

    RegionPointsRankResult result;
    result.regionName = *regionName;
    for (const auto &lesson : *lessons)
    {
        result.totalPoints += lesson.maxPoints;
    }
    result.totalPoints = roundToTwoDigits(result.totalPoints);
    result.totalPointsDisplay = formatFixed2(result.totalPoints);

    auto ordered = *metrics;
    std::sort(ordered.begin(),
              ordered.end(),
              [](const UserDoubleMetric &lhs, const UserDoubleMetric &rhs)
              {
                  if (lhs.value != rhs.value)
                  {
                      return lhs.value > rhs.value;
                  }
                  return lhs.userId < rhs.userId;
              });

    result.regionUsersCount = static_cast<int>(ordered.size());
    int currentRank = 0;
    double previousValue = -1.0;
    for (size_t index = 0; index < ordered.size(); ++index)
    {
        if (std::fabs(ordered[index].value - previousValue) > 1e-9)
        {
            currentRank = static_cast<int>(index) + 1;
            previousValue = ordered[index].value;
        }

        if (ordered[index].userId == userId)
        {
            result.userRank = currentRank;
            result.earnedPoints = roundToTwoDigits(ordered[index].value);
            result.earnedPointsDisplay = formatFixed2(result.earnedPoints);
            break;
        }
    }

    if (result.userRank == 0)
    {
        error = {drogon::k404NotFound, "Пользователь не найден среди учеников своего региона"};
        return std::nullopt;
    }

    result.sameValueUsersCount = static_cast<int>(std::count_if(
        ordered.begin(),
        ordered.end(),
        [&](const UserDoubleMetric &item) { return std::fabs(item.value - result.earnedPoints) <= 1e-9; }));

    if (result.userRank <= 10)
    {
        result.displayMode = "place";
        result.displayValue = result.userRank;
    }
    else if (result.userRank <= 100)
    {
        result.displayMode = "top";
        result.displayValue = std::min(100, static_cast<int>(std::ceil(result.userRank / 10.0) * 10.0));
    }
    else
    {
        const auto sameValueShare = roundPercent(result.sameValueUsersCount, result.regionUsersCount);
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

    result.displayText = buildRegionPointsDisplayText(result.displayMode, result.displayValue);
    return result;
}

std::optional<ClassTextDepthRankResult> SocialRankingsService::computeClassTextDepthRankQuarter(
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

    const auto className = loadUserClass(dbClient, userId, courseId, error);
    if (!className.has_value())
    {
        return std::nullopt;
    }

    const auto metrics = loadClassTextMetrics(dbClient, courseId, *className, *lessons, error);
    if (!metrics.has_value())
    {
        return std::nullopt;
    }

    ClassTextDepthRankResult result;
    result.className = *className;
    for (const auto &lesson : *lessons)
    {
        if (lesson.lessonNumber.has_value())
        {
            ++result.totalMaterialsCount;
        }
        if (lesson.hasPreparationMaterial)
        {
            ++result.totalMaterialsCount;
        }
    }

    auto ordered = *metrics;
    std::sort(ordered.begin(),
              ordered.end(),
              [](const UserIntMetric &lhs, const UserIntMetric &rhs)
              {
                  if (lhs.value != rhs.value)
                  {
                      return lhs.value > rhs.value;
                  }
                  return lhs.userId < rhs.userId;
              });

    result.classUsersCount = static_cast<int>(ordered.size());
    int currentRank = 0;
    int previousValue = -1;
    for (size_t index = 0; index < ordered.size(); ++index)
    {
        if (ordered[index].value != previousValue)
        {
            currentRank = static_cast<int>(index) + 1;
            previousValue = ordered[index].value;
        }

        if (ordered[index].userId == userId)
        {
            result.userRank = currentRank;
            result.learnedCount = ordered[index].value;
            break;
        }
    }

    if (result.userRank == 0)
    {
        error = {drogon::k404NotFound, "Пользователь не найден среди учеников своего класса"};
        return std::nullopt;
    }

    result.sameValueUsersCount = static_cast<int>(std::count_if(
        ordered.begin(),
        ordered.end(),
        [&](const UserIntMetric &item) { return item.value == result.learnedCount; }));

    if (result.userRank <= 10)
    {
        result.displayMode = "place";
        result.displayValue = result.userRank;
    }
    else if (result.userRank <= 100)
    {
        result.displayMode = "top";
        result.displayValue = std::min(100, static_cast<int>(std::ceil(result.userRank / 10.0) * 10.0));
    }
    else
    {
        const auto sameValueShare = roundPercent(result.sameValueUsersCount, result.classUsersCount);
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

    result.displayText = buildClassTextDisplayText(result.displayMode, result.displayValue);
    return result;
}

std::optional<ClassTextDepthRankResult> SocialRankingsService::computeClassTextDepthRankYear(
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

    const auto className = loadUserClass(dbClient, userId, courseId, error);
    if (!className.has_value())
    {
        return std::nullopt;
    }

    const auto metrics = loadClassTextMetrics(dbClient, courseId, *className, *lessons, error);
    if (!metrics.has_value())
    {
        return std::nullopt;
    }

    ClassTextDepthRankResult result;
    result.className = *className;
    for (const auto &lesson : *lessons)
    {
        if (lesson.lessonNumber.has_value())
        {
            ++result.totalMaterialsCount;
        }
        if (lesson.hasPreparationMaterial)
        {
            ++result.totalMaterialsCount;
        }
    }

    auto ordered = *metrics;
    std::sort(ordered.begin(),
              ordered.end(),
              [](const UserIntMetric &lhs, const UserIntMetric &rhs)
              {
                  if (lhs.value != rhs.value)
                  {
                      return lhs.value > rhs.value;
                  }
                  return lhs.userId < rhs.userId;
              });

    result.classUsersCount = static_cast<int>(ordered.size());
    int currentRank = 0;
    int previousValue = -1;
    for (size_t index = 0; index < ordered.size(); ++index)
    {
        if (ordered[index].value != previousValue)
        {
            currentRank = static_cast<int>(index) + 1;
            previousValue = ordered[index].value;
        }

        if (ordered[index].userId == userId)
        {
            result.userRank = currentRank;
            result.learnedCount = ordered[index].value;
            break;
        }
    }

    if (result.userRank == 0)
    {
        error = {drogon::k404NotFound, "Пользователь не найден среди учеников своего класса"};
        return std::nullopt;
    }

    result.sameValueUsersCount = static_cast<int>(std::count_if(
        ordered.begin(),
        ordered.end(),
        [&](const UserIntMetric &item) { return item.value == result.learnedCount; }));

    if (result.userRank <= 10)
    {
        result.displayMode = "place";
        result.displayValue = result.userRank;
    }
    else if (result.userRank <= 100)
    {
        result.displayMode = "top";
        result.displayValue = std::min(100, static_cast<int>(std::ceil(result.userRank / 10.0) * 10.0));
    }
    else
    {
        const auto sameValueShare = roundPercent(result.sameValueUsersCount, result.classUsersCount);
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

    result.displayText = buildClassTextDisplayText(result.displayMode, result.displayValue);
    return result;
}
}  // namespace yearreporter::services
