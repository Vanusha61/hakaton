#include "yearreporter_api_v1_Attendance.h"

#include "controllers/common/ApiHelpers.h"
#include "services/LessonAchievementsService.h"

using namespace api::v1;

void attendance::quarter(const drogon::HttpRequestPtr &request,
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
    const auto visitedAll =
        service.computeVisitedAllLessonsQuarter(drogon::app().getDbClient(),
                                                *quarterValue,
                                                *userId,
                                                *courseId,
                                                error);
    if (!visitedAll.has_value())
    {
        callback(makeErrorResponse(error.status, error.reason));
        return;
    }

    Json::Value json;
    json["user_id"] = *userId;
    json["course_id"] = *courseId;
    json["quarted_id"] = *quarterValue;
    json["is_user_visited_all_lessons"] = *visitedAll;
    callback(makeJsonResponse(json));
}

void attendance::year(const drogon::HttpRequestPtr &request,
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
    const auto visitedAll =
        service.computeVisitedAllLessonsYear(drogon::app().getDbClient(), *userId, *courseId, error);
    if (!visitedAll.has_value())
    {
        callback(makeErrorResponse(error.status, error.reason));
        return;
    }

    Json::Value json;
    json["user_id"] = *userId;
    json["course_id"] = *courseId;
    json["is_user_visited_all_lessons"] = *visitedAll;
    callback(makeJsonResponse(json));
}
