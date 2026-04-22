#include "yearreporter_api_v1_AverageScore.h"

#include <iomanip>
#include <sstream>

#include "controllers/common/ApiHelpers.h"
#include "services/TaskStatsService.h"

using namespace api::v1;

namespace
{
std::string formatFixed2(double value)
{
    std::ostringstream out;
    out.setf(std::ios::fixed);
    out << std::setprecision(2) << value;
    return out.str();
}

drogon::HttpResponsePtr makeAverageScoreQuarterResponse(
    int userId,
    int courseId,
    int quarter,
    const yearreporter::services::AverageScoreResult &result)
{
    const auto earnedPoints = formatFixed2(result.earnedPoints);
    const auto averageScore = formatFixed2(result.averageScore);

    std::ostringstream body;
    body << "{"
         << "\"user_id\":" << userId << ","
         << "\"course_id\":" << courseId << ","
         << "\"quarted_id\":" << quarter << ","
         << "\"solved_tasks_count\":" << result.solvedTasksCount << ","
         << "\"earned_points\":" << earnedPoints << ","
         << "\"earned_points_display\":\"" << earnedPoints << "\","
         << "\"average_score\":" << averageScore << ","
         << "\"average_score_display\":\"" << averageScore << "\""
         << "}";

    auto response = drogon::HttpResponse::newHttpResponse();
    response->setStatusCode(drogon::k200OK);
    response->setContentTypeCode(drogon::CT_APPLICATION_JSON);
    response->setBody(body.str());
    return response;
}

drogon::HttpResponsePtr makeAverageScoreYearResponse(
    int userId,
    int courseId,
    const yearreporter::services::AverageScoreResult &result)
{
    const auto earnedPoints = formatFixed2(result.earnedPoints);
    const auto averageScore = formatFixed2(result.averageScore);

    std::ostringstream body;
    body << "{"
         << "\"user_id\":" << userId << ","
         << "\"course_id\":" << courseId << ","
         << "\"solved_tasks_count\":" << result.solvedTasksCount << ","
         << "\"earned_points\":" << earnedPoints << ","
         << "\"earned_points_display\":\"" << earnedPoints << "\","
         << "\"average_score\":" << averageScore << ","
         << "\"average_score_display\":\"" << averageScore << "\""
         << "}";

    auto response = drogon::HttpResponse::newHttpResponse();
    response->setStatusCode(drogon::k200OK);
    response->setContentTypeCode(drogon::CT_APPLICATION_JSON);
    response->setBody(body.str());
    return response;
}
}  // namespace

void average_score::quarter(const drogon::HttpRequestPtr &request,
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

    yearreporter::services::TaskStatsService service;
    const auto result =
        service.computeAverageScoreQuarter(drogon::app().getDbClient(),
                                           *quarterValue,
                                           *userId,
                                           *courseId,
                                           error);
    if (!result.has_value())
    {
        callback(makeErrorResponse(error.status, error.reason));
        return;
    }

    callback(makeAverageScoreQuarterResponse(*userId, *courseId, *quarterValue, *result));
}

void average_score::year(const drogon::HttpRequestPtr &request,
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

    yearreporter::services::TaskStatsService service;
    const auto result =
        service.computeAverageScoreYear(drogon::app().getDbClient(), *userId, *courseId, error);
    if (!result.has_value())
    {
        callback(makeErrorResponse(error.status, error.reason));
        return;
    }

    callback(makeAverageScoreYearResponse(*userId, *courseId, *result));
}
