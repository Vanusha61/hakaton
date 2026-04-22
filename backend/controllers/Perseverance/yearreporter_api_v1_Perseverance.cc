#include "yearreporter_api_v1_Perseverance.h"

#include "controllers/common/ApiHelpers.h"
#include "services/PerseveranceService.h"

using namespace api::v1;

namespace
{
Json::Value toYearJson(int userId,
                       int courseId,
                       const yearreporter::services::PerseveranceResult &result)
{
    Json::Value json;
    json["user_id"] = userId;
    json["course_id"] = courseId;
    json["repeated_tasks_count"] = result.repeatedTasksCount;
    json["first_attempt_points"] = result.firstAttemptPoints;
    json["final_points"] = result.finalPoints;
    json["extra_points_from_retries"] = result.extraPointsFromRetries;
    json["report_name"] = result.reportName;
    json["insight"] = result.insight;
    return json;
}
}  // namespace

void perseverance::year(const drogon::HttpRequestPtr &request,
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

    yearreporter::services::PerseveranceService service;
    const auto result = service.computeYear(drogon::app().getDbClient(), *userId, *courseId, error);
    if (!result.has_value())
    {
        callback(makeErrorResponse(error.status, error.reason));
        return;
    }

    callback(makeJsonResponse(toYearJson(*userId, *courseId, *result)));
}
