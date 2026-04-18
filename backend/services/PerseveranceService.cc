#include "PerseveranceService.h"

#include <algorithm>
#include <cmath>
#include <sstream>

#include <json/json.h>

namespace yearreporter::services
{
namespace
{
double sumAttemptPoints(const Json::Value &attemptNode)
{
    if (!attemptNode.isObject())
    {
        return 0.0;
    }

    double total = 0.0;
    for (const auto &memberName : attemptNode.getMemberNames())
    {
        const auto &taskPart = attemptNode[memberName];
        if (taskPart.isObject() && taskPart["points"].isNumeric())
        {
            total += taskPart["points"].asDouble();
        }
    }
    return total;
}
}  // namespace

double PerseveranceService::roundToOneDecimal(double value)
{
    return std::round(value * 10.0) / 10.0;
}

double PerseveranceService::extractFirstAttemptPoints(const std::string &resultsJson, ApiError &error)
{
    if (resultsJson.empty())
    {
        return 0.0;
    }

    Json::CharReaderBuilder builder;
    builder["collectComments"] = false;
    Json::Value root;
    std::string parseErrors;
    std::istringstream stream(resultsJson);
    if (!Json::parseFromStream(builder, stream, &root, &parseErrors))
    {
        error = {drogon::k500InternalServerError,
                 "Не удалось разобрать JSON в поле results: " + parseErrors};
        return 0.0;
    }

    if (!root.isArray() || root.empty())
    {
        return 0.0;
    }

    return sumAttemptPoints(root[0]);
}

std::string PerseveranceService::buildInsight(double extraPointsFromRetries)
{
    if (extraPointsFromRetries >= 10.0)
    {
        return "В космосе, как и в учебе, не все получается с первого раза. Твой результат "
               "доказывает: ты умеешь проводить работу над ошибками и доводить начатое до "
               "конца. С таким уровнем упорства тебе по плечу любые перегрузки!";
    }

    if (extraPointsFromRetries > 0.0)
    {
        return "Ты не бросаешь задачу после первой ошибки и умеешь выжимать из повторных "
               "попыток дополнительный результат. Это хороший навык для длинных космических "
               "дистанций и сложных тем.";
    }

    return "Даже если повторные попытки пока не принесли много дополнительных баллов, сам "
           "подход уже правильный: ты возвращаешься к задаче и продолжаешь искать решение. "
           "Именно так и набирается настоящая учебная инерция.";
}

std::optional<PerseveranceResult> PerseveranceService::computeYear(
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
            "select points, results "
            "from public.user_answers "
            "where user_id = $1 "
            "  and attempts > 1 "
            "  and resource_type = 'Lesson'",
            userId);

        if (result.empty())
        {
            error = {drogon::k404NotFound,
                     "Для пользователя не найдены задачи с повторными попытками"};
            return std::nullopt;
        }

        PerseveranceResult metrics;
        metrics.reportName = "Профит от упорства";

        for (const auto &row : result)
        {
            const auto firstAttemptPoints = extractFirstAttemptPoints(
                row["results"].isNull() ? std::string() : row["results"].as<std::string>(),
                error);
            if (!error.reason.empty())
            {
                return std::nullopt;
            }

            const auto finalPoints = row["points"].isNull() ? 0.0 : row["points"].as<double>();

            metrics.firstAttemptPoints += firstAttemptPoints;
            metrics.finalPoints += finalPoints;
            metrics.extraPointsFromRetries += std::max(0.0, finalPoints - firstAttemptPoints);
            ++metrics.repeatedTasksCount;
        }

        metrics.firstAttemptPoints = roundToOneDecimal(metrics.firstAttemptPoints);
        metrics.finalPoints = roundToOneDecimal(metrics.finalPoints);
        metrics.extraPointsFromRetries = roundToOneDecimal(metrics.extraPointsFromRetries);
        metrics.insight = buildInsight(metrics.extraPointsFromRetries);
        return metrics;
    }
    catch (const std::exception &e)
    {
        error = {drogon::k500InternalServerError,
                 "Ошибка расчёта профита от повторных попыток: " + std::string(e.what())};
        return std::nullopt;
    }
}
}  // namespace yearreporter::services
