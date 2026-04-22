#include "yearreporter_api_v1_Progress.h"

#include <optional>
#include <string>

#include <drogon/drogon.h>

#include "services/ProgressService.h"

using namespace api::v1;

namespace
{
drogon::HttpResponsePtr makeJsonResponse(const Json::Value &json,
                                         drogon::HttpStatusCode code = drogon::k200OK)
{
    auto response = drogon::HttpResponse::newHttpJsonResponse(json);
    response->setStatusCode(code);
    return response;
}

drogon::HttpResponsePtr makeErrorResponse(drogon::HttpStatusCode code, const std::string &reason)
{
    Json::Value json;
    json["reason"] = reason;
    return makeJsonResponse(json, code);
}

std::optional<int> parseIntQueryParam(const drogon::HttpRequestPtr &request,
                                      const std::string &name,
                                      yearreporter::services::ApiError &error)
{
    const auto value = request->getParameter(name);
    if (value.empty())
    {
        error = {drogon::k400BadRequest,
                 "Не указан обязательный параметр '" + name + "'"};
        return std::nullopt;
    }

    try
    {
        size_t pos = 0;
        const int parsed = std::stoi(value, &pos);
        if (pos != value.size())
        {
            throw std::invalid_argument("tail");
        }
        return parsed;
    }
    catch (const std::exception &)
    {
        error = {drogon::k400BadRequest,
                 "Параметр '" + name + "' должен быть целым числом"};
        return std::nullopt;
    }
}
}  // namespace

void progress::quarter(const drogon::HttpRequestPtr &request,
                       std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    yearreporter::services::ApiError error;
    const auto quarterValue = parseIntQueryParam(request, "quarter", error);
    if (!quarterValue.has_value())
    {
        callback(makeErrorResponse(error.status, error.reason));
        return;
    }

    const auto userId = parseIntQueryParam(request, "user_id", error);
    if (!userId.has_value())
    {
        callback(makeErrorResponse(error.status, error.reason));
        return;
    }

    const auto courseId = parseIntQueryParam(request, "course_id", error);
    if (!courseId.has_value())
    {
        callback(makeErrorResponse(error.status, error.reason));
        return;
    }

    yearreporter::services::ProgressService service;
    const auto progress =
        service.computeQuarterProgress(drogon::app().getDbClient(), *quarterValue, *userId, *courseId, error);
    if (!progress.has_value())
    {
        callback(makeErrorResponse(error.status, error.reason));
        return;
    }

    Json::Value json;
    json["quarter"] = progress->quarter;
    json["progress"] = progress->percent;
    json["user_id"] = *userId;
    json["course_id"] = *courseId;
    callback(makeJsonResponse(json));
}

void progress::course(const drogon::HttpRequestPtr &request,
                      std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    yearreporter::services::ApiError error;
    const auto userId = parseIntQueryParam(request, "user_id", error);
    if (!userId.has_value())
    {
        callback(makeErrorResponse(error.status, error.reason));
        return;
    }

    const auto courseId = parseIntQueryParam(request, "course_id", error);
    if (!courseId.has_value())
    {
        callback(makeErrorResponse(error.status, error.reason));
        return;
    }

    yearreporter::services::ProgressService service;
    const auto progress =
        service.computeCourseProgress(drogon::app().getDbClient(), *userId, *courseId, error);
    if (!progress.has_value())
    {
        callback(makeErrorResponse(error.status, error.reason));
        return;
    }

    Json::Value json;
    json["progress"] = *progress;
    json["user_id"] = *userId;
    json["course_id"] = *courseId;
    callback(makeJsonResponse(json));
}
