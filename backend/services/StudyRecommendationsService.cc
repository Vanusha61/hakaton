#include "StudyRecommendationsService.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <unordered_map>

#include "constants/progress_constants.h"

namespace yearreporter::services
{
namespace
{
using Lesson = CourseMetricsService::CourseLesson;
using QuarterSlice = CourseMetricsService::QuarterSlice;

double safeAverage(const std::vector<double> &values)
{
    if (values.empty())
    {
        return 0.0;
    }

    double total = 0.0;
    for (double value : values)
    {
        total += value;
    }
    return total / static_cast<double>(values.size());
}

std::string formatOneDecimal(double value)
{
    std::ostringstream out;
    out.setf(std::ios::fixed);
    out.precision(1);
    out << value;
    return out.str();
}
}  // namespace

std::optional<StudyRecommendationsService::PeriodContext>
StudyRecommendationsService::loadQuarterContext(const drogon::orm::DbClientPtr &dbClient,
                                                int quarter,
                                                int userId,
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
    std::vector<Lesson> selectedLessons(
        lessons->begin() + static_cast<std::ptrdiff_t>(slice.begin),
        lessons->begin() + static_cast<std::ptrdiff_t>(slice.end));

    return loadPeriodContext(dbClient, selectedLessons, userId, courseId, error);
}

std::optional<StudyRecommendationsService::PeriodContext>
StudyRecommendationsService::loadYearContext(const drogon::orm::DbClientPtr &dbClient,
                                             int userId,
                                             int courseId,
                                             ApiError &error) const
{
    CourseMetricsService courseService;
    const auto lessons = courseService.loadCourseLessons(dbClient, courseId, error);
    if (!lessons.has_value())
    {
        return std::nullopt;
    }

    return loadPeriodContext(dbClient, *lessons, userId, courseId, error);
}

std::optional<StudyRecommendationsService::PeriodContext>
StudyRecommendationsService::loadPeriodContext(const drogon::orm::DbClientPtr &dbClient,
                                               const std::vector<Lesson> &selectedLessons,
                                               int userId,
                                               int courseId,
                                               ApiError &error) const
{
    if (selectedLessons.empty())
    {
        error = {drogon::k404NotFound, "Для выбранного периода не найдены уроки"};
        return std::nullopt;
    }

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

    const auto userStates = courseService.loadUserLessonStates(dbClient, userId, *usersCourseId, error);
    if (!userStates.has_value())
    {
        return std::nullopt;
    }

    const auto visitedPreparationMaterials =
        courseService.loadVisitedPreparationMaterials(dbClient, *usersCourseId, error);
    if (!visitedPreparationMaterials.has_value())
    {
        return std::nullopt;
    }

    PeriodContext context;
    context.lessons = selectedLessons;
    context.userStates = *userStates;
    context.visitedPreparationMaterials = *visitedPreparationMaterials;

    for (const auto &lesson : context.lessons)
    {
        context.totalMaxPoints += lesson.maxPoints;
        if (lesson.hasVideo)
        {
            ++context.totalVideoLessons;
        }
        if (lesson.lessonNumber.has_value())
        {
            ++context.totalConspects;
        }
        if (lesson.hasPreparationMaterial)
        {
            ++context.totalMaterialLessons;
        }
    }

    return context;
}

std::string StudyRecommendationsService::buildIdList(const std::vector<Lesson> &lessons)
{
    std::ostringstream out;
    for (size_t index = 0; index < lessons.size(); ++index)
    {
        if (index > 0)
        {
            out << ", ";
        }
        out << lessons[index].lessonId;
    }
    return out.str();
}

int StudyRecommendationsService::roundPercent(int valueCount, int totalCount)
{
    if (totalCount <= 0)
    {
        return 0;
    }

    return static_cast<int>(std::lround(static_cast<double>(valueCount) * 100.0 /
                                        static_cast<double>(totalCount)));
}

double StudyRecommendationsService::normalizeToFiveScale(double earnedPoints, double totalMaxPoints)
{
    if (totalMaxPoints <= 0.0)
    {
        return 0.0;
    }

    return std::clamp((earnedPoints / totalMaxPoints) * 5.0, 0.0, 5.0);
}

double StudyRecommendationsService::roundToOneDecimal(double value)
{
    return std::round(value * 10.0) / 10.0;
}

double StudyRecommendationsService::pearsonCorrelation(const std::vector<double> &xs,
                                                       const std::vector<double> &ys)
{
    if (xs.size() != ys.size() || xs.size() < 2)
    {
        return 0.0;
    }

    const auto meanX = safeAverage(xs);
    const auto meanY = safeAverage(ys);

    double numerator = 0.0;
    double sumSqX = 0.0;
    double sumSqY = 0.0;

    for (size_t index = 0; index < xs.size(); ++index)
    {
        const auto dx = xs[index] - meanX;
        const auto dy = ys[index] - meanY;
        numerator += dx * dy;
        sumSqX += dx * dx;
        sumSqY += dy * dy;
    }

    if (sumSqX <= 0.0 || sumSqY <= 0.0)
    {
        return 0.0;
    }

    return std::fabs(numerator / std::sqrt(sumSqX * sumSqY));
}

std::string StudyRecommendationsService::influenceLabel(double influenceScore)
{
    if (influenceScore >= 0.45)
    {
        return "Высокое 🚀";
    }
    if (influenceScore >= 0.2)
    {
        return "Среднее 📈";
    }
    return "Поддерживающее 🛠";
}

std::string StudyRecommendationsService::powerPhrase(double averageLevel)
{
    if (averageLevel < 0.34)
    {
        return "на 1/3 мощности";
    }
    if (averageLevel < 0.67)
    {
        return "на 2/3 мощности";
    }
    return "почти на полной мощности";
}

double StudyRecommendationsService::factorFraction(const UserProfile &profile,
                                                   const PeriodContext &context,
                                                   const std::string &key)
{
    if (key == "video")
    {
        return context.totalVideoLessons > 0
                   ? static_cast<double>(profile.watchedVideos) /
                         static_cast<double>(context.totalVideoLessons)
                   : 0.0;
    }
    if (key == "conspects")
    {
        return context.totalConspects > 0
                   ? static_cast<double>(profile.openedConspects) /
                         static_cast<double>(context.totalConspects)
                   : 0.0;
    }
    if (key == "materials")
    {
        return context.totalMaterialLessons > 0
                   ? static_cast<double>(profile.openedMaterials) /
                         static_cast<double>(context.totalMaterialLessons)
                   : 0.0;
    }
    return 0.0;
}

std::optional<std::vector<StudyRecommendationsService::UserProfile>>
StudyRecommendationsService::loadCourseProfiles(const drogon::orm::DbClientPtr &dbClient,
                                                int courseId,
                                                const PeriodContext &context,
                                                ApiError &error) const
{
    try
    {
        const auto lessonIds = buildIdList(context.lessons);

        const auto usersResult = dbClient->execSqlSync(
            "select distinct user_id "
            "from public.user_courses "
            "where course_id = $1",
            courseId);

        if (usersResult.empty())
        {
            error = {drogon::k404NotFound, "Для курса не найдены пользователи"};
            return std::nullopt;
        }

        std::unordered_map<int, UserProfile> profileByUserId;
        for (const auto &row : usersResult)
        {
            const auto userId = row["user_id"].as<int>();
            profileByUserId.emplace(userId, UserProfile{.userId = userId});
        }

        const auto lessonStatsResult = dbClient->execSqlSync(
            "select ul.user_id, "
            "       ul.lesson_id, "
            "       max(coalesce(ul.wk_points, 0)) as earned_points, "
            "       bool_or(coalesce(ul.video_visited, false) or coalesce(ul.video_viewed, false)) as video_done, "
            "       bool_or(coalesce(ul.translation_visited, false)) as conspect_done "
            "from public.user_lessons ul "
            "join public.lessons l on l.id = ul.lesson_id and l.course_id = $1 "
            "where ul.lesson_id in (" +
                lessonIds +
                ") "
                "group by ul.user_id, ul.lesson_id",
            courseId);

        std::unordered_map<int, Lesson> lessonById;
        for (const auto &lesson : context.lessons)
        {
            lessonById.emplace(lesson.lessonId, lesson);
        }

        for (const auto &row : lessonStatsResult)
        {
            const auto userId = row["user_id"].as<int>();
            const auto lessonId = row["lesson_id"].as<int>();
            const auto lessonIt = lessonById.find(lessonId);
            if (lessonIt == lessonById.end())
            {
                continue;
            }

            auto &profile = profileByUserId[userId];
            profile.userId = userId;
            profile.earnedPoints += row["earned_points"].as<double>();
            if (lessonIt->second.hasVideo && row["video_done"].as<bool>())
            {
                ++profile.watchedVideos;
            }
            if (lessonIt->second.lessonNumber.has_value() && row["conspect_done"].as<bool>())
            {
                ++profile.openedConspects;
            }
        }

        const auto materialsResult = dbClient->execSqlSync(
            "select a.user_id, a.lesson_id "
            "from public.wk_users_courses_actions a "
            "join public.lessons l on l.id = a.lesson_id and l.course_id = $1 "
            "where a.action = 'visit_preparation_material' "
            "  and a.lesson_id in (" +
                lessonIds +
                ") "
                "group by a.user_id, a.lesson_id",
            courseId);

        for (const auto &row : materialsResult)
        {
            const auto userId = row["user_id"].as<int>();
            const auto lessonId = row["lesson_id"].as<int>();
            const auto lessonIt = lessonById.find(lessonId);
            if (lessonIt == lessonById.end() || !lessonIt->second.hasPreparationMaterial)
            {
                continue;
            }
            auto &profile = profileByUserId[userId];
            profile.userId = userId;
            ++profile.openedMaterials;
        }

        std::vector<UserProfile> profiles;
        profiles.reserve(profileByUserId.size());
        for (const auto &[_, profile] : profileByUserId)
        {
            profiles.push_back(profile);
        }

        return profiles;
    }
    catch (const std::exception &e)
    {
        error = {drogon::k500InternalServerError,
                 "Ошибка расчёта рекомендаций по обучению: " + std::string(e.what())};
        return std::nullopt;
    }
}

std::optional<StudyRecommendationResult> StudyRecommendationsService::buildResult(
    const PeriodContext &context,
    const std::vector<UserProfile> &profiles,
    int userId,
    const std::string &periodPhrase,
    ApiError &error) const
{
    const auto userIt = std::find_if(
        profiles.begin(),
        profiles.end(),
        [userId](const UserProfile &profile) { return profile.userId == userId; });

    if (userIt == profiles.end())
    {
        error = {drogon::k404NotFound, "Для пользователя не найдены данные по выбранному периоду"};
        return std::nullopt;
    }

    const auto currentScore =
        roundToOneDecimal(normalizeToFiveScale(userIt->earnedPoints, context.totalMaxPoints));

    std::vector<double> scores;
    std::vector<double> videoLevels;
    std::vector<double> conspectLevels;
    std::vector<double> materialLevels;
    scores.reserve(profiles.size());
    videoLevels.reserve(profiles.size());
    conspectLevels.reserve(profiles.size());
    materialLevels.reserve(profiles.size());

    for (const auto &profile : profiles)
    {
        scores.push_back(normalizeToFiveScale(profile.earnedPoints, context.totalMaxPoints));
        videoLevels.push_back(factorFraction(profile, context, "video"));
        conspectLevels.push_back(factorFraction(profile, context, "conspects"));
        materialLevels.push_back(factorFraction(profile, context, "materials"));
    }

    const auto videoInfluence = pearsonCorrelation(videoLevels, scores);
    const auto conspectInfluence = pearsonCorrelation(conspectLevels, scores);
    const auto materialInfluence = pearsonCorrelation(materialLevels, scores);

    StudyRecommendationResult result;
    result.video = StudyRecommendationFactor{
        .key = "video",
        .title = "Видео",
        .levelPercentage = roundPercent(userIt->watchedVideos, context.totalVideoLessons),
        .influenceLabel = influenceLabel(videoInfluence),
        .influenceScore = videoInfluence};
    result.conspects = StudyRecommendationFactor{
        .key = "conspects",
        .title = "Конспекты",
        .levelPercentage = roundPercent(userIt->openedConspects, context.totalConspects),
        .influenceLabel = influenceLabel(conspectInfluence),
        .influenceScore = conspectInfluence};
    result.materials = StudyRecommendationFactor{
        .key = "materials",
        .title = "Доп. материалы",
        .levelPercentage = roundPercent(userIt->openedMaterials, context.totalMaterialLessons),
        .influenceLabel = influenceLabel(materialInfluence),
        .influenceScore = materialInfluence};

    std::vector<StudyRecommendationFactor> factors = {
        result.video,
        result.conspects,
        result.materials};

    std::sort(factors.begin(),
              factors.end(),
              [&](const StudyRecommendationFactor &lhs, const StudyRecommendationFactor &rhs)
              {
                  const auto lhsFraction = lhs.levelPercentage / 100.0;
                  const auto rhsFraction = rhs.levelPercentage / 100.0;
                  const auto lhsPriority = lhs.influenceScore * std::max(0.0, 0.75 - lhsFraction);
                  const auto rhsPriority = rhs.influenceScore * std::max(0.0, 0.75 - rhsFraction);
                  if (lhsPriority != rhsPriority)
                  {
                      return lhsPriority > rhsPriority;
                  }
                  if (lhs.levelPercentage != rhs.levelPercentage)
                  {
                      return lhs.levelPercentage < rhs.levelPercentage;
                  }
                  return lhs.influenceScore > rhs.influenceScore;
              });

    const auto focus = factors.front();
    result.recommendedFocusKey = focus.key;
    result.recommendedFocusTitle = focus.title;
    result.currentAverageScore = currentScore;

    std::vector<std::pair<double, double>> focusCandidates;
    focusCandidates.reserve(profiles.size());
    for (size_t index = 0; index < profiles.size(); ++index)
    {
        double level = 0.0;
        if (focus.key == "video")
        {
            level = videoLevels[index];
        }
        else if (focus.key == "conspects")
        {
            level = conspectLevels[index];
        }
        else
        {
            level = materialLevels[index];
        }
        focusCandidates.push_back({level, scores[index]});
    }

    std::vector<double> benchmarkScores;
    for (const auto &[level, score] : focusCandidates)
    {
        if (level >= 0.75)
        {
            benchmarkScores.push_back(score);
        }
    }

    if (benchmarkScores.size() < 3)
    {
        std::sort(focusCandidates.begin(),
                  focusCandidates.end(),
                  [](const auto &lhs, const auto &rhs)
                  {
                      if (lhs.first != rhs.first)
                      {
                          return lhs.first > rhs.first;
                      }
                      return lhs.second > rhs.second;
                  });

        benchmarkScores.clear();
        const auto sampleCount = std::min<size_t>(std::max<size_t>(3, focusCandidates.size() / 3),
                                                  focusCandidates.size());
        for (size_t index = 0; index < sampleCount; ++index)
        {
            benchmarkScores.push_back(focusCandidates[index].second);
        }
    }

    result.targetAverageScore = roundToOneDecimal(safeAverage(benchmarkScores));

    const auto averageLevel =
        (result.video.levelPercentage + result.conspects.levelPercentage + result.materials.levelPercentage) /
        300.0;

    result.recommendation = "Сделай упор на блок \"" + focus.title +
                            "\": именно он сейчас даёт лучший шанс поднять средний балл.";

    result.aiInsight = "Твоя ракета летит " + powerPhrase(averageLevel) +
                       ". Сейчас твой средний балл — " + formatOneDecimal(result.currentAverageScore) +
                       ". Анализ данных показывает: ученики, которые уделяют \"" + focus.title +
                       "\" хотя бы 75% внимания " + periodPhrase + ", в среднем получают " +
                       formatOneDecimal(result.targetAverageScore) +
                       " балла. Добавь больше активности в этот блок, и твой результат может заметно вырасти.";

    return result;
}

std::optional<StudyRecommendationResult> StudyRecommendationsService::computeQuarter(
    const drogon::orm::DbClientPtr &dbClient,
    int quarter,
    int userId,
    int courseId,
    ApiError &error) const
{
    const auto context = loadQuarterContext(dbClient, quarter, userId, courseId, error);
    if (!context.has_value())
    {
        return std::nullopt;
    }

    const auto profiles = loadCourseProfiles(dbClient, courseId, *context, error);
    if (!profiles.has_value())
    {
        return std::nullopt;
    }

    return buildResult(*context, *profiles, userId, "в этой четверти", error);
}

std::optional<StudyRecommendationResult> StudyRecommendationsService::computeYear(
    const drogon::orm::DbClientPtr &dbClient,
    int userId,
    int courseId,
    ApiError &error) const
{
    const auto context = loadYearContext(dbClient, userId, courseId, error);
    if (!context.has_value())
    {
        return std::nullopt;
    }

    const auto profiles = loadCourseProfiles(dbClient, courseId, *context, error);
    if (!profiles.has_value())
    {
        return std::nullopt;
    }

    return buildResult(*context, *profiles, userId, "в этом году", error);
}
}  // namespace yearreporter::services
