#pragma once

#include <optional>
#include <string>

#include <drogon/HttpResponse.h>
#include <drogon/drogon.h>

#include "services/ProgressService.h"

namespace yearreporter::controllers::common
{
inline drogon::HttpResponsePtr makeJsonResponse(
    const Json::Value &json,
    drogon::HttpStatusCode code = drogon::k200OK)
{
    auto response = drogon::HttpResponse::newHttpJsonResponse(json);
    response->setStatusCode(code);
    return response;
}

inline drogon::HttpResponsePtr makeErrorResponse(drogon::HttpStatusCode code,
                                                 const std::string &reason)
{
    Json::Value json;
    json["reason"] = reason;
    return makeJsonResponse(json, code);
}

inline std::optional<int> parseIntQueryParam(
    const drogon::HttpRequestPtr &request,
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
}  // namespace yearreporter::controllers::common
