#include "yearreporter_api_v1_AwardBadgesCount.h"

#include "controllers/common/ApiHelpers.h"
#include "services/AwardBadgesService.h"

using namespace api::v1;

namespace
{
Json::Value toQuarterJson(int userId,
                          int courseId,
                          int quarter,
                          const yearreporter::services::AwardBadgesCountResult &result)
{
    Json::Value json;
    json["user_id"] = userId;
    json["course_id"] = courseId;
    json["quarter"] = quarter;
    json["award_badges_count"] = result.totalCount;
    return json;
}

Json::Value toYearJson(int userId,
                       int courseId,
                       const yearreporter::services::AwardBadgesCountResult &result)
{
    Json::Value json;
    json["user_id"] = userId;
    json["course_id"] = courseId;
    json["award_badges_count"] = result.totalCount;
    return json;
}
}  // namespace

void award_badges_count::quarter(const drogon::HttpRequestPtr &request,
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

    yearreporter::services::AwardBadgesService service;
    const auto result =
        service.computeCountQuarter(drogon::app().getDbClient(), *quarterValue, *userId, *courseId, error);
    if (!result.has_value())
    {
        callback(makeErrorResponse(error.status, error.reason));
        return;
    }

    callback(makeJsonResponse(toQuarterJson(*userId, *courseId, *quarterValue, *result)));
}

void award_badges_count::year(const drogon::HttpRequestPtr &request,
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

    yearreporter::services::AwardBadgesService service;
    const auto result =
        service.computeCountYear(drogon::app().getDbClient(), *userId, *courseId, error);
    if (!result.has_value())
    {
        callback(makeErrorResponse(error.status, error.reason));
        return;
    }

    callback(makeJsonResponse(toYearJson(*userId, *courseId, *result)));
}
