#include "yearreporter_api_v1_AwardBadgesLevels.h"

#include "controllers/common/ApiHelpers.h"
#include "services/AwardBadgesService.h"

using namespace api::v1;

namespace
{
Json::Value levelsJson(const yearreporter::services::AwardBadgesLevelsResult &result)
{
    Json::Value json;
    json["special_count"] = result.specialCount;
    json["total_count"] = result.totalCount;
    json["level_1_count"] = result.level1Count;
    json["level_2_count"] = result.level2Count;
    json["level_3_count"] = result.level3Count;
    json["level_4_count"] = result.level4Count;
    json["level_5_count"] = result.level5Count;
    return json;
}

Json::Value toQuarterJson(int userId,
                          int courseId,
                          int quarter,
                          const yearreporter::services::AwardBadgesLevelsResult &result)
{
    Json::Value json;
    json["user_id"] = userId;
    json["course_id"] = courseId;
    json["quarter"] = quarter;
    json["award_badges_levels"] = levelsJson(result);
    return json;
}

Json::Value toYearJson(int userId,
                       int courseId,
                       const yearreporter::services::AwardBadgesLevelsResult &result)
{
    Json::Value json;
    json["user_id"] = userId;
    json["course_id"] = courseId;
    json["award_badges_levels"] = levelsJson(result);
    return json;
}
}  // namespace

void award_badges_levels::quarter(const drogon::HttpRequestPtr &request,
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
        service.computeLevelsQuarter(drogon::app().getDbClient(), *quarterValue, *userId, *courseId, error);
    if (!result.has_value())
    {
        callback(makeErrorResponse(error.status, error.reason));
        return;
    }

    callback(makeJsonResponse(toQuarterJson(*userId, *courseId, *quarterValue, *result)));
}

void award_badges_levels::year(const drogon::HttpRequestPtr &request,
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
        service.computeLevelsYear(drogon::app().getDbClient(), *userId, *courseId, error);
    if (!result.has_value())
    {
        callback(makeErrorResponse(error.status, error.reason));
        return;
    }

    callback(makeJsonResponse(toYearJson(*userId, *courseId, *result)));
}
