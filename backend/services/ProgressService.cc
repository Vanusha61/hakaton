#include "ProgressService.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

#include "constants/progress_constants.h"

namespace
{
int roundPercent(double value)
{
    const auto clamped = std::clamp(value, 0.0, 100.0);
    return static_cast<int>(std::lround(clamped));
}

double safeRatio(double numerator, double denominator)
{
    if (denominator <= 0.0)
    {
        return 0.0;
    }
    return std::clamp(numerator / denominator, 0.0, 1.0);
}
}  // namespace

namespace yearreporter::services
{
using namespace yearreporter::constants::progress;

std::optional<QuarterMetrics> ProgressService::computeQuarterProgress(
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

    CourseMetricsService metricsService;

    const auto course = metricsService.findUserCourse(dbClient, userId, courseId, error);
    if (!course.has_value())
    {
        return std::nullopt;
    }

    const auto lessonTrackId = metricsService.findMatchingLessonTrack(dbClient, *course, error);
    if (!lessonTrackId.has_value())
    {
        return std::nullopt;
    }

    const auto lessons = metricsService.loadCourseLessons(dbClient, courseId, error);
    if (!lessons.has_value())
    {
        return std::nullopt;
    }
    if (lessons->empty())
    {
        error = {drogon::k404NotFound,
                 "Для пользователя не найдены уроки по указанному курсу"};
        return std::nullopt;
    }

    const auto userStates = metricsService.loadUserLessonStates(dbClient, userId, *lessonTrackId, error);
    if (!userStates.has_value())
    {
        return std::nullopt;
    }

    return calculateQuarterMetrics(*course, *lessons, *userStates, quarter);
}

std::optional<int> ProgressService::computeCourseProgress(const drogon::orm::DbClientPtr &dbClient,
                                                          int userId,
                                                          int courseId,
                                                          ApiError &error) const
{
    double totalFraction = 0.0;
    for (int quarter = 1; quarter <= kQuarterCount; ++quarter)
    {
        auto quarterMetrics = computeQuarterProgress(dbClient, quarter, userId, courseId, error);
        if (!quarterMetrics.has_value())
        {
            return std::nullopt;
        }
        totalFraction += quarterMetrics->fraction;
    }

    return roundPercent(totalFraction * 25.0);
}

QuarterMetrics ProgressService::calculateQuarterMetrics(
    const CourseMetricsService::UserCourse &course,
    const std::vector<CourseMetricsService::CourseLesson> &lessons,
    const std::vector<CourseMetricsService::UserLessonState> &userStates,
                                                        int quarter)
{
    const auto slice = CourseMetricsService::quarterSliceFor(lessons, quarter);
    const auto quarterLessons = static_cast<double>(slice.end - slice.begin);
    const auto totalLessons = static_cast<double>(lessons.size());

    std::unordered_map<int, CourseMetricsService::UserLessonState> stateByLessonId;
    for (const auto &state : userStates)
    {
        stateByLessonId[state.lessonId] = state;
    }

    double quarterPoints = 0.0;
    double quarterSolvedTasks = 0.0;
    double quarterViewedVideos = 0.0;
    double quarterMaxTasks = 0.0;
    double quarterMaxVideos = 0.0;

    for (size_t index = slice.begin; index < slice.end; ++index)
    {
        const auto &lesson = lessons[index];
        quarterMaxTasks += static_cast<double>(lesson.maxTaskCount);
        quarterMaxVideos += lesson.hasVideo ? 1.0 : 0.0;
        const auto state = stateByLessonId.find(lesson.lessonId);
        if (state != stateByLessonId.end())
        {
            quarterPoints += state->second.earnedPoints;
            quarterSolvedTasks += static_cast<double>(state->second.solvedTasks);
            quarterViewedVideos += state->second.videoWatched ? 1.0 : 0.0;
        }
    }

    const auto quarterMaxPoints =
        totalLessons > 0.0 ? course.maxPoints * (quarterLessons / totalLessons) : 0.0;

    const auto pointsProgress = safeRatio(quarterPoints, quarterMaxPoints);
    const auto tasksProgress = safeRatio(quarterSolvedTasks, quarterMaxTasks);
    const auto videosProgress = safeRatio(quarterViewedVideos, quarterMaxVideos);

    const auto fraction = std::clamp(kQuarterWeightPoints * pointsProgress +
                                         kQuarterWeightTasks * tasksProgress +
                                         kQuarterWeightVideos * videosProgress,
                                     0.0,
                                     1.0);

    return QuarterMetrics{quarter, fraction, roundPercent(fraction * 100.0)};
}
}  // namespace yearreporter::services
