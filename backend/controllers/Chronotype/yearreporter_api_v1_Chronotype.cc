#include "yearreporter_api_v1_Chronotype.h"

#include "controllers/common/ApiHelpers.h"
#include "services/ChronotypeService.h"

using namespace api::v1;

namespace
{
Json::Value toQuarterJson(int userId,
                          int courseId,
                          int quarter,
                          const yearreporter::services::ChronotypeResult &result)
{
    Json::Value json;
    json["user_id"] = userId;
    json["course_id"] = courseId;
    json["quarter"] = quarter;
    json["timezone"] = result.timezone;
    json["peak_local_hour"] = result.peakLocalHour;
    json["peak_time"] = result.peakTime;
    json["type"] = result.type;
    json["report_name"] = result.reportName;
    json["insight"] = result.insight;
    json["peak_actions_count"] = result.peakActionsCount;
    json["total_actions_count"] = result.totalActionsCount;
    return json;
}

Json::Value toYearJson(int userId,
                       int courseId,
                       const yearreporter::services::ChronotypeResult &result)
{
    Json::Value json;
    json["user_id"] = userId;
    json["course_id"] = courseId;
    json["timezone"] = result.timezone;
    json["peak_local_hour"] = result.peakLocalHour;
    json["peak_time"] = result.peakTime;
    json["type"] = result.type;
    json["report_name"] = result.reportName;
    json["insight"] = result.insight;
    json["peak_actions_count"] = result.peakActionsCount;
    json["total_actions_count"] = result.totalActionsCount;
    return json;
}
}  // namespace

void chronotype::quarter(const drogon::HttpRequestPtr &request,
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

    yearreporter::services::ChronotypeService service;
    const auto result =
        service.computeQuarter(drogon::app().getDbClient(), *quarterValue, *userId, *courseId, error);
    if (!result.has_value())
    {
        callback(makeErrorResponse(error.status, error.reason));
        return;
    }

    callback(makeJsonResponse(toQuarterJson(*userId, *courseId, *quarterValue, *result)));
}

void chronotype::year(const drogon::HttpRequestPtr &request,
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

    yearreporter::services::ChronotypeService service;
    const auto result = service.computeYear(drogon::app().getDbClient(), *userId, *courseId, error);
    if (!result.has_value())
    {
        callback(makeErrorResponse(error.status, error.reason));
        return;
    }

    callback(makeJsonResponse(toYearJson(*userId, *courseId, *result)));
}
