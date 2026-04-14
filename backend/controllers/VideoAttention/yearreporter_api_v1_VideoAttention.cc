#include "yearreporter_api_v1_VideoAttention.h"

#include "controllers/common/ApiHelpers.h"
#include "services/VideoStoryService.h"

using namespace api::v1;

namespace
{
Json::Value toQuarterJson(int userId,
                          int courseId,
                          int quarter,
                          const yearreporter::services::VideoAttentionProfileResult &result)
{
    Json::Value json;
    json["user_id"] = userId;
    json["course_id"] = courseId;
    json["quarted_id"] = quarter;
    json["peak_position_percentage"] = result.peakPositionPercentage;
    json["role"] = result.role;
    json["report_name"] = result.reportName;
    json["insight"] = result.insight;
    return json;
}

Json::Value toYearJson(int userId,
                       int courseId,
                       const yearreporter::services::VideoAttentionProfileResult &result)
{
    Json::Value json;
    json["user_id"] = userId;
    json["course_id"] = courseId;
    json["peak_position_percentage"] = result.peakPositionPercentage;
    json["role"] = result.role;
    json["report_name"] = result.reportName;
    json["insight"] = result.insight;
    return json;
}
}  // namespace

void video_attention::quarter(const drogon::HttpRequestPtr &request,
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

    yearreporter::services::VideoStoryService service;
    const auto result =
        service.computeAttentionProfileQuarter(drogon::app().getDbClient(),
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

void video_attention::year(const drogon::HttpRequestPtr &request,
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

    yearreporter::services::VideoStoryService service;
    const auto result =
        service.computeAttentionProfileYear(drogon::app().getDbClient(), *userId, *courseId, error);
    if (!result.has_value())
    {
        callback(makeErrorResponse(error.status, error.reason));
        return;
    }

    callback(makeJsonResponse(toYearJson(*userId, *courseId, *result)));
}
