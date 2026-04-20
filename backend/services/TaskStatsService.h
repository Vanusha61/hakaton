#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <drogon/orm/DbClient.h>

#include "CourseMetricsService.h"

namespace yearreporter::services
{
struct SolvedTasksCountResult
{
    int solvedTasksCount{0};
};

struct AverageScoreResult
{
    int solvedTasksCount{0};
    double earnedPoints{0.0};
    double averageScore{0.0};
};

class TaskStatsService
{
  public:
    std::optional<SolvedTasksCountResult> computeHomeworkSolvedTasksQuarter(
        const drogon::orm::DbClientPtr &dbClient,
        int quarter,
        int userId,
        int courseId,
        ApiError &error) const;

    std::optional<SolvedTasksCountResult> computeHomeworkSolvedTasksYear(
        const drogon::orm::DbClientPtr &dbClient,
        int userId,
        int courseId,
        ApiError &error) const;

    std::optional<SolvedTasksCountResult> computeLessonSolvedTasksQuarter(
        const drogon::orm::DbClientPtr &dbClient,
        int quarter,
        int userId,
        int courseId,
        ApiError &error) const;

    std::optional<SolvedTasksCountResult> computeLessonSolvedTasksYear(
        const drogon::orm::DbClientPtr &dbClient,
        int userId,
        int courseId,
        ApiError &error) const;

    std::optional<AverageScoreResult> computeAverageScoreQuarter(
        const drogon::orm::DbClientPtr &dbClient,
        int quarter,
        int userId,
        int courseId,
        ApiError &error) const;

    std::optional<AverageScoreResult> computeAverageScoreYear(
        const drogon::orm::DbClientPtr &dbClient,
        int userId,
        int courseId,
        ApiError &error) const;

  private:
    struct Context
    {
        int userId{0};
        int courseId{0};
        int usersCourseId{0};
        std::string startedAt;
        std::string finishedAt;
        std::vector<CourseMetricsService::CourseLesson> lessons;
        std::unordered_map<int, CourseMetricsService::UserLessonState> stateByLessonId;
    };

    struct HomeworkAggregate
    {
        int solvedTasksCount{0};
        double earnedPoints{0.0};
    };

    std::optional<Context> loadContext(const drogon::orm::DbClientPtr &dbClient,
                                       int userId,
                                       int courseId,
                                       ApiError &error) const;

    static std::vector<CourseMetricsService::CourseLesson> sliceLessons(
        const std::vector<CourseMetricsService::CourseLesson> &lessons,
        int quarter,
        ApiError &error);

    std::optional<HomeworkAggregate> loadHomeworkYear(const drogon::orm::DbClientPtr &dbClient,
                                                      int userId,
                                                      ApiError &error) const;

    std::optional<HomeworkAggregate> loadHomeworkQuarter(const drogon::orm::DbClientPtr &dbClient,
                                                         int userId,
                                                         int quarter,
                                                         const std::string &startedAt,
                                                         const std::string &finishedAt,
                                                         ApiError &error) const;

    static AverageScoreResult buildAverageScoreResult(int solvedTasksCount, double earnedPoints);
    static double roundToTwoDigits(double value);
};
}  // namespace yearreporter::services
