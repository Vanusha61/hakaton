#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <drogon/orm/DbClient.h>

#include "CourseMetricsService.h"

namespace yearreporter::services
{
struct RareLesson
{
    std::string title;
    int notSolvedPercentage{0};
};

struct RareLessonsResult
{
    std::vector<RareLesson> items;
};

struct RarestVisitedLessonResult
{
    std::string title;
    int visitedPercentage{0};
    int visitedUsersCount{0};
    int totalUsersCount{0};
    std::string reportName;
    std::string insight;
};

class LessonAchievementsService
{
  public:
    std::optional<int> computeFullSolvedLessonsQuarter(const drogon::orm::DbClientPtr &dbClient,
                                                       int quarter,
                                                       int userId,
                                                       int courseId,
                                                       ApiError &error) const;

    std::optional<int> computeFullSolvedLessonsYear(const drogon::orm::DbClientPtr &dbClient,
                                                    int userId,
                                                    int courseId,
                                                    ApiError &error) const;

    std::optional<bool> computeVisitedAllLessonsQuarter(const drogon::orm::DbClientPtr &dbClient,
                                                        int quarter,
                                                        int userId,
                                                        int courseId,
                                                        ApiError &error) const;

    std::optional<bool> computeVisitedAllLessonsYear(const drogon::orm::DbClientPtr &dbClient,
                                                     int userId,
                                                     int courseId,
                                                     ApiError &error) const;

    std::optional<RareLessonsResult> computeRareLessonsQuarter(const drogon::orm::DbClientPtr &dbClient,
                                                               int quarter,
                                                               int userId,
                                                               int courseId,
                                                               ApiError &error) const;

    std::optional<RareLessonsResult> computeRareLessonsYear(const drogon::orm::DbClientPtr &dbClient,
                                                            int userId,
                                                            int courseId,
                                                            ApiError &error) const;

    std::optional<RarestVisitedLessonResult> computeRarestVisitedLessonQuarter(
        const drogon::orm::DbClientPtr &dbClient,
        int quarter,
        int userId,
        int courseId,
        ApiError &error) const;

    std::optional<RarestVisitedLessonResult> computeRarestVisitedLessonYear(
        const drogon::orm::DbClientPtr &dbClient,
        int userId,
        int courseId,
        ApiError &error) const;

  private:
    struct MetricsContext
    {
        std::vector<CourseMetricsService::CourseLesson> lessons;
        std::vector<CourseMetricsService::UserLessonState> userStates;
        std::unordered_map<int, CourseMetricsService::CourseLesson> lessonById;
        std::unordered_map<int, CourseMetricsService::UserLessonState> stateByLessonId;
    };

    std::optional<MetricsContext> loadContext(const drogon::orm::DbClientPtr &dbClient,
                                              int userId,
                                              int courseId,
                                              ApiError &error) const;

    static std::vector<CourseMetricsService::CourseLesson> sliceLessons(
        const std::vector<CourseMetricsService::CourseLesson> &lessons,
        int quarter,
        ApiError &error);

    static std::string makeLessonTitle(const CourseMetricsService::CourseLesson &lesson);

    std::optional<RareLessonsResult> computeRareLessons(
        const drogon::orm::DbClientPtr &dbClient,
        int courseId,
        const std::vector<CourseMetricsService::CourseLesson> &lessons,
        const std::unordered_map<int, CourseMetricsService::UserLessonState> &stateByLessonId,
        ApiError &error) const;

    std::optional<RarestVisitedLessonResult> computeRarestVisitedLesson(
        const drogon::orm::DbClientPtr &dbClient,
        int courseId,
        const std::vector<CourseMetricsService::CourseLesson> &lessons,
        const std::unordered_map<int, CourseMetricsService::UserLessonState> &stateByLessonId,
        const std::string &periodPhrase,
        ApiError &error) const;
};
}  // namespace yearreporter::services
