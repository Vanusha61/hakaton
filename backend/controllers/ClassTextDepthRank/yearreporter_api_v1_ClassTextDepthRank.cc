#include "yearreporter_api_v1_ClassTextDepthRank.h"

#include "controllers/common/ApiHelpers.h"
#include "services/SocialRankingsService.h"

using namespace api::v1;

namespace
{
Json::Value toQuarterJson(int userId,
                          int courseId,
                          int quarter,
                          const yearreporter::services::ClassTextDepthRankResult &result)
{
    Json::Value json;
    json["user_id"] = userId;
    json["course_id"] = courseId;
    json["quarter"] = quarter;
    json["class_name"] = result.className;
    json["learned_count"] = result.learnedCount;
    json["total_materials_count"] = result.totalMaterialsCount;
    json["user_rank"] = result.userRank;
    json["class_users_count"] = result.classUsersCount;
    json["same_value_users_count"] = result.sameValueUsersCount;
    json["display_mode"] = result.displayMode;
    json["display_value"] = result.displayValue;
    json["display_text"] = result.displayText;
    return json;
}

Json::Value toYearJson(int userId,
                       int courseId,
                       const yearreporter::services::ClassTextDepthRankResult &result)
{
    Json::Value json;
    json["user_id"] = userId;
    json["course_id"] = courseId;
    json["class_name"] = result.className;
    json["learned_count"] = result.learnedCount;
    json["total_materials_count"] = result.totalMaterialsCount;
    json["user_rank"] = result.userRank;
    json["class_users_count"] = result.classUsersCount;
    json["same_value_users_count"] = result.sameValueUsersCount;
    json["display_mode"] = result.displayMode;
    json["display_value"] = result.displayValue;
    json["display_text"] = result.displayText;
    return json;
}
}  // namespace

void class_text_depth_rank::quarter(const drogon::HttpRequestPtr &request,
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
    const auto result = service.computeClassTextDepthRankQuarter(
        drogon::app().getDbClient(), *quarterValue, *userId, *courseId, error);
    if (!result.has_value())
    {
        callback(makeErrorResponse(error.status, error.reason));
        return;
    }

    callback(makeJsonResponse(toQuarterJson(*userId, *courseId, *quarterValue, *result)));
}

void class_text_depth_rank::year(const drogon::HttpRequestPtr &request,
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
        service.computeClassTextDepthRankYear(drogon::app().getDbClient(), *userId, *courseId, error);
    if (!result.has_value())
    {
        callback(makeErrorResponse(error.status, error.reason));
        return;
    }

    callback(makeJsonResponse(toYearJson(*userId, *courseId, *result)));
}
