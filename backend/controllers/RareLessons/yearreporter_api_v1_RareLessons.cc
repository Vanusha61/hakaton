#include "yearreporter_api_v1_RareLessons.h"

#include "controllers/common/ApiHelpers.h"
#include "services/LessonAchievementsService.h"

using namespace api::v1;

namespace
{
Json::Value makeRareLessonsJson(const yearreporter::services::RareLessonsResult &result)
{
    Json::Value json(Json::objectValue);
    for (size_t index = 0; index < result.items.size(); ++index)
    {
        Json::Value lessonJson;
        lessonJson["title"] = result.items[index].title;
        lessonJson["not_solved_percentage"] = result.items[index].notSolvedPercentage;
        json[std::to_string(index + 1)] = lessonJson;
    }
    return json;
}
}  // namespace

void rare_lessons::quarter(const drogon::HttpRequestPtr &request,
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

    yearreporter::services::LessonAchievementsService service;
    const auto rareLessons =
        service.computeRareLessonsQuarter(drogon::app().getDbClient(),
                                          *quarterValue,
                                          *userId,
                                          *courseId,
                                          error);
    if (!rareLessons.has_value())
    {
        callback(makeErrorResponse(error.status, error.reason));
        return;
    }

    Json::Value json;
    json["user_id"] = *userId;
    json["course_id"] = *courseId;
    json["quarted_id"] = *quarterValue;
    json["rare_lessons"] = makeRareLessonsJson(*rareLessons);
    callback(makeJsonResponse(json));
}

void rare_lessons::year(const drogon::HttpRequestPtr &request,
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

    yearreporter::services::LessonAchievementsService service;
    const auto rareLessons =
        service.computeRareLessonsYear(drogon::app().getDbClient(), *userId, *courseId, error);
    if (!rareLessons.has_value())
    {
        callback(makeErrorResponse(error.status, error.reason));
        return;
    }

    Json::Value json;
    json["user_id"] = *userId;
    json["course_id"] = *courseId;
    json["rare_lessons"] = makeRareLessonsJson(*rareLessons);
    callback(makeJsonResponse(json));
}
