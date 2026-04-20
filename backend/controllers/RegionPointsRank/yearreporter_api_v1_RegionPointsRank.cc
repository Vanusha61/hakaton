#include "yearreporter_api_v1_RegionPointsRank.h"

#include "controllers/common/ApiHelpers.h"
#include "services/SocialRankingsService.h"

using namespace api::v1;

namespace
{
Json::Value toQuarterJson(int userId,
                          int courseId,
                          int quarter,
                          const yearreporter::services::RegionPointsRankResult &result)
{
    Json::Value json;
    json["user_id"] = userId;
    json["course_id"] = courseId;
    json["quarter"] = quarter;
    json["region_name"] = result.regionName;
    json["earned_points"] = result.earnedPoints;
    json["earned_points_display"] = result.earnedPointsDisplay;
    json["total_points"] = result.totalPoints;
    json["total_points_display"] = result.totalPointsDisplay;
    json["user_rank"] = result.userRank;
    json["region_users_count"] = result.regionUsersCount;
    json["same_value_users_count"] = result.sameValueUsersCount;
    json["display_mode"] = result.displayMode;
    json["display_value"] = result.displayValue;
    json["display_text"] = result.displayText;
    return json;
}

Json::Value toYearJson(int userId,
                       int courseId,
                       const yearreporter::services::RegionPointsRankResult &result)
{
    Json::Value json;
    json["user_id"] = userId;
    json["course_id"] = courseId;
    json["region_name"] = result.regionName;
    json["earned_points"] = result.earnedPoints;
    json["earned_points_display"] = result.earnedPointsDisplay;
    json["total_points"] = result.totalPoints;
    json["total_points_display"] = result.totalPointsDisplay;
    json["user_rank"] = result.userRank;
    json["region_users_count"] = result.regionUsersCount;
    json["same_value_users_count"] = result.sameValueUsersCount;
    json["display_mode"] = result.displayMode;
    json["display_value"] = result.displayValue;
    json["display_text"] = result.displayText;
    return json;
}
}  // namespace

void region_points_rank::quarter(const drogon::HttpRequestPtr &request,
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

    yearreporter::services::SocialRankingsService service;
    const auto result = service.computeRegionPointsRankQuarter(
        drogon::app().getDbClient(), *quarterValue, *userId, *courseId, error);
    if (!result.has_value())
    {
        callback(makeErrorResponse(error.status, error.reason));
        return;
    }

    callback(makeJsonResponse(toQuarterJson(*userId, *courseId, *quarterValue, *result)));
}

void region_points_rank::year(const drogon::HttpRequestPtr &request,
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

    yearreporter::services::SocialRankingsService service;
    const auto result =
        service.computeRegionPointsRankYear(drogon::app().getDbClient(), *userId, *courseId, error);
    if (!result.has_value())
    {
        callback(makeErrorResponse(error.status, error.reason));
        return;
    }

    callback(makeJsonResponse(toYearJson(*userId, *courseId, *result)));
}
