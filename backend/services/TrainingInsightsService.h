#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <drogon/orm/DbClient.h>

#include "CourseMetricsService.h"

namespace yearreporter::services
{
struct TrainingDurationResult
{
    int trainingId{0};
    std::string trainingName;
    int durationSeconds{0};
    std::string durationHuman;
};

struct TrainingSolvedTasksResult
{
    int solvedTasksCount{0};
    int trainingsCount{0};
};

struct TrainingPerfectStreakResult
{
    int trainingId{0};
    std::string trainingName;
    int maxSolvedWithoutMistakes{0};
    std::string reportName;
    std::string insight;
};

class TrainingInsightsService
{
  public:
    std::optional<TrainingDurationResult> computeShortestQuarter(
        const drogon::orm::DbClientPtr &dbClient,
        int quarter,
        int userId,
        int courseId,
        ApiError &error) const;

    std::optional<TrainingDurationResult> computeShortestYear(
        const drogon::orm::DbClientPtr &dbClient,
        int userId,
        int courseId,
        ApiError &error) const;

    std::optional<TrainingDurationResult> computeLongestQuarter(
        const drogon::orm::DbClientPtr &dbClient,
        int quarter,
        int userId,
        int courseId,
        ApiError &error) const;

    std::optional<TrainingDurationResult> computeLongestYear(
        const drogon::orm::DbClientPtr &dbClient,
        int userId,
        int courseId,
        ApiError &error) const;

    std::optional<TrainingSolvedTasksResult> computeSolvedTasksQuarter(
        const drogon::orm::DbClientPtr &dbClient,
        int quarter,
        int userId,
        int courseId,
        ApiError &error) const;

    std::optional<TrainingSolvedTasksResult> computeSolvedTasksYear(
        const drogon::orm::DbClientPtr &dbClient,
        int userId,
        int courseId,
        ApiError &error) const;

    std::optional<TrainingPerfectStreakResult> computePerfectStreakQuarter(
        const drogon::orm::DbClientPtr &dbClient,
        int quarter,
        int userId,
        int courseId,
        ApiError &error) const;

    std::optional<TrainingPerfectStreakResult> computePerfectStreakYear(
        const drogon::orm::DbClientPtr &dbClient,
        int userId,
        int courseId,
        ApiError &error) const;

  private:
    struct TrainingDefinition
    {
        int trainingId{0};
        std::string name;
        std::optional<int> lessonId;
        int taskTemplatesCount{0};
    };

    struct UserTrainingRecord
    {
        int trainingId{0};
        int solvedTasksCount{0};
        int submittedAnswersCount{0};
        int attempts{0};
        std::string startedAt;
        std::string finishedAt;
    };

    struct ScopedContext
    {
        std::vector<TrainingDefinition> trainings;
        std::vector<UserTrainingRecord> userTrainings;
    };

    std::optional<ScopedContext> loadQuarterContext(const drogon::orm::DbClientPtr &dbClient,
                                                    int quarter,
                                                    int userId,
                                                    int courseId,
                                                    ApiError &error) const;

    std::optional<ScopedContext> loadYearContext(const drogon::orm::DbClientPtr &dbClient,
                                                 int userId,
                                                 int courseId,
                                                 ApiError &error) const;

    std::optional<ScopedContext> loadContextForLessons(const drogon::orm::DbClientPtr &dbClient,
                                                       const std::vector<CourseMetricsService::CourseLesson> &lessons,
                                                       int userId,
                                                       int courseId,
                                                       ApiError &error) const;

    static std::optional<TrainingDurationResult> shortestFromContext(const ScopedContext &context,
                                                                     ApiError &error);
    static std::optional<TrainingDurationResult> longestFromContext(const ScopedContext &context,
                                                                    ApiError &error);
    static std::optional<TrainingPerfectStreakResult> perfectStreakFromContext(
        const ScopedContext &context,
        ApiError &error);

    static const std::vector<TrainingDefinition> &definitions();
    static std::optional<int> parseDurationSeconds(const std::string &startedAt,
                                                   const std::string &finishedAt);
    static std::string formatDuration(int totalSeconds);
};
}  // namespace yearreporter::services
