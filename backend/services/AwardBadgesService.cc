#include "AwardBadgesService.h"

#include <chrono>
#include <ctime>
#include <iomanip>
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
            "       coalesce(ab.level, 0) as level, "
            "       coalesce(ab.special, false) as special, "
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
                .level = row["level"].as<int>(),
                .special = row["special"].as<bool>(),
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
}  // namespace yearreporter::services
