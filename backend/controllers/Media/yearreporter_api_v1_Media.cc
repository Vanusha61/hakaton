#include "yearreporter_api_v1_Media.h"

#include "controllers/common/ApiHelpers.h"
#include "services/MediaStatsService.h"

using namespace api::v1;

namespace
{
Json::Value toQuarterJson(int userId,
                          int courseId,
                          int quarter,
                          const yearreporter::services::MediaStats &stats)
{
    Json::Value json;
    json["user_id"] = userId;
    json["course_id"] = courseId;
    json["quarted_id"] = quarter;
    json["live_lesson_count"] = stats.liveLesson.count;
    json["live_lesson_percentage"] = stats.liveLesson.percentage;
    json["recorded_lesson_count"] = stats.recordedLesson.count;
    json["recorded_lesson_percentage"] = stats.recordedLesson.percentage;
    json["prerecorded_lesson_count"] = stats.prerecordedLesson.count;
    json["prerecorded_lesson_percentage"] = stats.prerecordedLesson.percentage;
    json["total_watched_videos_cnt"] = stats.totalWatchedVideosCount;
    return json;
}

Json::Value toYearJson(int userId,
                       int courseId,
                       const yearreporter::services::MediaStats &stats)
{
    Json::Value json;
    json["user_id"] = userId;
    json["course_id"] = courseId;
    json["live_lesson_count"] = stats.liveLesson.count;
    json["live_lesson_percentage"] = stats.liveLesson.percentage;
    json["recorded_lesson_count"] = stats.recordedLesson.count;
    json["recorded_lesson_percentage"] = stats.recordedLesson.percentage;
    json["prerecorded_lesson_count"] = stats.prerecordedLesson.count;
    json["prerecorded_lesson_percentage"] = stats.prerecordedLesson.percentage;
    json["total_watched_videos_cnt"] = stats.totalWatchedVideosCount;
    return json;
}
}  // namespace

void media::quarter(const drogon::HttpRequestPtr &request,
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

    yearreporter::services::MediaStatsService service;
    const auto stats =
        service.computeQuarter(drogon::app().getDbClient(), *quarterValue, *userId, *courseId, error);
    if (!stats.has_value())
    {
        callback(makeErrorResponse(error.status, error.reason));
        return;
    }

    callback(makeJsonResponse(toQuarterJson(*userId, *courseId, *quarterValue, *stats)));
}

void media::year(const drogon::HttpRequestPtr &request,
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

    yearreporter::services::MediaStatsService service;
    const auto stats =
        service.computeYear(drogon::app().getDbClient(), *userId, *courseId, error);
    if (!stats.has_value())
    {
        callback(makeErrorResponse(error.status, error.reason));
        return;
    }

    callback(makeJsonResponse(toYearJson(*userId, *courseId, *stats)));
}
