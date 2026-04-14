#include "yearreporter_api_v1_RarestVisitedLesson.h"

#include "controllers/common/ApiHelpers.h"
#include "services/LessonAchievementsService.h"

using namespace api::v1;

namespace
{
Json::Value toQuarterJson(int userId,
                          int courseId,
                          int quarter,
                          const yearreporter::services::RarestVisitedLessonResult &result)
{
    Json::Value json;
    json["user_id"] = userId;
    json["course_id"] = courseId;
    json["quarted_id"] = quarter;
    json["title"] = result.title;
    json["visited_percentage"] = result.visitedPercentage;
    json["visited_users_count"] = result.visitedUsersCount;
    json["total_users_count"] = result.totalUsersCount;
    json["report_name"] = result.reportName;
    json["insight"] = result.insight;
    return json;
}

Json::Value toYearJson(int userId,
                       int courseId,
                       const yearreporter::services::RarestVisitedLessonResult &result)
{
    Json::Value json;
    json["user_id"] = userId;
    json["course_id"] = courseId;
    json["title"] = result.title;
    json["visited_percentage"] = result.visitedPercentage;
    json["visited_users_count"] = result.visitedUsersCount;
    json["total_users_count"] = result.totalUsersCount;
    json["report_name"] = result.reportName;
    json["insight"] = result.insight;
    return json;
}
}  // namespace

void rarest_visited_lesson::quarter(const drogon::HttpRequestPtr &request,
                                    std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    using namespace yearreporter::controllers::common;
    yearreporter::services::ApiError error;
    const auto quarterValue = parseIntQueryParam(request, "quarter", error);
    const auto userId = parseIntQueryParam(request, "user_id", error);
    const auto courseId = parseIntQueryParam(request, "course_id", error);

    if (!quarterValue.has_value() || !userId.has_value() || !courseId.has_value())
    {
        callback(makeErrorResponse(error.status, error.reason));
        return;
    }

    yearreporter::services::LessonAchievementsService service;
    const auto result =
        service.computeRarestVisitedLessonQuarter(drogon::app().getDbClient(),
                                                  *quarterValue,
                                                  *userId,
                                                  *courseId,
                                                  error);
    if (!result.has_value())
    {
        callback(makeErrorResponse(error.status, error.reason));
        return;
    }

    callback(makeJsonResponse(toQuarterJson(*userId, *courseId, *quarterValue, *result)));
}

void rarest_visited_lesson::year(const drogon::HttpRequestPtr &request,
                                 std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    using namespace yearreporter::controllers::common;
    yearreporter::services::ApiError error;
    const auto userId = parseIntQueryParam(request, "user_id", error);
    const auto courseId = parseIntQueryParam(request, "course_id", error);

    if (!userId.has_value() || !courseId.has_value())
    {
        callback(makeErrorResponse(error.status, error.reason));
        return;
    }

    yearreporter::services::LessonAchievementsService service;
    const auto result =
        service.computeRarestVisitedLessonYear(drogon::app().getDbClient(), *userId, *courseId, error);
    if (!result.has_value())
    {
        callback(makeErrorResponse(error.status, error.reason));
        return;
    }

    callback(makeJsonResponse(toYearJson(*userId, *courseId, *result)));
}
