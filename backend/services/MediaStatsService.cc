#include "MediaStatsService.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <unordered_set>

#include "constants/progress_constants.h"

namespace yearreporter::services
{
using Type = MediaStatsService::MediaResource::Type;

namespace
{
int roundToInt(double value)
{
    return static_cast<int>(std::lround(value));
}
}  // namespace

std::optional<std::vector<MediaStatsService::MediaResource>> MediaStatsService::loadWatchedResources(
    const drogon::orm::DbClientPtr &dbClient,
    int userId,
    int usersCourseId,
    ApiError &error) const
{
    try
    {
        const auto result = dbClient->execSqlSync(
            "with user_scope as ("
            "    select lesson_id, group_id "
            "    from public.user_lessons "
            "    where users_course_id = $1"
            "), scoped_media as ("
            "    select distinct "
            "        case "
            "            when m.resource_type = 'Lesson' then us.lesson_id "
            "            else us.lesson_id "
            "        end as lesson_id, "
            "        case "
            "            when m.kind = 'ulms_live' then 'live' "
            "            when m.kind = 'ulms_vod' then 'recorded' "
            "            else 'prerecorded' "
            "        end as media_type "
            "    from public.media_view_sessions m "
            "    join user_scope us on ("
            "        m.resource_type = 'Lesson' "
            "        and replace(m.resource_id, ',', '')::int = us.lesson_id"
            "    ) or ("
            "        m.resource_type = 'Group' "
            "        and replace(m.resource_id, ',', '')::int = us.group_id"
            "    ) "
            "    where replace(m.viewer_id, ',', '')::int = $2 "
            "      and m.kind in ('ulms_live', 'ulms_vod', 'kinescope')"
            ") "
            "select lesson_id, media_type "
            "from scoped_media",
            usersCourseId,
            userId);

        std::vector<MediaResource> resources;
        resources.reserve(result.size());
        for (const auto &row : result)
        {
            MediaResource resource;
            resource.lessonId = row["lesson_id"].as<int>();
            const auto type = row["media_type"].as<std::string>();
            if (type == "live")
            {
                resource.type = Type::Live;
            }
            else if (type == "recorded")
            {
                resource.type = Type::Recorded;
            }
            else
            {
                resource.type = Type::Prerecorded;
            }
            resources.push_back(resource);
        }
        return resources;
    }
    catch (const std::exception &e)
    {
        error = {drogon::k500InternalServerError,
                 "Ошибка загрузки медиа-статистики: " + std::string(e.what())};
        return std::nullopt;
    }
}

std::vector<int> MediaStatsService::computePercentages(int total,
                                                       int live,
                                                       int recorded,
                                                       int prerecorded)
{
    if (total <= 0)
    {
        return {0, 0, 0};
    }

    struct Bucket
    {
        int index;
        int floorValue;
        double fraction;
    };

    const std::vector<double> raw = {
        (static_cast<double>(live) * 100.0) / static_cast<double>(total),
        (static_cast<double>(recorded) * 100.0) / static_cast<double>(total),
        (static_cast<double>(prerecorded) * 100.0) / static_cast<double>(total)};

    std::vector<int> result(3, 0);
    std::vector<Bucket> buckets;
    int used = 0;
    for (int i = 0; i < 3; ++i)
    {
        const auto floored = static_cast<int>(std::floor(raw[i]));
        result[i] = floored;
        used += floored;
        buckets.push_back(Bucket{i, floored, raw[i] - floored});
    }

    std::sort(buckets.begin(), buckets.end(), [](const Bucket &lhs, const Bucket &rhs) {
        if (lhs.fraction == rhs.fraction)
        {
            return lhs.index < rhs.index;
        }
        return lhs.fraction > rhs.fraction;
    });

    for (int i = 0; i < 100 - used; ++i)
    {
        ++result[buckets[i % 3].index];
    }

    return result;
}

MediaStats MediaStatsService::buildStats(const std::vector<MediaResource> &resources)
{
    int live = 0;
    int recorded = 0;
    int prerecorded = 0;

    for (const auto &resource : resources)
    {
        switch (resource.type)
        {
            case Type::Live:
                ++live;
                break;
            case Type::Recorded:
                ++recorded;
                break;
            case Type::Prerecorded:
                ++prerecorded;
                break;
        }
    }

    const auto total = live + recorded + prerecorded;
    const auto percentages = computePercentages(total, live, recorded, prerecorded);

    MediaStats stats;
    stats.liveLesson = MediaTypeMetrics{live, percentages[0]};
    stats.recordedLesson = MediaTypeMetrics{recorded, percentages[1]};
    stats.prerecordedLesson = MediaTypeMetrics{prerecorded, percentages[2]};
    stats.totalWatchedVideosCount = total;
    return stats;
}

std::optional<MediaStats> MediaStatsService::computeQuarter(const drogon::orm::DbClientPtr &dbClient,
                                                            int quarter,
                                                            int userId,
                                                            int courseId,
                                                            ApiError &error) const
{
    if (quarter < 1 || quarter > yearreporter::constants::progress::kQuarterCount)
    {
        error = {drogon::k400BadRequest,
                 "Параметр 'quarter' должен быть числом от 1 до 4"};
        return std::nullopt;
    }

    CourseMetricsService courseService;
    const auto course = courseService.findUserCourse(dbClient, userId, courseId, error);
    if (!course.has_value())
    {
        return std::nullopt;
    }
    const auto trackId = courseService.findMatchingLessonTrack(dbClient, *course, error);
    if (!trackId.has_value())
    {
        return std::nullopt;
    }
    const auto lessons = courseService.loadCourseLessons(dbClient, courseId, error);
    if (!lessons.has_value())
    {
        return std::nullopt;
    }
    const auto watchedResources = loadWatchedResources(dbClient, userId, *trackId, error);
    if (!watchedResources.has_value())
    {
        return std::nullopt;
    }

    const auto slice = CourseMetricsService::quarterSliceFor(*lessons, quarter);
    std::unordered_set<int> quarterLessonIds;
    for (size_t index = slice.begin; index < slice.end; ++index)
    {
        quarterLessonIds.insert((*lessons)[index].lessonId);
    }

    std::vector<MediaResource> quarterResources;
    for (const auto &resource : *watchedResources)
    {
        if (quarterLessonIds.contains(resource.lessonId))
        {
            quarterResources.push_back(resource);
        }
    }

    return buildStats(quarterResources);
}

std::optional<MediaStats> MediaStatsService::computeYear(const drogon::orm::DbClientPtr &dbClient,
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
    const auto trackId = courseService.findMatchingLessonTrack(dbClient, *course, error);
    if (!trackId.has_value())
    {
        return std::nullopt;
    }
    const auto watchedResources = loadWatchedResources(dbClient, userId, *trackId, error);
    if (!watchedResources.has_value())
    {
        return std::nullopt;
    }

    return buildStats(*watchedResources);
}
}  // namespace yearreporter::services
