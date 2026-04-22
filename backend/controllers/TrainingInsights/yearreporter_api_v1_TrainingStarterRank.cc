#include "yearreporter_api_v1_TrainingStarterRank.h"

#include "controllers/common/ApiHelpers.h"
#include "services/TrainingInsightsService.h"

using namespace api::v1;

namespace
{
Json::Value toQuarterJson(int userId,
                          int courseId,
                          int quarter,
                          const yearreporter::services::TrainingStarterRankResult &result)
{
    Json::Value json;
    json["user_id"] = userId;
    json["course_id"] = courseId;
    json["quarter"] = quarter;
    json["training_id"] = result.trainingId;
    json["training_name"] = result.trainingName;
    json["user_rank"] = result.userRank;
    json["is_user_first_starter"] = result.isUserFirstStarter;
    json["is_user_top3_starter"] = result.isUserTop3Starter;
    json["user_started_at"] = result.userStartedAt;
    json["global_first_started_at"] = result.globalFirstStartedAt;
    return json;
}

Json::Value toYearJson(int userId,
                       int courseId,
                       const yearreporter::services::TrainingStarterRankResult &result)
{
    Json::Value json;
    json["user_id"] = userId;
    json["course_id"] = courseId;
    json["training_id"] = result.trainingId;
    json["training_name"] = result.trainingName;
    json["user_rank"] = result.userRank;
    json["is_user_first_starter"] = result.isUserFirstStarter;
    json["is_user_top3_starter"] = result.isUserTop3Starter;
    json["user_started_at"] = result.userStartedAt;
    json["global_first_started_at"] = result.globalFirstStartedAt;
    return json;
}
}  // namespace

void training_starter_rank::quarter(const drogon::HttpRequestPtr &request,
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
    const auto result =
        service.computeStarterRankQuarter(drogon::app().getDbClient(), *quarterValue, *userId, *courseId, error);
    if (!result.has_value())
    {
        callback(makeErrorResponse(error.status, error.reason));
        return;
    }

    callback(makeJsonResponse(toQuarterJson(*userId, *courseId, *quarterValue, *result)));
}

void training_starter_rank::year(const drogon::HttpRequestPtr &request,
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
        service.computeStarterRankYear(drogon::app().getDbClient(), *userId, *courseId, error);
    if (!result.has_value())
    {
        callback(makeErrorResponse(error.status, error.reason));
        return;
    }

    callback(makeJsonResponse(toYearJson(*userId, *courseId, *result)));
}
