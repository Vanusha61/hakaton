#include "PartialCreditShareService.h"

#include <cmath>

namespace yearreporter::services
{
double PartialCreditShareService::roundToOneDecimal(double value)
{
    return std::round(value * 10.0) / 10.0;
}

std::string PartialCreditShareService::buildInsight(double partialPointsPercentage)
{
    if (partialPointsPercentage >= 20.0)
    {
        return "Ты не из тех, кто оставляет баллы на столе. Большая доля частичных баллов "
               "показывает, что ты умеешь выжимать максимум даже из сложных задач и не "
               "теряешь очки там, где можно забрать хотя бы часть результата.";
    }

    if (partialPointsPercentage >= 10.0)
    {
        return "Ты хорошо работаешь с частично решёнными задачами и умеешь сохранять баллы "
               "даже в неоднозначных ситуациях. Это признак аккуратности и умения доводить "
               "решение до полезного промежуточного результата.";
    }

    return "Основная часть твоих баллов приходит из полностью решённых задач. Это тоже сильный "
           "сигнал, а если ещё чаще забирать частичные баллы в пограничных случаях, итоговый "
           "результат сможет стать ещё выше.";
}

std::optional<PartialCreditShareResult> PartialCreditShareService::computeYear(
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
            "select "
            "    count(*) filter (where coalesce(points, 0) > 0 and coalesce(solved, false) = false) "
            "        as partial_answers_count, "
            "    coalesce(sum(case "
            "        when coalesce(points, 0) > 0 and coalesce(solved, false) = false "
            "        then points else 0 end), 0) as partial_points, "
            "    coalesce(sum(coalesce(points, 0)), 0) as total_points "
            "from public.user_answers "
            "where user_id = $1 "
            "  and resource_type = 'Lesson'",
            userId);

        if (result.empty())
        {
            error = {drogon::k404NotFound, "Для пользователя не найдены ответы по задачам"};
            return std::nullopt;
        }

        PartialCreditShareResult metrics;
        metrics.reportName = "Копейка рубль бережет";
        metrics.partialAnswersCount = result[0]["partial_answers_count"].as<int>();
        metrics.partialPoints = result[0]["partial_points"].as<double>();
        metrics.totalPoints = result[0]["total_points"].as<double>();

        if (metrics.totalPoints <= 0.0)
        {
            error = {drogon::k404NotFound, "Для пользователя не найдены баллы по задачам"};
            return std::nullopt;
        }

        metrics.partialPoints = roundToOneDecimal(metrics.partialPoints);
        metrics.totalPoints = roundToOneDecimal(metrics.totalPoints);
        metrics.partialPointsPercentage =
            roundToOneDecimal(metrics.partialPoints * 100.0 / metrics.totalPoints);
        metrics.insight = buildInsight(metrics.partialPointsPercentage);
        return metrics;
    }
    catch (const std::exception &e)
    {
        error = {drogon::k500InternalServerError,
                 "Ошибка расчёта доли частичных баллов: " + std::string(e.what())};
        return std::nullopt;
    }
}
}  // namespace yearreporter::services
