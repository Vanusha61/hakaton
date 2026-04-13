#pragma once

#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <drogon/HttpResponse.h>
#include <drogon/orm/DbClient.h>

namespace yearreporter::services
{
struct ApiError
{
    drogon::HttpStatusCode status{drogon::k500InternalServerError};
    std::string reason;
};

struct CountMetrics
{
    int valueCount{0};
    int totalCount{0};
};

class CourseMetricsService
{
  public:
    struct UserCourse
    {
        int userId{0};
        int courseId{0};
        double earnedPoints{0.0};
        double maxPoints{0.0};
        int solvedTaskCount{0};
    };

    struct CourseLesson
    {
        int lessonId{0};
        std::optional<int> lessonNumber;
        double maxPoints{0.0};
        int maxTaskCount{0};
        bool hasVideo{false};
        bool hasPreparationMaterial{false};
    };

    struct UserLessonState
    {
        int lessonId{0};
        double earnedPoints{0.0};
        int solvedTasks{0};
        bool videoWatched{false};
        bool translationVisited{false};
    };

    struct QuarterSlice
    {
        size_t begin{0};
        size_t end{0};
    };

    std::optional<CountMetrics> computeVideoQuarter(const drogon::orm::DbClientPtr &dbClient,
                                                    int quarter,
                                                    int userId,
                                                    int courseId,
                                                    ApiError &error) const;

    std::optional<CountMetrics> computeVideoYear(const drogon::orm::DbClientPtr &dbClient,
                                                 int userId,
                                                 int courseId,
                                                 ApiError &error) const;

    std::optional<CountMetrics> computePointsQuarter(const drogon::orm::DbClientPtr &dbClient,
                                                     int quarter,
                                                     int userId,
                                                     int courseId,
                                                     ApiError &error) const;

    std::optional<CountMetrics> computePointsYear(const drogon::orm::DbClientPtr &dbClient,
                                                  int userId,
                                                  int courseId,
                                                  ApiError &error) const;

    std::optional<CountMetrics> computeConspectsQuarter(const drogon::orm::DbClientPtr &dbClient,
                                                        int quarter,
                                                        int userId,
                                                        int courseId,
                                                        ApiError &error) const;

    std::optional<CountMetrics> computeConspectsYear(const drogon::orm::DbClientPtr &dbClient,
                                                     int userId,
                                                     int courseId,
                                                     ApiError &error) const;

    std::optional<UserCourse> findUserCourse(const drogon::orm::DbClientPtr &dbClient,
                                             int userId,
                                             int courseId,
                                             ApiError &error) const;

    std::optional<int> findMatchingLessonTrack(const drogon::orm::DbClientPtr &dbClient,
                                               const UserCourse &course,
                                               ApiError &error) const;

    std::optional<std::vector<CourseLesson>> loadCourseLessons(const drogon::orm::DbClientPtr &dbClient,
                                                               int courseId,
                                                               ApiError &error) const;

    std::optional<std::vector<UserLessonState>> loadUserLessonStates(
        const drogon::orm::DbClientPtr &dbClient,
        int userId,
        int usersCourseId,
        ApiError &error) const;

    std::optional<std::unordered_set<int>> loadCoursePreparationMaterials(
        const drogon::orm::DbClientPtr &dbClient,
        int courseId,
        ApiError &error) const;

    std::optional<std::unordered_set<int>> loadVisitedPreparationMaterials(
        const drogon::orm::DbClientPtr &dbClient,
        int usersCourseId,
        ApiError &error) const;

    static QuarterSlice quarterSliceFor(const std::vector<CourseLesson> &lessons, int quarter);
    static int roundCount(double value);

  private:
};
}  // namespace yearreporter::services
