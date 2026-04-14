#include "yearreporter_api_v1_FirstVideoStarter.h"

#include "controllers/common/ApiHelpers.h"
#include "services/VideoStoryService.h"

using namespace api::v1;

namespace
{
Json::Value toQuarterJson(int userId,
                          int courseId,
                          int quarter,
                          const yearreporter::services::FirstVideoStarterResult &result)
{
    Json::Value json;
    json["user_id"] = userId;
    json["course_id"] = courseId;
    json["quarted_id"] = quarter;
    json["is_user_first_video_starter"] = result.isUserFirstVideoStarter;
    json["user_first_started_at"] = result.userFirstStartedAt;
    json["global_first_started_at"] = result.globalFirstStartedAt;
    return json;
}

Json::Value toYearJson(int userId,
                       int courseId,
                       const yearreporter::services::FirstVideoStarterResult &result)
{
    Json::Value json;
    json["user_id"] = userId;
    json["course_id"] = courseId;
    json["is_user_first_video_starter"] = result.isUserFirstVideoStarter;
    json["user_first_started_at"] = result.userFirstStartedAt;
    json["global_first_started_at"] = result.globalFirstStartedAt;
    return json;
}
}  // namespace

void first_video_starter::quarter(const drogon::HttpRequestPtr &request,
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
        service.computeFirstVideoStarterQuarter(drogon::app().getDbClient(),
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

void first_video_starter::year(const drogon::HttpRequestPtr &request,
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
        service.computeFirstVideoStarterYear(drogon::app().getDbClient(), *userId, *courseId, error);
    if (!result.has_value())
    {
        callback(makeErrorResponse(error.status, error.reason));
        return;
    }

    callback(makeJsonResponse(toYearJson(*userId, *courseId, *result)));
}
