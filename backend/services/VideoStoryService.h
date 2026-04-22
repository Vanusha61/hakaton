#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <drogon/orm/DbClient.h>

#include "CourseMetricsService.h"

namespace yearreporter::services
{
struct FirstVideoStarterResult
{
    bool isUserFirstVideoStarter{false};
    std::string userFirstStartedAt;
    std::string globalFirstStartedAt;
};

struct VideoAttentionProfileResult
{
    int peakPositionPercentage{0};
    std::string role;
    std::string reportName;
    std::string insight;
};

class VideoStoryService
{
  public:
    struct AttentionDefinition
    {
        std::string role;
        std::string reportName;
        std::string insight;
    };

    std::optional<FirstVideoStarterResult> computeFirstVideoStarterQuarter(
        const drogon::orm::DbClientPtr &dbClient,
        int quarter,
        int userId,
        int courseId,
        ApiError &error) const;

    std::optional<FirstVideoStarterResult> computeFirstVideoStarterYear(
        const drogon::orm::DbClientPtr &dbClient,
        int userId,
        int courseId,
        ApiError &error) const;

    std::optional<VideoAttentionProfileResult> computeAttentionProfileQuarter(
        const drogon::orm::DbClientPtr &dbClient,
        int quarter,
        int userId,
        int courseId,
        ApiError &error) const;

    std::optional<VideoAttentionProfileResult> computeAttentionProfileYear(
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
        std::vector<CourseMetricsService::CourseLesson> lessons;
    };

    struct MediaSession
    {
        int viewerId{0};
        int lessonId{0};
        std::string startedAt;
        int segmentsTotal{0};
        std::string viewedSegments;
    };

    std::optional<Context> loadContext(const drogon::orm::DbClientPtr &dbClient,
                                       int userId,
                                       int courseId,
                                       ApiError &error) const;

    std::optional<std::vector<MediaSession>> loadSessions(const drogon::orm::DbClientPtr &dbClient,
                                                          int courseId,
                                                          int usersCourseId,
                                                          ApiError &error) const;

    static std::vector<CourseMetricsService::CourseLesson> sliceLessons(
        const std::vector<CourseMetricsService::CourseLesson> &lessons,
        int quarter,
        ApiError &error);

    static std::vector<MediaSession> filterSessionsByLessons(
        const std::vector<MediaSession> &sessions,
        const std::vector<CourseMetricsService::CourseLesson> &lessons);

    static std::vector<int> parseViewedSegments(const std::string &viewedSegments);
    static std::string normalizeStartedAt(const std::string &startedAt);
    static AttentionDefinition classifyAttentionProfile(
        const std::unordered_map<int, int> &hitsByPosition,
        int peakPositionPercentage);
    static bool hasMultiplePeaks(const std::unordered_map<int, int> &hitsByPosition);
    static VideoAttentionProfileResult buildAttentionProfile(
        const std::vector<MediaSession> &sessions);
    static std::optional<FirstVideoStarterResult> buildFirstStarterResult(
        const std::vector<MediaSession> &sessions,
        int userId,
        ApiError &error);
};
}  // namespace yearreporter::services
