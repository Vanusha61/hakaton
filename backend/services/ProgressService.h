#pragma once

#include <optional>
#include <string>
#include <vector>

#include "CourseMetricsService.h"

namespace yearreporter::services
{
struct QuarterMetrics
{
    int quarter{0};
    double fraction{0.0};
    int percent{0};
};

class ProgressService
{
  public:
    std::optional<QuarterMetrics> computeQuarterProgress(const drogon::orm::DbClientPtr &dbClient,
                                                         int quarter,
                                                         int userId,
                                                         int courseId,
                                                         ApiError &error) const;

    std::optional<int> computeCourseProgress(const drogon::orm::DbClientPtr &dbClient,
                                             int userId,
                                             int courseId,
                                             ApiError &error) const;

  private:
    static QuarterMetrics calculateQuarterMetrics(const CourseMetricsService::UserCourse &course,
                                                  const std::vector<CourseMetricsService::CourseLesson> &lessons,
                                                  const std::vector<CourseMetricsService::UserLessonState> &userStates,
                                                  int quarter);
};
}  // namespace yearreporter::services
