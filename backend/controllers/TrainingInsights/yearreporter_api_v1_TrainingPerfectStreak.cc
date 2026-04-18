#include "yearreporter_api_v1_TrainingPerfectStreak.h"

#include "controllers/common/ApiHelpers.h"
#include "services/TrainingInsightsService.h"

using namespace api::v1;

namespace
{
Json::Value toQuarterJson(int userId,
                          int courseId,
                          int quarter,
                          const yearreporter::services::TrainingPerfectStreakResult &result)
{
    Json::Value json;
    json["user_id"] = userId;
    json["course_id"] = courseId;
    json["quarter"] = quarter;
    json["training_id"] = result.trainingId;
    json["training_name"] = result.trainingName;
    json["max_solved_without_mistakes"] = result.maxSolvedWithoutMistakes;
    json["report_name"] = result.reportName;
    json["insight"] = result.insight;
    return json;
}

Json::Value toYearJson(int userId,
                       int courseId,
                       const yearreporter::services::TrainingPerfectStreakResult &result)
{
    Json::Value json;
    json["user_id"] = userId;
    json["course_id"] = courseId;
    json["training_id"] = result.trainingId;
    json["training_name"] = result.trainingName;
    json["max_solved_without_mistakes"] = result.maxSolvedWithoutMistakes;
    json["report_name"] = result.reportName;
    json["insight"] = result.insight;
    return json;
}
}  // namespace

void training_perfect_streak::quarter(const drogon::HttpRequestPtr &request,
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
        service.computePerfectStreakQuarter(drogon::app().getDbClient(), *quarterValue, *userId, *courseId, error);
    if (!result.has_value())
    {
        callback(makeErrorResponse(error.status, error.reason));
        return;
    }

    callback(makeJsonResponse(toQuarterJson(*userId, *courseId, *quarterValue, *result)));
}

void training_perfect_streak::year(const drogon::HttpRequestPtr &request,
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
        service.computePerfectStreakYear(drogon::app().getDbClient(), *userId, *courseId, error);
    if (!result.has_value())
    {
        callback(makeErrorResponse(error.status, error.reason));
        return;
    }

    callback(makeJsonResponse(toYearJson(*userId, *courseId, *result)));
}
