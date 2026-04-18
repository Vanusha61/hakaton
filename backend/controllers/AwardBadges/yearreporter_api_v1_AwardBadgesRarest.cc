#include "yearreporter_api_v1_AwardBadgesRarest.h"

#include <cmath>

#include "controllers/common/ApiHelpers.h"
#include "services/AwardBadgesService.h"

using namespace api::v1;

namespace
{
Json::Value badgeJson(const yearreporter::services::RareAwardBadgeItem &item)
{
    Json::Value json;
    json["badge_id"] = item.badgeId;
    json["name"] = item.name;
    json["title"] = item.title;
    json["level"] = item.level;
    json["special"] = item.special;
    json["image_url"] = item.imageUrl;
    json["share_image_url"] = item.shareImageUrl;
    json["owners_count"] = item.ownersCount;
    json["total_users_count"] = item.totalUsersCount;
    json["owners_percentage"] = std::round(item.ownersPercentage * 10.0) / 10.0;
    return json;
}

Json::Value toYearJson(int userId,
                       int courseId,
                       const yearreporter::services::RarestAwardBadgesResult &result)
{
    Json::Value json;
    json["user_id"] = userId;
    json["course_id"] = courseId;

    Json::Value items(Json::arrayValue);
    for (const auto &item : result.items)
    {
        items.append(badgeJson(item));
    }

    json["rarest_award_badges"] = items;
    return json;
}
}  // namespace

void award_badges_rarest::year(const drogon::HttpRequestPtr &request,
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

    yearreporter::services::AwardBadgesService service;
    const auto result =
        service.computeRarestYear(drogon::app().getDbClient(), *userId, *courseId, error);
    if (!result.has_value())
    {
        callback(makeErrorResponse(error.status, error.reason));
        return;
    }

    callback(makeJsonResponse(toYearJson(*userId, *courseId, *result)));
}
