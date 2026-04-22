#include "yearreporter_api_v1_TrainingDifficultyStats.h"

#include "controllers/common/ApiHelpers.h"
#include "services/TrainingInsightsService.h"

using namespace api::v1;

namespace
{
Json::Value statsJson(const yearreporter::services::TrainingDifficultyStatsResult &result)
{
    Json::Value json;
    json["easy_count"] = result.easyCount;
    json["medium_count"] = result.mediumCount;
    json["hard_count"] = result.hardCount;
    json["total_count"] = result.totalCount;
    return json;
}

Json::Value toQuarterJson(int userId,
                          int courseId,
                          int quarter,
                          const yearreporter::services::TrainingDifficultyStatsResult &result)
{
    Json::Value json;
    json["user_id"] = userId;
    json["course_id"] = courseId;
    json["quarter"] = quarter;
    json["difficulty_stats"] = statsJson(result);
    return json;
}

Json::Value toYearJson(int userId,
                       int courseId,
                       const yearreporter::services::TrainingDifficultyStatsResult &result)
{
    Json::Value json;
    json["user_id"] = userId;
    json["course_id"] = courseId;
    json["difficulty_stats"] = statsJson(result);
    return json;
}
}  // namespace

void training_difficulty_stats::quarter(const drogon::HttpRequestPtr &request,
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

    yearreporter::services::TrainingInsightsService service;
    const auto result = service.computeDifficultyStatsQuarter(
        drogon::app().getDbClient(), *quarterValue, *userId, *courseId, error);
    if (!result.has_value())
    {
        callback(makeErrorResponse(error.status, error.reason));
        return;
    }

    callback(makeJsonResponse(toQuarterJson(*userId, *courseId, *quarterValue, *result)));
}

void training_difficulty_stats::year(const drogon::HttpRequestPtr &request,
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

    yearreporter::services::TrainingInsightsService service;
    const auto result =
        service.computeDifficultyStatsYear(drogon::app().getDbClient(), *userId, *courseId, error);
    if (!result.has_value())
    {
        callback(makeErrorResponse(error.status, error.reason));
        return;
    }

    callback(makeJsonResponse(toYearJson(*userId, *courseId, *result)));
}
