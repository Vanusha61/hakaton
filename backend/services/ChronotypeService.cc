#include "ChronotypeService.h"

#include <algorithm>

#include "constants/progress_constants.h"

namespace yearreporter::services
{
namespace
{
const ChronotypeService::ChronotypeDefinition kLarkDefinition{
    .peakTime = "05:00 - 10:00",
    .type = "Жаворонок",
    .reportName = "Первый луч",
    .insight = "Твоя ракета стартует раньше всех! Ты заряжаешься энергией солнца и решаешь самые сложные задачи, пока остальные еще спят."};

const ChronotypeService::ChronotypeDefinition kDayDefinition{
    .peakTime = "11:00 - 17:00",
    .type = "Дневной",
    .reportName = "Солнечный странник",
    .insight = "Твой полет проходит при максимальном освещении. Ты предпочитаешь стабильный ритм и ясную видимость своих маневров."};

const ChronotypeService::ChronotypeDefinition kEveningDefinition{
    .peakTime = "18:00 - 22:00",
    .type = "Вечерний",
    .reportName = "Вечерняя звезда",
    .insight = "Твои двигатели работают на полную в конце дня. Спокойная вечерняя атмосфера помогает тебе лучше концентрироваться."};

const ChronotypeService::ChronotypeDefinition kNightDefinition{
    .peakTime = "23:00 - 04:00",
    .type = "Сова",
    .reportName = "Ночной охотник",
    .insight = "Ты настоящий мастер ночных перелетов. Твоя ракета светит ярче всех в полной темноте, когда вокруг царит тишина."};

const ChronotypeService::ChronotypeDefinition kUnknownDefinition{
    .peakTime = "",
    .type = "Не определён",
    .reportName = "Космический хронотип не определён",
    .insight = "Недостаточно действий в логах, чтобы уверенно определить время пиковой активности."};
}  // namespace

std::optional<ChronotypeService::CourseTimeContext> ChronotypeService::loadCourseContext(
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

    try
    {
        const auto result = dbClient->execSqlSync(
            "select "
            "    coalesce(nullif(s.timezone, ''), 'Europe/Moscow') as timezone, "
            "    uc.created_at as started_at, "
            "    coalesce(nullif(uc.access_finished_at, ''), nullif(uc.updated_at, ''), uc.created_at) as finished_at "
            "from public.user_courses uc "
            "left join public.students_of_interest s on s.id = uc.user_id "
            "where uc.user_id = $1 and uc.course_id = $2 "
            "limit 1",
            userId,
            courseId);

        if (result.empty())
        {
            error = {drogon::k404NotFound, "Курс пользователя не найден"};
            return std::nullopt;
        }

        CourseTimeContext context;
        context.userId = userId;
        context.courseId = courseId;
        context.usersCourseId = *usersCourseId;
        context.timezone = result[0]["timezone"].as<std::string>();
        context.startedAt = result[0]["started_at"].as<std::string>();
        context.finishedAt = result[0]["finished_at"].as<std::string>();
        return context;
    }
    catch (const std::exception &e)
    {
        error = {drogon::k500InternalServerError,
                 "Ошибка загрузки контекста хронотипа: " + std::string(e.what())};
        return std::nullopt;
    }
}

std::optional<std::vector<ChronotypeService::HourBucket>> ChronotypeService::loadYearBuckets(
    const drogon::orm::DbClientPtr &dbClient,
    const CourseTimeContext &context,
    ApiError &error) const
{
    try
    {
        const auto result = dbClient->execSqlSync(
            "select "
            "    extract(hour from timezone($3, a.created_at at time zone 'UTC'))::int as local_hour, "
            "    count(*)::int as actions_count "
            "from public.wk_users_courses_actions a "
            "where a.user_id = $1 "
            "  and a.users_course_id = $2 "
            "group by local_hour "
            "order by actions_count desc, local_hour asc",
            context.userId,
            context.usersCourseId,
            context.timezone);

        std::vector<HourBucket> buckets;
        buckets.reserve(result.size());
        for (const auto &row : result)
        {
            buckets.push_back(HourBucket{.localHour = row["local_hour"].as<int>(),
                                         .actionsCount = row["actions_count"].as<int>()});
        }
        return buckets;
    }
    catch (const std::exception &e)
    {
        error = {drogon::k500InternalServerError,
                 "Ошибка загрузки активности по году: " + std::string(e.what())};
        return std::nullopt;
    }
}

std::optional<std::vector<ChronotypeService::HourBucket>> ChronotypeService::loadQuarterBuckets(
    const drogon::orm::DbClientPtr &dbClient,
    const CourseTimeContext &context,
    int quarter,
    ApiError &error) const
{
    if (quarter < 1 || quarter > yearreporter::constants::progress::kQuarterCount)
    {
        error = {drogon::k400BadRequest,
                 "Параметр 'quarter' должен быть числом от 1 до 4"};
        return std::nullopt;
    }

    try
    {
        const auto result = dbClient->execSqlSync(
            "with bounds as ("
            "    select "
            "        $4::timestamptz as started_at, "
            "        $5::timestamptz as finished_at"
            "), quarter_bounds as ("
            "    select "
            "        case $3::int "
            "            when 1 then started_at "
            "            when 2 then started_at + (finished_at - started_at) / 4 "
            "            when 3 then started_at + (finished_at - started_at) / 2 "
            "            else started_at + ((finished_at - started_at) * 3) / 4 "
            "        end as quarter_start, "
            "        case $3::int "
            "            when 1 then started_at + (finished_at - started_at) / 4 "
            "            when 2 then started_at + (finished_at - started_at) / 2 "
            "            when 3 then started_at + ((finished_at - started_at) * 3) / 4 "
            "            else finished_at "
            "        end as quarter_end "
            "    from bounds"
            ") "
            "select "
            "    extract(hour from timezone($6, a.created_at at time zone 'UTC'))::int as local_hour, "
            "    count(*)::int as actions_count "
            "from public.wk_users_courses_actions a "
            "cross join quarter_bounds qb "
            "where a.user_id = $1 "
            "  and a.users_course_id = $2 "
            "  and (a.created_at at time zone 'UTC') >= qb.quarter_start "
            "  and (( $3 < 4 and (a.created_at at time zone 'UTC') < qb.quarter_end ) "
            "       or ( $3::int = 4 and (a.created_at at time zone 'UTC') <= qb.quarter_end )) "
            "group by local_hour "
            "order by actions_count desc, local_hour asc",
            context.userId,
            context.usersCourseId,
            quarter,
            context.startedAt,
            context.finishedAt,
            context.timezone);

        std::vector<HourBucket> buckets;
        buckets.reserve(result.size());
        for (const auto &row : result)
        {
            buckets.push_back(HourBucket{.localHour = row["local_hour"].as<int>(),
                                         .actionsCount = row["actions_count"].as<int>()});
        }
        return buckets;
    }
    catch (const std::exception &e)
    {
        error = {drogon::k500InternalServerError,
                 "Ошибка загрузки активности по четверти: " + std::string(e.what())};
        return std::nullopt;
    }
}

ChronotypeService::ChronotypeDefinition ChronotypeService::definitionForHour(int localHour)
{
    if (localHour >= 5 && localHour <= 10)
    {
        return kLarkDefinition;
    }
    if (localHour >= 11 && localHour <= 17)
    {
        return kDayDefinition;
    }
    if (localHour >= 18 && localHour <= 22)
    {
        return kEveningDefinition;
    }
    return kNightDefinition;
}

ChronotypeResult ChronotypeService::buildResult(const std::vector<HourBucket> &buckets,
                                                const std::string &timezone)
{
    ChronotypeResult result;
    result.timezone = timezone;

    for (const auto &bucket : buckets)
    {
        result.totalActionsCount += bucket.actionsCount;
    }

    if (buckets.empty())
    {
        result.peakLocalHour = -1;
        result.peakTime = kUnknownDefinition.peakTime;
        result.type = kUnknownDefinition.type;
        result.reportName = kUnknownDefinition.reportName;
        result.insight = kUnknownDefinition.insight;
        return result;
    }

    const auto peak = std::max_element(
        buckets.begin(),
        buckets.end(),
        [](const HourBucket &lhs, const HourBucket &rhs)
        {
            if (lhs.actionsCount == rhs.actionsCount)
            {
                return lhs.localHour > rhs.localHour;
            }
            return lhs.actionsCount < rhs.actionsCount;
        });

    const auto definition = definitionForHour(peak->localHour);
    result.peakLocalHour = peak->localHour;
    result.peakTime = definition.peakTime;
    result.type = definition.type;
    result.reportName = definition.reportName;
    result.insight = definition.insight;
    result.peakActionsCount = peak->actionsCount;
    return result;
}

std::optional<ChronotypeResult> ChronotypeService::computeQuarter(const drogon::orm::DbClientPtr &dbClient,
                                                                  int quarter,
                                                                  int userId,
                                                                  int courseId,
                                                                  ApiError &error) const
{
    const auto context = loadCourseContext(dbClient, userId, courseId, error);
    if (!context.has_value())
    {
        return std::nullopt;
    }

    const auto buckets = loadQuarterBuckets(dbClient, *context, quarter, error);
    if (!buckets.has_value())
    {
        return std::nullopt;
    }

    return buildResult(*buckets, context->timezone);
}

std::optional<ChronotypeResult> ChronotypeService::computeYear(const drogon::orm::DbClientPtr &dbClient,
                                                               int userId,
                                                               int courseId,
                                                               ApiError &error) const
{
    const auto context = loadCourseContext(dbClient, userId, courseId, error);
    if (!context.has_value())
    {
        return std::nullopt;
    }

    const auto buckets = loadYearBuckets(dbClient, *context, error);
    if (!buckets.has_value())
    {
        return std::nullopt;
    }

    return buildResult(*buckets, context->timezone);
}
}  // namespace yearreporter::services
