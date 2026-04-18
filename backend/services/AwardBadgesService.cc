#include "AwardBadgesService.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <set>
#include <sstream>

#include "constants/progress_constants.h"

namespace yearreporter::services
{
std::optional<std::chrono::system_clock::time_point> AwardBadgesService::parseIsoTimestamp(
    const std::string &value)
{
    if (value.empty())
    {
        return std::nullopt;
    }

    std::tm tm = {};
    std::istringstream stream(value.substr(0, 19));
    stream >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    if (stream.fail())
    {
        return std::nullopt;
    }

    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

std::optional<std::vector<AwardBadgesService::BadgeRow>> AwardBadgesService::loadYearBadges(
    const drogon::orm::DbClientPtr &dbClient,
    int userId,
    int courseId,
    ApiError &error) const
{
    CourseMetricsService courseService;
    const auto course = courseService.findUserCourse(dbClient, userId, courseId, error);
    if (!course.has_value())
    {
        return std::nullopt;
    }

    try
    {
        const auto result = dbClient->execSqlSync(
            "select uab.award_badge_id, "
            "       coalesce(ab.name, '') as name, "
            "       coalesce(ab.title, '') as title, "
            "       coalesce(ab.level, 0) as level, "
            "       coalesce(ab.special, false) as special, "
            "       coalesce(ab.image_url, '') as image_url, "
            "       coalesce(ab.share_image_url, '') as share_image_url, "
            "       coalesce(uab.created_at, '') as created_at "
            "from public.user_award_badges uab "
            "join public.award_badges ab on ab.id = uab.award_badge_id "
            "where uab.user_id = $1",
            userId);

        if (result.empty())
        {
            error = {drogon::k404NotFound, "Для пользователя не найдены достижения"};
            return std::nullopt;
        }

        std::vector<BadgeRow> badges;
        badges.reserve(result.size());
        for (const auto &row : result)
        {
            badges.push_back(BadgeRow{
                .badgeId = row["award_badge_id"].as<int>(),
                .name = row["name"].as<std::string>(),
                .title = row["title"].as<std::string>(),
                .level = row["level"].as<int>(),
                .special = row["special"].as<bool>(),
                .imageUrl = row["image_url"].as<std::string>(),
                .shareImageUrl = row["share_image_url"].as<std::string>(),
                .createdAt = row["created_at"].as<std::string>()});
        }
        return badges;
    }
    catch (const std::exception &e)
    {
        error = {drogon::k500InternalServerError,
                 "Ошибка загрузки достижений: " + std::string(e.what())};
        return std::nullopt;
    }
}

std::optional<std::vector<AwardBadgesService::BadgeRow>> AwardBadgesService::filterByQuarter(
    const std::vector<BadgeRow> &badges,
    int quarter,
    ApiError &error)
{
    if (quarter < 1 || quarter > yearreporter::constants::progress::kQuarterCount)
    {
        error = {drogon::k400BadRequest,
                 "Параметр 'quarter' должен быть числом от 1 до 4"};
        return std::nullopt;
    }

    std::vector<BadgeRow> filtered;
    filtered.reserve(badges.size());

    for (const auto &badge : badges)
    {
        const auto timestamp = parseIsoTimestamp(badge.createdAt);
        if (!timestamp.has_value())
        {
            continue;
        }

        const auto time = std::chrono::system_clock::to_time_t(*timestamp);
        const auto *tm = std::localtime(&time);
        if (tm == nullptr)
        {
            continue;
        }

        const int month = tm->tm_mon + 1;
        const int derivedQuarter = ((month - 1) / 3) + 1;
        if (derivedQuarter == quarter)
        {
            filtered.push_back(badge);
        }
    }

    if (filtered.empty())
    {
        error = {drogon::k404NotFound, "Для пользователя не найдены достижения в выбранной четверти"};
        return std::nullopt;
    }

    return filtered;
}

std::optional<std::vector<AwardBadgesService::BadgeRow>> AwardBadgesService::loadQuarterBadges(
    const drogon::orm::DbClientPtr &dbClient,
    int quarter,
    int userId,
    int courseId,
    ApiError &error) const
{
    const auto badges = loadYearBadges(dbClient, userId, courseId, error);
    if (!badges.has_value())
    {
        return std::nullopt;
    }
    return filterByQuarter(*badges, quarter, error);
}

std::optional<AwardBadgesCountResult> AwardBadgesService::computeCountQuarter(
    const drogon::orm::DbClientPtr &dbClient,
    int quarter,
    int userId,
    int courseId,
    ApiError &error) const
{
    const auto badges = loadQuarterBadges(dbClient, quarter, userId, courseId, error);
    if (!badges.has_value())
    {
        return std::nullopt;
    }

    return AwardBadgesCountResult{.totalCount = static_cast<int>(badges->size())};
}

std::optional<AwardBadgesCountResult> AwardBadgesService::computeCountYear(
    const drogon::orm::DbClientPtr &dbClient,
    int userId,
    int courseId,
    ApiError &error) const
{
    const auto badges = loadYearBadges(dbClient, userId, courseId, error);
    if (!badges.has_value())
    {
        return std::nullopt;
    }

    return AwardBadgesCountResult{.totalCount = static_cast<int>(badges->size())};
}

std::optional<AwardBadgesLevelsResult> AwardBadgesService::computeLevelsQuarter(
    const drogon::orm::DbClientPtr &dbClient,
    int quarter,
    int userId,
    int courseId,
    ApiError &error) const
{
    const auto badges = loadQuarterBadges(dbClient, quarter, userId, courseId, error);
    if (!badges.has_value())
    {
        return std::nullopt;
    }

    AwardBadgesLevelsResult result;
    result.totalCount = static_cast<int>(badges->size());
    for (const auto &badge : *badges)
    {
        if (badge.special)
        {
            ++result.specialCount;
        }
        switch (badge.level)
        {
            case 1:
                ++result.level1Count;
                break;
            case 2:
                ++result.level2Count;
                break;
            case 3:
                ++result.level3Count;
                break;
            case 4:
                ++result.level4Count;
                break;
            case 5:
                ++result.level5Count;
                break;
        }
    }
    return result;
}

std::optional<AwardBadgesLevelsResult> AwardBadgesService::computeLevelsYear(
    const drogon::orm::DbClientPtr &dbClient,
    int userId,
    int courseId,
    ApiError &error) const
{
    const auto badges = loadYearBadges(dbClient, userId, courseId, error);
    if (!badges.has_value())
    {
        return std::nullopt;
    }

    AwardBadgesLevelsResult result;
    result.totalCount = static_cast<int>(badges->size());
    for (const auto &badge : *badges)
    {
        if (badge.special)
        {
            ++result.specialCount;
        }
        switch (badge.level)
        {
            case 1:
                ++result.level1Count;
                break;
            case 2:
                ++result.level2Count;
                break;
            case 3:
                ++result.level3Count;
                break;
            case 4:
                ++result.level4Count;
                break;
            case 5:
                ++result.level5Count;
                break;
        }
    }
    return result;
}

std::optional<RarestAwardBadgesResult> AwardBadgesService::computeRarestYear(
    const drogon::orm::DbClientPtr &dbClient,
    int userId,
    int courseId,
    ApiError &error) const
{
    CourseMetricsService courseService;
    const auto course = courseService.findUserCourse(dbClient, userId, courseId, error);
    if (!course.has_value())
    {
        return std::nullopt;
    }

    const auto badges = loadYearBadges(dbClient, userId, courseId, error);
    if (!badges.has_value())
    {
        return std::nullopt;
    }

    std::set<int> uniqueBadgeIds;
    std::vector<BadgeRow> uniqueBadges;
    uniqueBadges.reserve(badges->size());
    for (const auto &badge : *badges)
    {
        if (uniqueBadgeIds.insert(badge.badgeId).second)
        {
            uniqueBadges.push_back(badge);
        }
    }

    try
    {
        const auto courseUsersResult = dbClient->execSqlSync(
            "select count(distinct user_id) as total_users_count "
            "from public.user_courses "
            "where course_id = $1",
            courseId);

        const int totalUsersCount =
            courseUsersResult.empty() ? 0 : courseUsersResult[0]["total_users_count"].as<int>();
        if (totalUsersCount <= 0)
        {
            error = {drogon::k404NotFound, "Для курса не найдены пользователи"};
            return std::nullopt;
        }

        RarestAwardBadgesResult result;
        result.items.reserve(uniqueBadges.size());

        for (const auto &badge : uniqueBadges)
        {
            const auto ownersResult = dbClient->execSqlSync(
                "select count(distinct uab.user_id) as owners_count "
                "from public.user_award_badges uab "
                "join public.user_courses uc on uc.user_id = uab.user_id "
                "where uc.course_id = $1 and uab.award_badge_id = $2",
                courseId,
                badge.badgeId);

            const int ownersCount = ownersResult.empty() ? 0 : ownersResult[0]["owners_count"].as<int>();
            const double ownersPercentage =
                static_cast<double>(ownersCount) * 100.0 / static_cast<double>(totalUsersCount);

            result.items.push_back(RareAwardBadgeItem{
                .badgeId = badge.badgeId,
                .name = badge.name,
                .title = badge.title,
                .level = badge.level,
                .special = badge.special,
                .imageUrl = badge.imageUrl,
                .shareImageUrl = badge.shareImageUrl,
                .ownersCount = ownersCount,
                .totalUsersCount = totalUsersCount,
                .ownersPercentage = ownersPercentage});
        }

        std::sort(result.items.begin(),
                  result.items.end(),
                  [](const RareAwardBadgeItem &left, const RareAwardBadgeItem &right) {
                      if (left.ownersPercentage != right.ownersPercentage)
                      {
                          return left.ownersPercentage < right.ownersPercentage;
                      }
                      if (left.ownersCount != right.ownersCount)
                      {
                          return left.ownersCount < right.ownersCount;
                      }
                      return left.badgeId < right.badgeId;
                  });

        constexpr size_t kTopBadgesLimit = 3;
        if (result.items.size() > kTopBadgesLimit)
        {
            result.items.resize(kTopBadgesLimit);
        }

        return result;
    }
    catch (const std::exception &e)
    {
        error = {drogon::k500InternalServerError,
                 "Ошибка расчёта редкости достижений: " + std::string(e.what())};
        return std::nullopt;
    }
}
}  // namespace yearreporter::services
