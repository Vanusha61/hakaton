#include "VideoStoryService.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <unordered_set>

#include "constants/progress_constants.h"

namespace yearreporter::services
{
namespace
{
const VideoStoryService::AttentionDefinition kQuickStart{
    .role = "Быстрый старт",
    .reportName = "Пик в начале",
    .insight = "Ты всегда тщательно проверяешь системы перед взлетом. Вводные инструкции — твой фундамент!"};

const VideoStoryService::AttentionDefinition kMiddlePeak{
    .role = "Исследователь сути",
    .reportName = "Пик в середине",
    .insight = "Ты не боишься нырять в самую гущу событий. Твой фокус на ключевых темах урока делает твою ракету неуязвимой для сложных задач."};

const VideoStoryService::AttentionDefinition kFinalPeak{
    .role = "Финалист",
    .reportName = "Пик в конце",
    .insight = "Ты из тех, кто всегда доводит маневр до идеала. Итоговые выводы для тебя важнее всего!"};

const VideoStoryService::AttentionDefinition kDetailedPeak{
    .role = "Деталист",
    .reportName = "Множество мелких пиков",
    .insight = "Твой глаз — как радар! Ты замечаешь мельчайшие детали в каждом отсеке урока."};
}  // namespace

std::optional<VideoStoryService::Context> VideoStoryService::loadContext(
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

    const auto usersCourseId = courseService.findMatchingLessonTrack(dbClient, *course, error);
    if (!usersCourseId.has_value())
    {
        return std::nullopt;
    }

    const auto lessons = courseService.loadCourseLessons(dbClient, courseId, error);
    if (!lessons.has_value())
    {
        return std::nullopt;
    }

    return Context{
        .userId = userId,
        .courseId = courseId,
        .usersCourseId = *usersCourseId,
        .lessons = *lessons};
}

std::optional<std::vector<VideoStoryService::MediaSession>> VideoStoryService::loadSessions(
    const drogon::orm::DbClientPtr &dbClient,
    int courseId,
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
            "), scoped_sessions as ("
            "    select distinct "
            "        replace(m.viewer_id, ',', '')::int as viewer_id, "
            "        us.lesson_id as lesson_id, "
            "        to_char(to_timestamp(m.started_at, 'FMDD Mon, YYYY, HH24:MI'), "
            "                'YYYY-MM-DD HH24:MI:SS') as started_at, "
            "        coalesce(m.segments_total, 0) as segments_total, "
            "        coalesce(m.viewed_segments, '') as viewed_segments "
            "    from public.media_view_sessions m "
            "    join user_scope us on ("
            "        m.resource_type = 'Lesson' "
            "        and replace(m.resource_id, ',', '')::int = us.lesson_id"
            "    ) or ("
            "        m.resource_type = 'Group' "
            "        and replace(m.resource_id, ',', '')::int = us.group_id"
            "    ) "
            "    join public.user_courses uc on uc.user_id = replace(m.viewer_id, ',', '')::int "
            "    where uc.course_id = $2 "
            "      and m.kind in ('ulms_live', 'ulms_vod', 'kinescope')"
            ") "
            "select viewer_id, lesson_id, started_at, segments_total, viewed_segments "
            "from scoped_sessions",
            usersCourseId,
            courseId);

        std::vector<MediaSession> sessions;
        sessions.reserve(result.size());
        for (const auto &row : result)
        {
            sessions.push_back(MediaSession{
                .viewerId = row["viewer_id"].as<int>(),
                .lessonId = row["lesson_id"].as<int>(),
                .startedAt = row["started_at"].as<std::string>(),
                .segmentsTotal = row["segments_total"].as<int>(),
                .viewedSegments = row["viewed_segments"].as<std::string>()});
        }
        return sessions;
    }
    catch (const std::exception &e)
    {
        error = {drogon::k500InternalServerError,
                 "Ошибка загрузки видео-сессий: " + std::string(e.what())};
        return std::nullopt;
    }
}

std::vector<CourseMetricsService::CourseLesson> VideoStoryService::sliceLessons(
    const std::vector<CourseMetricsService::CourseLesson> &lessons,
    int quarter,
    ApiError &error)
{
    if (quarter < 1 || quarter > yearreporter::constants::progress::kQuarterCount)
    {
        error = {drogon::k400BadRequest,
                 "Параметр 'quarter' должен быть числом от 1 до 4"};
        return {};
    }

    const auto slice = CourseMetricsService::quarterSliceFor(lessons, quarter);
    return {lessons.begin() + static_cast<std::ptrdiff_t>(slice.begin),
            lessons.begin() + static_cast<std::ptrdiff_t>(slice.end)};
}

std::vector<VideoStoryService::MediaSession> VideoStoryService::filterSessionsByLessons(
    const std::vector<MediaSession> &sessions,
    const std::vector<CourseMetricsService::CourseLesson> &lessons)
{
    std::unordered_set<int> lessonIds;
    for (const auto &lesson : lessons)
    {
        lessonIds.insert(lesson.lessonId);
    }

    std::vector<MediaSession> filtered;
    for (const auto &session : sessions)
    {
        if (lessonIds.contains(session.lessonId))
        {
            filtered.push_back(session);
        }
    }
    return filtered;
}

std::vector<int> VideoStoryService::parseViewedSegments(const std::string &viewedSegments)
{
    std::string cleaned;
    cleaned.reserve(viewedSegments.size());
    for (const auto ch : viewedSegments)
    {
        if (ch == '[' || ch == ']')
        {
            continue;
        }
        cleaned.push_back(ch);
    }

    std::stringstream stream(cleaned);
    std::vector<int> segments;
    int value = 0;
    while (stream >> value)
    {
        segments.push_back(value);
    }
    return segments;
}

std::string VideoStoryService::normalizeStartedAt(const std::string &startedAt)
{
    return startedAt;
}

bool VideoStoryService::hasMultiplePeaks(const std::unordered_map<int, int> &hitsByPosition)
{
    if (hitsByPosition.empty())
    {
        return false;
    }

    const auto maxHits = std::max_element(
        hitsByPosition.begin(),
        hitsByPosition.end(),
        [](const auto &lhs, const auto &rhs) { return lhs.second < rhs.second; })->second;

    std::vector<int> peaks;
    for (const auto &[position, hits] : hitsByPosition)
    {
        if (static_cast<double>(hits) >= static_cast<double>(maxHits) * 0.8)
        {
            peaks.push_back(position);
        }
    }

    std::sort(peaks.begin(), peaks.end());
    int distinctPeaks = 0;
    int lastPeak = -100;
    for (const auto position : peaks)
    {
        if (position - lastPeak >= 12)
        {
            ++distinctPeaks;
            lastPeak = position;
        }
    }
    return distinctPeaks >= 3;
}

VideoStoryService::AttentionDefinition VideoStoryService::classifyAttentionProfile(
    const std::unordered_map<int, int> &hitsByPosition,
    int peakPositionPercentage)
{
    if (hasMultiplePeaks(hitsByPosition))
    {
        return kDetailedPeak;
    }

    if (peakPositionPercentage <= 15)
    {
        return kQuickStart;
    }
    if (peakPositionPercentage >= 40 && peakPositionPercentage <= 60)
    {
        return kMiddlePeak;
    }
    if (peakPositionPercentage >= 80)
    {
        return kFinalPeak;
    }

    if (peakPositionPercentage < 40)
    {
        return kQuickStart;
    }
    if (peakPositionPercentage < 80)
    {
        return kMiddlePeak;
    }
    return kFinalPeak;
}

VideoAttentionProfileResult VideoStoryService::buildAttentionProfile(
    const std::vector<MediaSession> &sessions)
{
    std::unordered_map<int, int> hitsByPosition;

    for (const auto &session : sessions)
    {
        if (session.segmentsTotal <= 0)
        {
            continue;
        }

        for (const auto segmentIndex : parseViewedSegments(session.viewedSegments))
        {
            const auto position = CourseMetricsService::roundCount(
                (static_cast<double>(segmentIndex) / static_cast<double>(session.segmentsTotal)) * 100.0);
            ++hitsByPosition[position];
        }
    }

    if (hitsByPosition.empty())
    {
        return VideoAttentionProfileResult{
            .peakPositionPercentage = 0,
            .role = kQuickStart.role,
            .reportName = kQuickStart.reportName,
            .insight = kQuickStart.insight};
    }

    const auto peak = std::max_element(
        hitsByPosition.begin(),
        hitsByPosition.end(),
        [](const auto &lhs, const auto &rhs)
        {
            if (lhs.second == rhs.second)
            {
                return lhs.first > rhs.first;
            }
            return lhs.second < rhs.second;
        });

    const auto peakPosition = peak == hitsByPosition.end() ? 0 : peak->first;
    const auto definition = classifyAttentionProfile(hitsByPosition, peakPosition);

    return VideoAttentionProfileResult{
        .peakPositionPercentage = peakPosition,
        .role = definition.role,
        .reportName = definition.reportName,
        .insight = definition.insight};
}

std::optional<FirstVideoStarterResult> VideoStoryService::buildFirstStarterResult(
    const std::vector<MediaSession> &sessions,
    int userId,
    ApiError &error)
{
    std::unordered_map<int, std::string> firstStartByViewer;

    for (const auto &session : sessions)
    {
        const auto startedAt = normalizeStartedAt(session.startedAt);
        const auto current = firstStartByViewer.find(session.viewerId);
        if (current == firstStartByViewer.end() || startedAt < current->second)
        {
            firstStartByViewer[session.viewerId] = startedAt;
        }
    }

    if (!firstStartByViewer.contains(userId))
    {
        error = {drogon::k404NotFound, "Для пользователя не найдено стартов просмотра видео"};
        return std::nullopt;
    }

    auto globalIt = std::min_element(
        firstStartByViewer.begin(),
        firstStartByViewer.end(),
        [](const auto &lhs, const auto &rhs)
        {
            if (lhs.second == rhs.second)
            {
                return lhs.first < rhs.first;
            }
            return lhs.second < rhs.second;
        });

    const auto &userFirst = firstStartByViewer[userId];
    return FirstVideoStarterResult{
        .isUserFirstVideoStarter = userFirst == globalIt->second && userId == globalIt->first,
        .userFirstStartedAt = userFirst,
        .globalFirstStartedAt = globalIt->second};
}

std::optional<FirstVideoStarterResult> VideoStoryService::computeFirstVideoStarterQuarter(
    const drogon::orm::DbClientPtr &dbClient,
    int quarter,
    int userId,
    int courseId,
    ApiError &error) const
{
    const auto context = loadContext(dbClient, userId, courseId, error);
    if (!context.has_value())
    {
        return std::nullopt;
    }

    const auto lessons = sliceLessons(context->lessons, quarter, error);
    if (!error.reason.empty())
    {
        return std::nullopt;
    }

    const auto sessions = loadSessions(dbClient, courseId, context->usersCourseId, error);
    if (!sessions.has_value())
    {
        return std::nullopt;
    }

    return buildFirstStarterResult(filterSessionsByLessons(*sessions, lessons), userId, error);
}

std::optional<FirstVideoStarterResult> VideoStoryService::computeFirstVideoStarterYear(
    const drogon::orm::DbClientPtr &dbClient,
    int userId,
    int courseId,
    ApiError &error) const
{
    const auto context = loadContext(dbClient, userId, courseId, error);
    if (!context.has_value())
    {
        return std::nullopt;
    }

    const auto sessions = loadSessions(dbClient, courseId, context->usersCourseId, error);
    if (!sessions.has_value())
    {
        return std::nullopt;
    }

    return buildFirstStarterResult(*sessions, userId, error);
}

std::optional<VideoAttentionProfileResult> VideoStoryService::computeAttentionProfileQuarter(
    const drogon::orm::DbClientPtr &dbClient,
    int quarter,
    int userId,
    int courseId,
    ApiError &error) const
{
    const auto context = loadContext(dbClient, userId, courseId, error);
    if (!context.has_value())
    {
        return std::nullopt;
    }

    const auto lessons = sliceLessons(context->lessons, quarter, error);
    if (!error.reason.empty())
    {
        return std::nullopt;
    }

    const auto sessions = loadSessions(dbClient, courseId, context->usersCourseId, error);
    if (!sessions.has_value())
    {
        return std::nullopt;
    }

    std::vector<MediaSession> mySessions;
    for (const auto &session : filterSessionsByLessons(*sessions, lessons))
    {
        if (session.viewerId == userId)
        {
            mySessions.push_back(session);
        }
    }

    if (mySessions.empty())
    {
        error = {drogon::k404NotFound, "Для пользователя не найдено просмотренных видео в выбранной четверти"};
        return std::nullopt;
    }

    return buildAttentionProfile(mySessions);
}

std::optional<VideoAttentionProfileResult> VideoStoryService::computeAttentionProfileYear(
    const drogon::orm::DbClientPtr &dbClient,
    int userId,
    int courseId,
    ApiError &error) const
{
    const auto context = loadContext(dbClient, userId, courseId, error);
    if (!context.has_value())
    {
        return std::nullopt;
    }

    const auto sessions = loadSessions(dbClient, courseId, context->usersCourseId, error);
    if (!sessions.has_value())
    {
        return std::nullopt;
    }

    std::vector<MediaSession> mySessions;
    for (const auto &session : *sessions)
    {
        if (session.viewerId == userId)
        {
            mySessions.push_back(session);
        }
    }

    if (mySessions.empty())
    {
        error = {drogon::k404NotFound, "Для пользователя не найдено просмотренных видео за год"};
        return std::nullopt;
    }

    return buildAttentionProfile(mySessions);
}
}  // namespace yearreporter::services
