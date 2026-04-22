#pragma once

#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include <drogon/orm/DbClient.h>

#include "CourseMetricsService.h"

namespace yearreporter::services
{
struct StudyRecommendationFactor
{
    std::string key;
    std::string title;
    int levelPercentage{0};
    std::string influenceLabel;
    double influenceScore{0.0};
};

struct StudyRecommendationResult
{
    StudyRecommendationFactor video;
    StudyRecommendationFactor conspects;
    StudyRecommendationFactor materials;
    double currentAverageScore{0.0};
    double targetAverageScore{0.0};
    std::string recommendedFocusKey;
    std::string recommendedFocusTitle;
    std::string recommendation;
    std::string aiInsight;
};

class StudyRecommendationsService
{
  public:
    std::optional<StudyRecommendationResult> computeQuarter(
        const drogon::orm::DbClientPtr &dbClient,
        int quarter,
        int userId,
        int courseId,
        ApiError &error) const;

    std::optional<StudyRecommendationResult> computeYear(
        const drogon::orm::DbClientPtr &dbClient,
        int userId,
        int courseId,
        ApiError &error) const;

  private:
    struct PeriodContext
    {
        std::vector<CourseMetricsService::CourseLesson> lessons;
        std::vector<CourseMetricsService::UserLessonState> userStates;
        std::unordered_set<int> visitedPreparationMaterials;
        double totalMaxPoints{0.0};
        int totalVideoLessons{0};
        int totalConspects{0};
        int totalMaterialLessons{0};
    };

    struct UserProfile
    {
        int userId{0};
        int watchedVideos{0};
        int openedConspects{0};
        int openedMaterials{0};
        double earnedPoints{0.0};
    };

    std::optional<PeriodContext> loadQuarterContext(const drogon::orm::DbClientPtr &dbClient,
                                                    int quarter,
                                                    int userId,
                                                    int courseId,
                                                    ApiError &error) const;

    std::optional<PeriodContext> loadYearContext(const drogon::orm::DbClientPtr &dbClient,
                                                 int userId,
                                                 int courseId,
                                                 ApiError &error) const;

    std::optional<PeriodContext> loadPeriodContext(const drogon::orm::DbClientPtr &dbClient,
                                                   const std::vector<CourseMetricsService::CourseLesson> &selectedLessons,
                                                   int userId,
                                                   int courseId,
                                                   ApiError &error) const;

    std::optional<std::vector<UserProfile>> loadCourseProfiles(
        const drogon::orm::DbClientPtr &dbClient,
        int courseId,
        const PeriodContext &context,
        ApiError &error) const;

    std::optional<StudyRecommendationResult> buildResult(
        const PeriodContext &context,
        const std::vector<UserProfile> &profiles,
        int userId,
        const std::string &periodPhrase,
        ApiError &error) const;

    static std::string buildIdList(const std::vector<CourseMetricsService::CourseLesson> &lessons);
    static int roundPercent(int valueCount, int totalCount);
    static double normalizeToFiveScale(double earnedPoints, double totalMaxPoints);
    static double roundToOneDecimal(double value);
    static double pearsonCorrelation(const std::vector<double> &xs, const std::vector<double> &ys);
    static std::string influenceLabel(double influenceScore);
    static std::string powerPhrase(double averageLevel);
    static double factorFraction(const UserProfile &profile,
                                 const PeriodContext &context,
                                 const std::string &key);
};
}  // namespace yearreporter::services
