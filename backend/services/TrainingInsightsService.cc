#include "TrainingInsightsService.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <optional>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

#include "constants/progress_constants.h"

namespace yearreporter::services
{
namespace
{
using Lesson = CourseMetricsService::CourseLesson;
using QuarterSlice = CourseMetricsService::QuarterSlice;

std::optional<std::chrono::system_clock::time_point> parseIsoTimestamp(const std::string &value)
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
}  // namespace

const std::vector<TrainingInsightsService::TrainingDefinition> &TrainingInsightsService::definitions()
{
    static const std::vector<TrainingDefinition> kDefinitions = {
        {.trainingId = 2357,
         .name = "Промежуточный контроль (тестирование)",
         .lessonId = 5591,
         .difficulty = 3,
         .taskTemplatesCount = 5},
        {.trainingId = 2356,
         .name = "Промежуточный контроль по физике в проекте Физтех-кузница",
         .lessonId = std::nullopt,
         .difficulty = 3,
         .taskTemplatesCount = 5},
        {.trainingId = 2355,
         .name = "Как устроена Физтех-кузница",
         .lessonId = std::nullopt,
         .difficulty = 1,
         .taskTemplatesCount = 7},
        {.trainingId = 2358,
         .name = "Промежуточный контроль по математике в проекте Физтех-кузница",
         .lessonId = std::nullopt,
         .difficulty = 3,
         .taskTemplatesCount = 5},
        {.trainingId = 2359,
         .name = "Промежуточный контроль (тестирование)",
         .lessonId = 5677,
         .difficulty = 3,
         .taskTemplatesCount = 5},
        {.trainingId = 2360,
         .name = "Промежуточный контроль по информатике в проекте Физтех-кузница",
         .lessonId = std::nullopt,
         .difficulty = 3,
         .taskTemplatesCount = 5},
        {.trainingId = 2362,
         .name = "Промежуточный контроль (тестирование)",
         .lessonId = 5760,
         .difficulty = 3,
         .taskTemplatesCount = 5}};
    return kDefinitions;
}

std::string TrainingInsightsService::buildTrainingIdList(const std::vector<TrainingDefinition> &trainings)
{
    std::ostringstream out;
    for (size_t index = 0; index < trainings.size(); ++index)
    {
        if (index > 0)
        {
            out << ", ";
        }
        out << trainings[index].trainingId;
    }
    return out.str();
}

std::optional<int> TrainingInsightsService::parseDurationSeconds(const std::string &startedAt,
                                                                 const std::string &finishedAt)
{
    const auto started = parseIsoTimestamp(startedAt);
    const auto finished = parseIsoTimestamp(finishedAt);
    if (!started.has_value() || !finished.has_value())
    {
        return std::nullopt;
    }

    const auto diff = std::chrono::duration_cast<std::chrono::seconds>(*finished - *started).count();
    if (diff < 0)
    {
        return std::nullopt;
    }

    return static_cast<int>(diff);
}

std::string TrainingInsightsService::formatDuration(int totalSeconds)
{
    const auto hours = totalSeconds / 3600;
    const auto minutes = (totalSeconds % 3600) / 60;
    const auto seconds = totalSeconds % 60;

    std::ostringstream out;
    out << std::setfill('0');
    if (hours > 0)
    {
        out << std::setw(2) << hours << ":";
    }
    out << std::setw(2) << minutes << ":" << std::setw(2) << seconds;
    return out.str();
}

std::optional<TrainingInsightsService::ScopedContext>
TrainingInsightsService::loadContextForLessons(const drogon::orm::DbClientPtr &dbClient,
                                               const std::vector<Lesson> &lessons,
                                               int userId,
                                               int courseId,
                                               ApiError &error) const
{
    if (lessons.empty())
    {
        error = {drogon::k404NotFound, "Для выбранного периода не найдены уроки"};
        return std::nullopt;
    }

    CourseMetricsService courseService;
    const auto course = courseService.findUserCourse(dbClient, userId, courseId, error);
    if (!course.has_value())
    {
        return std::nullopt;
    }

    std::unordered_set<int> lessonIds;
    for (const auto &lesson : lessons)
    {
        lessonIds.insert(lesson.lessonId);
    }

    ScopedContext context;
    for (const auto &definition : definitions())
    {
        if (definition.lessonId.has_value() && lessonIds.contains(*definition.lessonId))
        {
            context.trainings.push_back(definition);
        }
    }

    if (context.trainings.empty())
    {
        error = {drogon::k404NotFound, "Для выбранного периода не найдены тренинги, привязанные к урокам"};
        return std::nullopt;
    }

    try
    {
        const auto result = dbClient->execSqlSync(
            "select user_id, "
            "       training_id, "
            "       coalesce(solved_tasks_count, 0) as solved_tasks_count, "
            "       coalesce(submitted_answers_count, 0) as submitted_answers_count, "
            "       coalesce(attempts, 0) as attempts, "
            "       coalesce(started_at, '') as started_at, "
            "       coalesce(finished_at, '') as finished_at "
            "from public.user_trainings "
            "where training_id in (" +
                buildTrainingIdList(context.trainings) +
                ")");

        for (const auto &row : result)
        {
            context.userTrainings.push_back(UserTrainingRecord{
                .userId = row["user_id"].as<int>(),
                .trainingId = row["training_id"].as<int>(),
                .solvedTasksCount = row["solved_tasks_count"].as<int>(),
                .submittedAnswersCount = row["submitted_answers_count"].as<int>(),
                .attempts = row["attempts"].as<int>(),
                .startedAt = row["started_at"].as<std::string>(),
                .finishedAt = row["finished_at"].as<std::string>()});
        }

        const auto hasTargetUserRows = std::any_of(
            context.userTrainings.begin(),
            context.userTrainings.end(),
            [userId](const UserTrainingRecord &record) { return record.userId == userId; });

        if (!hasTargetUserRows)
        {
            error = {drogon::k404NotFound, "Для пользователя не найдены тренинги в выбранном периоде"};
            return std::nullopt;
        }

        return context;
    }
    catch (const std::exception &e)
    {
        error = {drogon::k500InternalServerError,
                 "Ошибка загрузки данных по тренингам: " + std::string(e.what())};
        return std::nullopt;
    }
}

std::optional<TrainingInsightsService::ScopedContext>
TrainingInsightsService::loadQuarterContext(const drogon::orm::DbClientPtr &dbClient,
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
    const auto lessons = courseService.loadCourseLessons(dbClient, courseId, error);
    if (!lessons.has_value())
    {
        return std::nullopt;
    }

    const QuarterSlice slice = CourseMetricsService::quarterSliceFor(*lessons, quarter);
    std::vector<Lesson> selectedLessons(
        lessons->begin() + static_cast<std::ptrdiff_t>(slice.begin),
        lessons->begin() + static_cast<std::ptrdiff_t>(slice.end));

    return loadContextForLessons(dbClient, selectedLessons, userId, courseId, error);
}

std::optional<TrainingInsightsService::ScopedContext>
TrainingInsightsService::loadYearContext(const drogon::orm::DbClientPtr &dbClient,
                                         int userId,
                                         int courseId,
                                         ApiError &error) const
{
    CourseMetricsService courseService;
    const auto lessons = courseService.loadCourseLessons(dbClient, courseId, error);
    if (!lessons.has_value())
    {
        return std::nullopt;
    }

    return loadContextForLessons(dbClient, *lessons, userId, courseId, error);
}

std::optional<TrainingDurationResult> TrainingInsightsService::shortestFromContext(
    const ScopedContext &context,
    int userId,
    ApiError &error)
{
    std::unordered_map<int, TrainingDefinition> trainingById;
    for (const auto &training : context.trainings)
    {
        trainingById.emplace(training.trainingId, training);
    }

    std::optional<TrainingDurationResult> best;
    for (const auto &record : context.userTrainings)
    {
        if (record.userId != userId)
        {
            continue;
        }

        const auto duration = parseDurationSeconds(record.startedAt, record.finishedAt);
        if (!duration.has_value())
        {
            continue;
        }

        const auto trainingIt = trainingById.find(record.trainingId);
        if (trainingIt == trainingById.end())
        {
            continue;
        }

        if (!best.has_value() || *duration < best->durationSeconds)
        {
            best = TrainingDurationResult{
                .trainingId = record.trainingId,
                .trainingName = trainingIt->second.name,
                .durationSeconds = *duration,
                .durationHuman = formatDuration(*duration)};
        }
    }

    if (!best.has_value())
    {
        error = {drogon::k404NotFound, "Не удалось определить длительность тренингов в выбранном периоде"};
        return std::nullopt;
    }

    return best;
}

std::optional<TrainingDurationResult> TrainingInsightsService::longestFromContext(
    const ScopedContext &context,
    int userId,
    ApiError &error)
{
    std::unordered_map<int, TrainingDefinition> trainingById;
    for (const auto &training : context.trainings)
    {
        trainingById.emplace(training.trainingId, training);
    }

    std::optional<TrainingDurationResult> best;
    for (const auto &record : context.userTrainings)
    {
        if (record.userId != userId)
        {
            continue;
        }

        const auto duration = parseDurationSeconds(record.startedAt, record.finishedAt);
        if (!duration.has_value())
        {
            continue;
        }

        const auto trainingIt = trainingById.find(record.trainingId);
        if (trainingIt == trainingById.end())
        {
            continue;
        }

        if (!best.has_value() || *duration > best->durationSeconds)
        {
            best = TrainingDurationResult{
                .trainingId = record.trainingId,
                .trainingName = trainingIt->second.name,
                .durationSeconds = *duration,
                .durationHuman = formatDuration(*duration)};
        }
    }

    if (!best.has_value())
    {
        error = {drogon::k404NotFound, "Не удалось определить длительность тренингов в выбранном периоде"};
        return std::nullopt;
    }

    return best;
}

std::optional<TrainingPerfectStreakResult> TrainingInsightsService::perfectStreakFromContext(
    const ScopedContext &context,
    int userId,
    ApiError &error)
{
    std::unordered_map<int, TrainingDefinition> trainingById;
    for (const auto &training : context.trainings)
    {
        trainingById.emplace(training.trainingId, training);
    }

    std::optional<TrainingPerfectStreakResult> best;
    for (const auto &record : context.userTrainings)
    {
        if (record.userId != userId)
        {
            continue;
        }

        const auto trainingIt = trainingById.find(record.trainingId);
        if (trainingIt == trainingById.end())
        {
            continue;
        }

        const bool solvedWithoutMistakes =
            record.solvedTasksCount > 0 &&
            record.solvedTasksCount == trainingIt->second.taskTemplatesCount &&
            record.submittedAnswersCount == record.solvedTasksCount;

        if (!solvedWithoutMistakes)
        {
            continue;
        }

        if (!best.has_value() || record.solvedTasksCount > best->maxSolvedWithoutMistakes)
        {
            best = TrainingPerfectStreakResult{
                .trainingId = record.trainingId,
                .trainingName = trainingIt->second.name,
                .maxSolvedWithoutMistakes = record.solvedTasksCount,
                .reportName = "Серия без ошибок",
                .insight = "Ты решил " + std::to_string(record.solvedTasksCount) +
                           " задач в тренинге без ошибок. Это отличный показатель концентрации и точности."};
        }
    }

    if (!best.has_value())
    {
        error = {drogon::k404NotFound,
                 "Для пользователя не найдено тренингов, решённых без ошибок в выбранном периоде"};
        return std::nullopt;
    }

    return best;
}

std::optional<TrainingStarterRankResult> TrainingInsightsService::starterRankFromContext(
    const ScopedContext &context,
    int userId,
    ApiError &error)
{
    std::unordered_map<int, TrainingDefinition> trainingById;
    for (const auto &training : context.trainings)
    {
        trainingById.emplace(training.trainingId, training);
    }

    std::unordered_map<int, std::vector<const UserTrainingRecord *>> recordsByTraining;
    for (const auto &record : context.userTrainings)
    {
        if (!record.startedAt.empty())
        {
            recordsByTraining[record.trainingId].push_back(&record);
        }
    }

    std::optional<TrainingStarterRankResult> best;
    for (const auto &[trainingId, rawRecords] : recordsByTraining)
    {
        auto records = rawRecords;
        std::sort(records.begin(),
                  records.end(),
                  [](const UserTrainingRecord *lhs, const UserTrainingRecord *rhs)
                  {
                      if (lhs->startedAt != rhs->startedAt)
                      {
                          return lhs->startedAt < rhs->startedAt;
                      }
                      return lhs->userId < rhs->userId;
                  });

        int rank = 0;
        for (const auto *record : records)
        {
            ++rank;
            if (record->userId != userId)
            {
                continue;
            }

            const auto trainingIt = trainingById.find(trainingId);
            if (trainingIt == trainingById.end())
            {
                continue;
            }

            TrainingStarterRankResult candidate{
                .trainingId = trainingId,
                .trainingName = trainingIt->second.name,
                .userRank = rank,
                .isUserFirstStarter = rank == 1,
                .isUserTop3Starter = rank <= 3,
                .userStartedAt = record->startedAt,
                .globalFirstStartedAt = records.front()->startedAt};

            if (!best.has_value() || candidate.userRank < best->userRank)
            {
                best = candidate;
            }
            break;
        }
    }

    if (!best.has_value())
    {
        error = {drogon::k404NotFound,
                 "Для пользователя не найдено стартов тренингов в выбранном периоде"};
        return std::nullopt;
    }

    return best;
}

TrainingDifficultyStatsResult TrainingInsightsService::difficultyStatsFromContext(const ScopedContext &context,
                                                                                  int userId)
{
    std::unordered_map<int, TrainingDefinition> trainingById;
    for (const auto &training : context.trainings)
    {
        trainingById.emplace(training.trainingId, training);
    }

    TrainingDifficultyStatsResult result;
    for (const auto &record : context.userTrainings)
    {
        if (record.userId != userId)
        {
            continue;
        }

        const auto trainingIt = trainingById.find(record.trainingId);
        if (trainingIt == trainingById.end())
        {
            continue;
        }

        ++result.totalCount;
        if (trainingIt->second.difficulty <= 1)
        {
            ++result.easyCount;
        }
        else if (trainingIt->second.difficulty >= 3)
        {
            ++result.hardCount;
        }
        else
        {
            ++result.mediumCount;
        }
    }

    return result;
}

std::optional<TrainingDurationResult> TrainingInsightsService::computeShortestQuarter(
    const drogon::orm::DbClientPtr &dbClient,
    int quarter,
    int userId,
    int courseId,
    ApiError &error) const
{
    const auto context = loadQuarterContext(dbClient, quarter, userId, courseId, error);
    if (!context.has_value())
    {
        return std::nullopt;
    }
    return shortestFromContext(*context, userId, error);
}

std::optional<TrainingDurationResult> TrainingInsightsService::computeShortestYear(
    const drogon::orm::DbClientPtr &dbClient,
    int userId,
    int courseId,
    ApiError &error) const
{
    const auto context = loadYearContext(dbClient, userId, courseId, error);
    if (!context.has_value())
    {
        return std::nullopt;
    }
    return shortestFromContext(*context, userId, error);
}

std::optional<TrainingDurationResult> TrainingInsightsService::computeLongestQuarter(
    const drogon::orm::DbClientPtr &dbClient,
    int quarter,
    int userId,
    int courseId,
    ApiError &error) const
{
    const auto context = loadQuarterContext(dbClient, quarter, userId, courseId, error);
    if (!context.has_value())
    {
        return std::nullopt;
    }
    return longestFromContext(*context, userId, error);
}

std::optional<TrainingDurationResult> TrainingInsightsService::computeLongestYear(
    const drogon::orm::DbClientPtr &dbClient,
    int userId,
    int courseId,
    ApiError &error) const
{
    const auto context = loadYearContext(dbClient, userId, courseId, error);
    if (!context.has_value())
    {
        return std::nullopt;
    }
    return longestFromContext(*context, userId, error);
}

std::optional<TrainingSolvedTasksResult> TrainingInsightsService::computeSolvedTasksQuarter(
    const drogon::orm::DbClientPtr &dbClient,
    int quarter,
    int userId,
    int courseId,
    ApiError &error) const
{
    const auto context = loadQuarterContext(dbClient, quarter, userId, courseId, error);
    if (!context.has_value())
    {
        return std::nullopt;
    }

    TrainingSolvedTasksResult result;
    for (const auto &record : context->userTrainings)
    {
        if (record.userId != userId)
        {
            continue;
        }
        ++result.trainingsCount;
        result.solvedTasksCount += record.solvedTasksCount;
    }
    return result;
}

std::optional<TrainingSolvedTasksResult> TrainingInsightsService::computeSolvedTasksYear(
    const drogon::orm::DbClientPtr &dbClient,
    int userId,
    int courseId,
    ApiError &error) const
{
    const auto context = loadYearContext(dbClient, userId, courseId, error);
    if (!context.has_value())
    {
        return std::nullopt;
    }

    TrainingSolvedTasksResult result;
    for (const auto &record : context->userTrainings)
    {
        if (record.userId != userId)
        {
            continue;
        }
        ++result.trainingsCount;
        result.solvedTasksCount += record.solvedTasksCount;
    }
    return result;
}

std::optional<TrainingPerfectStreakResult> TrainingInsightsService::computePerfectStreakQuarter(
    const drogon::orm::DbClientPtr &dbClient,
    int quarter,
    int userId,
    int courseId,
    ApiError &error) const
{
    const auto context = loadQuarterContext(dbClient, quarter, userId, courseId, error);
    if (!context.has_value())
    {
        return std::nullopt;
    }
    return perfectStreakFromContext(*context, userId, error);
}

std::optional<TrainingPerfectStreakResult> TrainingInsightsService::computePerfectStreakYear(
    const drogon::orm::DbClientPtr &dbClient,
    int userId,
    int courseId,
    ApiError &error) const
{
    const auto context = loadYearContext(dbClient, userId, courseId, error);
    if (!context.has_value())
    {
        return std::nullopt;
    }
    return perfectStreakFromContext(*context, userId, error);
}

std::optional<TrainingStarterRankResult> TrainingInsightsService::computeStarterRankQuarter(
    const drogon::orm::DbClientPtr &dbClient,
    int quarter,
    int userId,
    int courseId,
    ApiError &error) const
{
    const auto context = loadQuarterContext(dbClient, quarter, userId, courseId, error);
    if (!context.has_value())
    {
        return std::nullopt;
    }
    return starterRankFromContext(*context, userId, error);
}

std::optional<TrainingStarterRankResult> TrainingInsightsService::computeStarterRankYear(
    const drogon::orm::DbClientPtr &dbClient,
    int userId,
    int courseId,
    ApiError &error) const
{
    const auto context = loadYearContext(dbClient, userId, courseId, error);
    if (!context.has_value())
    {
        return std::nullopt;
    }
    return starterRankFromContext(*context, userId, error);
}

std::optional<TrainingDifficultyStatsResult> TrainingInsightsService::computeDifficultyStatsQuarter(
    const drogon::orm::DbClientPtr &dbClient,
    int quarter,
    int userId,
    int courseId,
    ApiError &error) const
{
    const auto context = loadQuarterContext(dbClient, quarter, userId, courseId, error);
    if (!context.has_value())
    {
        return std::nullopt;
    }
    return difficultyStatsFromContext(*context, userId);
}

std::optional<TrainingDifficultyStatsResult> TrainingInsightsService::computeDifficultyStatsYear(
    const drogon::orm::DbClientPtr &dbClient,
    int userId,
    int courseId,
    ApiError &error) const
{
    const auto context = loadYearContext(dbClient, userId, courseId, error);
    if (!context.has_value())
    {
        return std::nullopt;
    }
    return difficultyStatsFromContext(*context, userId);
}
}  // namespace yearreporter::services
