#include "yearreporter_api_v1_StudyRecommendations.h"

#include "controllers/common/ApiHelpers.h"
#include "services/StudyRecommendationsService.h"

using namespace api::v1;

namespace
{
Json::Value factorJson(const yearreporter::services::StudyRecommendationFactor &factor)
{
    Json::Value json;
    json["title"] = factor.title;
    json["level_percentage"] = factor.levelPercentage;
    json["influence"] = factor.influenceLabel;
    return json;
}

Json::Value quarterJson(int userId,
                        int courseId,
                        int quarter,
                        const yearreporter::services::StudyRecommendationResult &result)
{
    Json::Value json;
    json["user_id"] = userId;
    json["course_id"] = courseId;
    json["quarter"] = quarter;
    json["video"] = factorJson(result.video);
    json["conspects"] = factorJson(result.conspects);
    json["materials"] = factorJson(result.materials);
    json["current_average_score"] = result.currentAverageScore;
    json["target_average_score"] = result.targetAverageScore;
    json["recommended_focus_key"] = result.recommendedFocusKey;
    json["recommended_focus_title"] = result.recommendedFocusTitle;
    json["recommendation"] = result.recommendation;
    json["ai_insight"] = result.aiInsight;
    return json;
}

Json::Value yearJson(int userId,
                     int courseId,
                     const yearreporter::services::StudyRecommendationResult &result)
{
    Json::Value json;
    json["user_id"] = userId;
    json["course_id"] = courseId;
    json["video"] = factorJson(result.video);
    json["conspects"] = factorJson(result.conspects);
    json["materials"] = factorJson(result.materials);
    json["current_average_score"] = result.currentAverageScore;
    json["target_average_score"] = result.targetAverageScore;
    json["recommended_focus_key"] = result.recommendedFocusKey;
    json["recommended_focus_title"] = result.recommendedFocusTitle;
    json["recommendation"] = result.recommendation;
    json["ai_insight"] = result.aiInsight;
    return json;
}
}  // namespace

void study_recommendations::quarter(const drogon::HttpRequestPtr &request,
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

    yearreporter::services::StudyRecommendationsService service;
    const auto result =
        service.computeQuarter(drogon::app().getDbClient(), *quarterValue, *userId, *courseId, error);
    if (!result.has_value())
    {
        callback(makeErrorResponse(error.status, error.reason));
        return;
    }

    callback(makeJsonResponse(quarterJson(*userId, *courseId, *quarterValue, *result)));
}

void study_recommendations::year(const drogon::HttpRequestPtr &request,
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

    yearreporter::services::StudyRecommendationsService service;
    const auto result =
        service.computeYear(drogon::app().getDbClient(), *userId, *courseId, error);
    if (!result.has_value())
    {
        callback(makeErrorResponse(error.status, error.reason));
        return;
    }

    callback(makeJsonResponse(yearJson(*userId, *courseId, *result)));
}
