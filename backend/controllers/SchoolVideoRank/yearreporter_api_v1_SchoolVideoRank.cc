#include "yearreporter_api_v1_SchoolVideoRank.h"

#include "controllers/common/ApiHelpers.h"
#include "services/SchoolRankingsService.h"

using namespace api::v1;

namespace
{
Json::Value toQuarterJson(int userId,
                          int courseId,
                          int quarter,
                          const yearreporter::services::SchoolVideoRankResult &result)
{
    Json::Value json;
    json["user_id"] = userId;
    json["course_id"] = courseId;
    json["quarter"] = quarter;
    json["school_name"] = result.schoolName;
    json["watched_lessons_count"] = result.watchedLessonsCount;
    json["total_video_lessons_count"] = result.totalVideoLessonsCount;
    json["user_rank"] = result.userRank;
    json["school_users_count"] = result.schoolUsersCount;
    json["same_value_users_count"] = result.sameValueUsersCount;
    json["display_mode"] = result.displayMode;
    json["display_value"] = result.displayValue;
    json["display_text"] = result.displayText;
    return json;
}

Json::Value toYearJson(int userId,
                       int courseId,
                       const yearreporter::services::SchoolVideoRankResult &result)
{
    Json::Value json;
    json["user_id"] = userId;
    json["course_id"] = courseId;
    json["school_name"] = result.schoolName;
    json["watched_lessons_count"] = result.watchedLessonsCount;
    json["total_video_lessons_count"] = result.totalVideoLessonsCount;
    json["user_rank"] = result.userRank;
    json["school_users_count"] = result.schoolUsersCount;
    json["same_value_users_count"] = result.sameValueUsersCount;
    json["display_mode"] = result.displayMode;
    json["display_value"] = result.displayValue;
    json["display_text"] = result.displayText;
    return json;
}
}  // namespace

void school_video_rank::quarter(const drogon::HttpRequestPtr &request,
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

    yearreporter::services::SchoolRankingsService service;
    const auto result =
        service.computeVideoRankQuarter(drogon::app().getDbClient(), *quarterValue, *userId, *courseId, error);
    if (!result.has_value())
    {
        callback(makeErrorResponse(error.status, error.reason));
        return;
    }

    callback(makeJsonResponse(toQuarterJson(*userId, *courseId, *quarterValue, *result)));
}

void school_video_rank::year(const drogon::HttpRequestPtr &request,
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

    yearreporter::services::SchoolRankingsService service;
    const auto result =
        service.computeVideoRankYear(drogon::app().getDbClient(), *userId, *courseId, error);
    if (!result.has_value())
    {
        callback(makeErrorResponse(error.status, error.reason));
        return;
    }

    callback(makeJsonResponse(toYearJson(*userId, *courseId, *result)));
}
