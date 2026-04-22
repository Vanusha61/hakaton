#pragma once

#include <optional>
#include <string>
#include <vector>

#include <drogon/orm/DbClient.h>

#include "CourseMetricsService.h"

namespace yearreporter::services
{
struct SchoolVideoRankResult
{
    std::string schoolName;
    int watchedLessonsCount{0};
    int totalVideoLessonsCount{0};
    int userRank{0};
    int schoolUsersCount{0};
    int sameValueUsersCount{0};
    std::string displayMode;
    int displayValue{0};
    std::string displayText;
};

class SchoolRankingsService
{
  public:
    std::optional<SchoolVideoRankResult> computeVideoRankQuarter(
        const drogon::orm::DbClientPtr &dbClient,
        int quarter,
        int userId,
        int courseId,
        ApiError &error) const;

    std::optional<SchoolVideoRankResult> computeVideoRankYear(
        const drogon::orm::DbClientPtr &dbClient,
        int userId,
        int courseId,
        ApiError &error) const;

  private:
    struct SchoolUserVideoCount
    {
        int userId{0};
        int watchedLessonsCount{0};
    };

    std::optional<std::vector<CourseMetricsService::CourseLesson>> loadQuarterLessons(
        const drogon::orm::DbClientPtr &dbClient,
        int quarter,
        int courseId,
        ApiError &error) const;

    std::optional<std::vector<CourseMetricsService::CourseLesson>> loadYearLessons(
        const drogon::orm::DbClientPtr &dbClient,
        int courseId,
        ApiError &error) const;

    std::optional<std::string> loadUserSchool(const drogon::orm::DbClientPtr &dbClient,
                                              int userId,
                                              int courseId,
                                              ApiError &error) const;

    std::optional<std::vector<SchoolUserVideoCount>> loadSchoolVideoCounts(
        const drogon::orm::DbClientPtr &dbClient,
        int courseId,
        const std::string &schoolName,
        const std::vector<CourseMetricsService::CourseLesson> &lessons,
        ApiError &error) const;

    static SchoolVideoRankResult buildResult(const std::string &schoolName,
                                             const std::vector<CourseMetricsService::CourseLesson> &lessons,
                                             const std::vector<SchoolUserVideoCount> &counts,
                                             int userId,
                                             ApiError &error);

    static std::string buildLessonIdList(const std::vector<CourseMetricsService::CourseLesson> &lessons);
    static int roundPercent(int value, int total);
};
}  // namespace yearreporter::services
