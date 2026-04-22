#pragma once

#include <optional>
#include <vector>

#include <drogon/orm/DbClient.h>

#include "CourseMetricsService.h"

namespace yearreporter::services
{
struct MediaTypeMetrics
{
    int count{0};
    int percentage{0};
};

struct MediaStats
{
    MediaTypeMetrics liveLesson;
    MediaTypeMetrics recordedLesson;
    MediaTypeMetrics prerecordedLesson;
    int totalWatchedVideosCount{0};
};

class MediaStatsService
{
  public:
    struct MediaResource
    {
        enum class Type
        {
            Live,
            Recorded,
            Prerecorded
        };

        int lessonId{0};
        Type type{Type::Prerecorded};
    };

    std::optional<MediaStats> computeQuarter(const drogon::orm::DbClientPtr &dbClient,
                                             int quarter,
                                             int userId,
                                             int courseId,
                                             ApiError &error) const;

    std::optional<MediaStats> computeYear(const drogon::orm::DbClientPtr &dbClient,
                                          int userId,
                                          int courseId,
                                          ApiError &error) const;

    std::optional<std::vector<MediaResource>> loadWatchedResources(
        const drogon::orm::DbClientPtr &dbClient,
        int userId,
        int usersCourseId,
        ApiError &error) const;

    static MediaStats buildStats(const std::vector<MediaResource> &resources);
    static std::vector<int> computePercentages(int total, int live, int recorded, int prerecorded);

  private:
};
}  // namespace yearreporter::services
