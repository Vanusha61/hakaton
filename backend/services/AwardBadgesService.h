#pragma once

#include <optional>
#include <vector>

#include <drogon/orm/DbClient.h>

#include "CourseMetricsService.h"

namespace yearreporter::services
{
struct AwardBadgesCountResult
{
    int totalCount{0};
};

struct AwardBadgesLevelsResult
{
    int specialCount{0};
    int totalCount{0};
    int level1Count{0};
    int level2Count{0};
    int level3Count{0};
    int level4Count{0};
    int level5Count{0};
};

struct RareAwardBadgeItem
{
    int badgeId{0};
    std::string name;
    std::string title;
    int level{0};
    bool special{false};
    std::string imageUrl;
    std::string shareImageUrl;
    int ownersCount{0};
    int totalUsersCount{0};
    double ownersPercentage{0.0};
};

struct RarestAwardBadgesResult
{
    std::vector<RareAwardBadgeItem> items;
};

class AwardBadgesService
{
  public:
    std::optional<AwardBadgesCountResult> computeCountQuarter(const drogon::orm::DbClientPtr &dbClient,
                                                              int quarter,
                                                              int userId,
                                                              int courseId,
                                                              ApiError &error) const;

    std::optional<AwardBadgesCountResult> computeCountYear(const drogon::orm::DbClientPtr &dbClient,
                                                           int userId,
                                                           int courseId,
                                                           ApiError &error) const;

    std::optional<AwardBadgesLevelsResult> computeLevelsQuarter(
        const drogon::orm::DbClientPtr &dbClient,
        int quarter,
        int userId,
        int courseId,
        ApiError &error) const;

    std::optional<AwardBadgesLevelsResult> computeLevelsYear(const drogon::orm::DbClientPtr &dbClient,
                                                             int userId,
                                                             int courseId,
                                                             ApiError &error) const;

    std::optional<RarestAwardBadgesResult> computeRarestYear(const drogon::orm::DbClientPtr &dbClient,
                                                             int userId,
                                                             int courseId,
                                                             ApiError &error) const;

  private:
    struct BadgeRow
    {
        int badgeId{0};
        std::string name;
        std::string title;
        int level{0};
        bool special{false};
        std::string imageUrl;
        std::string shareImageUrl;
        std::string createdAt;
    };

    std::optional<std::vector<BadgeRow>> loadYearBadges(const drogon::orm::DbClientPtr &dbClient,
                                                        int userId,
                                                        int courseId,
                                                        ApiError &error) const;

    std::optional<std::vector<BadgeRow>> loadQuarterBadges(const drogon::orm::DbClientPtr &dbClient,
                                                           int quarter,
                                                           int userId,
                                                           int courseId,
                                                           ApiError &error) const;

    static std::optional<std::vector<BadgeRow>> filterByQuarter(const std::vector<BadgeRow> &badges,
                                                                int quarter,
                                                                ApiError &error);
    static std::optional<std::chrono::system_clock::time_point> parseIsoTimestamp(const std::string &value);
};
}  // namespace yearreporter::services
