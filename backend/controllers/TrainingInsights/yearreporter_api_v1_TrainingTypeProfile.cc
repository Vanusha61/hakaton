#include "yearreporter_api_v1_TrainingTypeProfile.h"

#include "controllers/common/ApiHelpers.h"
#include "services/TrainingInsightsService.h"

using namespace api::v1;

namespace
{
Json::Value toYearJson(int userId,
                       int courseId,
                       const yearreporter::services::TrainingTypeProfileResult &result)
{
    Json::Value json;
    json["user_id"] = userId;
    json["course_id"] = courseId;
    json["lesson_training_count"] = result.lessonTrainingCount;
    json["lesson_training_percentage"] = result.lessonTrainingPercentage;
    json["regular_training_count"] = result.regularTrainingCount;
    json["regular_training_percentage"] = result.regularTrainingPercentage;
    json["olympiad_training_count"] = result.olympiadTrainingCount;
    json["olympiad_training_percentage"] = result.olympiadTrainingPercentage;
    json["total_count"] = result.totalCount;
    json["has_olympiad_training"] = result.hasOlympiadTraining;
    json["report_name"] = result.reportName;
    json["insight"] = result.insight;
    return json;
}
}  // namespace

void training_type_profile::year(const drogon::HttpRequestPtr &request,
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
        service.computeTypeProfileYear(drogon::app().getDbClient(), *userId, *courseId, error);
    if (!result.has_value())
    {
        callback(makeErrorResponse(error.status, error.reason));
        return;
    }

    callback(makeJsonResponse(toYearJson(*userId, *courseId, *result)));
}
