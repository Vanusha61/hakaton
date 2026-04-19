#include "yearreporter_api_v1_PartialCreditShare.h"

#include "controllers/common/ApiHelpers.h"
#include "services/PartialCreditShareService.h"

using namespace api::v1;

namespace
{
Json::Value toYearJson(int userId,
                       int courseId,
                       const yearreporter::services::PartialCreditShareResult &result)
{
    Json::Value json;
    json["user_id"] = userId;
    json["course_id"] = courseId;
    json["partial_answers_count"] = result.partialAnswersCount;
    json["partial_points"] = result.partialPoints;
    json["total_points"] = result.totalPoints;
    json["partial_points_percentage"] = result.partialPointsPercentage;
    json["report_name"] = result.reportName;
    json["insight"] = result.insight;
    return json;
}
}  // namespace

void partial_credit_share::year(const drogon::HttpRequestPtr &request,
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

    yearreporter::services::PartialCreditShareService service;
    const auto result = service.computeYear(drogon::app().getDbClient(), *userId, *courseId, error);
    if (!result.has_value())
    {
        callback(makeErrorResponse(error.status, error.reason));
        return;
    }

    callback(makeJsonResponse(toYearJson(*userId, *courseId, *result)));
}
