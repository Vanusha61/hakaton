#pragma once

#include <optional>
#include <string>

#include <drogon/orm/DbClient.h>

#include "CourseMetricsService.h"

namespace yearreporter::services
{
struct PerseveranceResult
{
    int repeatedTasksCount{0};
    double firstAttemptPoints{0.0};
    double finalPoints{0.0};
    double extraPointsFromRetries{0.0};
    std::string reportName;
    std::string insight;
};

class PerseveranceService
{
  public:
    std::optional<PerseveranceResult> computeYear(const drogon::orm::DbClientPtr &dbClient,
                                                  int userId,
                                                  int courseId,
                                                  ApiError &error) const;

  private:
    static double extractFirstAttemptPoints(const std::string &resultsJson, ApiError &error);
    static double roundToOneDecimal(double value);
    static std::string buildInsight(double extraPointsFromRetries);
};
}  // namespace yearreporter::services
