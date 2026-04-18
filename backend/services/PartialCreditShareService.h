#pragma once

#include <optional>
#include <string>

#include <drogon/orm/DbClient.h>

#include "CourseMetricsService.h"

namespace yearreporter::services
{
struct PartialCreditShareResult
{
    int partialAnswersCount{0};
    double partialPoints{0.0};
    double totalPoints{0.0};
    double partialPointsPercentage{0.0};
    std::string reportName;
    std::string insight;
};

class PartialCreditShareService
{
  public:
    std::optional<PartialCreditShareResult> computeYear(const drogon::orm::DbClientPtr &dbClient,
                                                        int userId,
                                                        int courseId,
                                                        ApiError &error) const;

  private:
    static double roundToOneDecimal(double value);
    static std::string buildInsight(double partialPointsPercentage);
};
}  // namespace yearreporter::services
