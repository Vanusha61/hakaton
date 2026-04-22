#pragma once

#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include <drogon/orm/DbClient.h>

#include "CourseMetricsService.h"

namespace yearreporter::services
{
struct RegionPointsRankResult
{
    std::string regionName;
    double earnedPoints{0.0};
    double totalPoints{0.0};
    int userRank{0};
    int regionUsersCount{0};
    int sameValueUsersCount{0};
    std::string displayMode;
    int displayValue{0};
    std::string displayText;
    std::string earnedPointsDisplay;
    std::string totalPointsDisplay;
};

struct ClassTextDepthRankResult
{
    std::string className;
    int learnedCount{0};
    int totalMaterialsCount{0};
    int userRank{0};
    int classUsersCount{0};
    int sameValueUsersCount{0};
    std::string displayMode;
    int displayValue{0};
    std::string displayText;
};

class SocialRankingsService
{
  public:
    std::optional<RegionPointsRankResult> computeRegionPointsRankQuarter(
        const drogon::orm::DbClientPtr &dbClient,
        int quarter,
        int userId,
        int courseId,
        ApiError &error) const;

    std::optional<RegionPointsRankResult> computeRegionPointsRankYear(
        const drogon::orm::DbClientPtr &dbClient,
        int userId,
        int courseId,
        ApiError &error) const;

    std::optional<ClassTextDepthRankResult> computeClassTextDepthRankQuarter(
        const drogon::orm::DbClientPtr &dbClient,
        int quarter,
        int userId,
        int courseId,
        ApiError &error) const;

    std::optional<ClassTextDepthRankResult> computeClassTextDepthRankYear(
        const drogon::orm::DbClientPtr &dbClient,
        int userId,
        int courseId,
        ApiError &error) const;

  private:
    struct UserDoubleMetric
    {
        int userId{0};
        double value{0.0};
    };

    struct UserIntMetric
    {
        int userId{0};
        int value{0};
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

    std::optional<std::string> loadUserRegion(const drogon::orm::DbClientPtr &dbClient,
                                              int userId,
                                              int courseId,
                                              ApiError &error) const;

    std::optional<std::string> loadUserClass(const drogon::orm::DbClientPtr &dbClient,
                                             int userId,
                                             int courseId,
                                             ApiError &error) const;

    std::optional<std::vector<UserDoubleMetric>> loadRegionPointsMetrics(
        const drogon::orm::DbClientPtr &dbClient,
        int courseId,
        const std::string &regionName,
        const std::vector<CourseMetricsService::CourseLesson> &lessons,
        ApiError &error) const;

    std::optional<std::vector<UserIntMetric>> loadClassTextMetrics(
        const drogon::orm::DbClientPtr &dbClient,
        int courseId,
        const std::string &className,
        const std::vector<CourseMetricsService::CourseLesson> &lessons,
        ApiError &error) const;

    static std::string buildLessonIdList(const std::vector<CourseMetricsService::CourseLesson> &lessons);
    static int roundPercent(int value, int total);
    static double roundToTwoDigits(double value);
    static std::string formatFixed2(double value);
};
}  // namespace yearreporter::services
