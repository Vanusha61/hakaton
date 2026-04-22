#pragma once

#include <optional>
#include <string>
#include <vector>

#include <drogon/orm/DbClient.h>

#include "CourseMetricsService.h"

namespace yearreporter::services
{
struct ChronotypeResult
{
    std::string timezone;
    int peakLocalHour{0};
    std::string peakTime;
    std::string type;
    std::string reportName;
    std::string insight;
    int peakActionsCount{0};
    int totalActionsCount{0};
};

class ChronotypeService
{
  public:
    struct ChronotypeDefinition
    {
        std::string peakTime;
        std::string type;
        std::string reportName;
        std::string insight;
    };

    std::optional<ChronotypeResult> computeQuarter(const drogon::orm::DbClientPtr &dbClient,
                                                   int quarter,
                                                   int userId,
                                                   int courseId,
                                                   ApiError &error) const;

    std::optional<ChronotypeResult> computeYear(const drogon::orm::DbClientPtr &dbClient,
                                                int userId,
                                                int courseId,
                                                ApiError &error) const;

  private:
    struct CourseTimeContext
    {
        int userId{0};
        int courseId{0};
        int usersCourseId{0};
        std::string timezone;
        std::string startedAt;
        std::string finishedAt;
    };

    struct HourBucket
    {
        int localHour{0};
        int actionsCount{0};
    };

    std::optional<CourseTimeContext> loadCourseContext(const drogon::orm::DbClientPtr &dbClient,
                                                       int userId,
                                                       int courseId,
                                                       ApiError &error) const;

    std::optional<std::vector<HourBucket>> loadYearBuckets(const drogon::orm::DbClientPtr &dbClient,
                                                           const CourseTimeContext &context,
                                                           ApiError &error) const;

    std::optional<std::vector<HourBucket>> loadQuarterBuckets(const drogon::orm::DbClientPtr &dbClient,
                                                              const CourseTimeContext &context,
                                                              int quarter,
                                                              ApiError &error) const;

    static ChronotypeResult buildResult(const std::vector<HourBucket> &buckets,
                                        const std::string &timezone);
    static ChronotypeDefinition definitionForHour(int localHour);
};
}  // namespace yearreporter::services
