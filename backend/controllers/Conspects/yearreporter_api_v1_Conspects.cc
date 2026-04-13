#include "yearreporter_api_v1_Conspects.h"

#include "controllers/common/ApiHelpers.h"
#include "services/CourseMetricsService.h"

using namespace api::v1;

void conspects::quarter(const drogon::HttpRequestPtr &request,
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

    yearreporter::services::CourseMetricsService service;
    const auto metrics =
        service.computeConspectsQuarter(drogon::app().getDbClient(), *quarterValue, *userId, *courseId, error);
    if (!metrics.has_value())
    {
        callback(makeErrorResponse(error.status, error.reason));
        return;
    }

    Json::Value json;
    json["course_id"] = *courseId;
    json["user_id"] = *userId;
    json["quarter"] = *quarterValue;
    json["learned_count"] = metrics->valueCount;
    json["total_count"] = metrics->totalCount;
    callback(makeJsonResponse(json));
}

void conspects::year(const drogon::HttpRequestPtr &request,
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

    yearreporter::services::CourseMetricsService service;
    const auto metrics =
        service.computeConspectsYear(drogon::app().getDbClient(), *userId, *courseId, error);
    if (!metrics.has_value())
    {
        callback(makeErrorResponse(error.status, error.reason));
        return;
    }

    Json::Value json;
    json["course_id"] = *courseId;
    json["user_id"] = *userId;
    json["learned_count"] = metrics->valueCount;
    json["total_count"] = metrics->totalCount;
    callback(makeJsonResponse(json));
}
