#include <drogon/drogon.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

using drogon::HttpRequestPtr;
using drogon::HttpResponsePtr;
using drogon::orm::DbClient;
using drogon::orm::Result;

namespace
{
constexpr double kQuarterWeightPoints = 0.60;
constexpr double kQuarterWeightTasks = 0.25;
constexpr double kQuarterWeightVideos = 0.15;
constexpr int kQuarterCount = 4;
constexpr double kPointEpsilon = 0.01;

struct ApiError
{
    drogon::HttpStatusCode status{drogon::k500InternalServerError};
    std::string reason;
};

struct UserCourse
{
    int userId{0};
    int courseId{0};
    double earnedPoints{0.0};
    double maxPoints{0.0};
    int maxViewableLessons{0};
    int maxTaskCount{0};
    int solvedTaskCount{0};
};

struct LessonRecord
{
    std::string createdAt;
    int lessonId{0};
    double points{0.0};
    int solvedTasks{0};
    bool videoViewed{false};
};

struct QuarterSlice
{
    size_t begin{0};
    size_t end{0};
};

struct QuarterMetrics
{
    int quarter{0};
    double fraction{0.0};
    int percent{0};
};

struct AppConfig
{
    std::string listenAddress{"127.0.0.1"};
    uint16_t listenPort{8080};
    std::string dbHost{"127.0.0.1"};
    uint16_t dbPort{5555};
    std::string dbName{"xakaton"};
    std::string dbUser{"postgres"};
    std::string dbPassword{"postgres"};
    size_t dbConnections{1};
    trantor::Logger::LogLevel logLevel{trantor::Logger::kWarn};
};

int roundPercent(double value)
{
    const auto clamped = std::clamp(value, 0.0, 100.0);
    return static_cast<int>(std::lround(clamped));
}

double safeRatio(double numerator, double denominator)
{
    if (denominator <= 0.0)
    {
        return 0.0;
    }
    return std::clamp(numerator / denominator, 0.0, 1.0);
}

drogon::HttpResponsePtr makeJsonResponse(const Json::Value &json,
                                         drogon::HttpStatusCode code = drogon::k200OK)
{
    auto response = drogon::HttpResponse::newHttpJsonResponse(json);
    response->setStatusCode(code);
    return response;
}

drogon::HttpResponsePtr makeErrorResponse(drogon::HttpStatusCode code,
                                          const std::string &reason)
{
    Json::Value json;
    json["reason"] = reason;
    return makeJsonResponse(json, code);
}

std::optional<int> parseIntQueryParam(const HttpRequestPtr &request,
                                      const std::string &name,
                                      ApiError &error)
{
    const auto value = request->getParameter(name);
    if (value.empty())
    {
        error = {drogon::k400BadRequest,
                 "Не указан обязательный параметр '" + name + "'"};
        return std::nullopt;
    }

    try
    {
        size_t pos = 0;
        const int parsed = std::stoi(value, &pos);
        if (pos != value.size())
        {
            throw std::invalid_argument("tail");
        }
        return parsed;
    }
    catch (const std::exception &)
    {
        error = {drogon::k400BadRequest,
                 "Параметр '" + name + "' должен быть целым числом"};
        return std::nullopt;
    }
}

class ProgressService
{
  public:
    using DbClientPtr = std::shared_ptr<DbClient>;

    explicit ProgressService(DbClientPtr dbClient) : dbClient_(std::move(dbClient))
    {
    }

    std::optional<QuarterMetrics> computeQuarterProgress(int quarter,
                                                         int userId,
                                                         int courseId,
                                                         ApiError &error) const
    {
        if (quarter < 1 || quarter > kQuarterCount)
        {
            error = {drogon::k400BadRequest,
                     "Параметр 'quarter' должен быть числом от 1 до 4"};
            return std::nullopt;
        }

        const auto course = findUserCourse(userId, courseId, error);
        if (!course.has_value())
        {
            return std::nullopt;
        }

        const auto lessonTrackId = findMatchingLessonTrack(*course, error);
        if (!lessonTrackId.has_value())
        {
            return std::nullopt;
        }

        const auto lessons = loadLessons(userId, *lessonTrackId, error);
        if (!lessons.has_value())
        {
            return std::nullopt;
        }
        if (lessons->empty())
        {
            error = {drogon::k404NotFound,
                     "Для пользователя не найдены уроки по указанному курсу"};
            return std::nullopt;
        }

        return calculateQuarterMetrics(*course, *lessons, quarter);
    }

    std::optional<int> computeCourseProgress(int userId, int courseId, ApiError &error) const
    {
        double totalFraction = 0.0;
        for (int quarter = 1; quarter <= kQuarterCount; ++quarter)
        {
            auto quarterMetrics = computeQuarterProgress(quarter, userId, courseId, error);
            if (!quarterMetrics.has_value())
            {
                return std::nullopt;
            }
            totalFraction += quarterMetrics->fraction;
        }

        return roundPercent(totalFraction * 25.0);
    }

  private:
    std::optional<UserCourse> findUserCourse(int userId, int courseId, ApiError &error) const
    {
        try
        {
            const auto result = dbClient_->execSqlSync(
                "select user_id, course_id, wk_points, wk_max_points, "
                "wk_max_viewable_lessons, wk_max_task_count, wk_solved_task_count "
                "from public.user_courses where user_id = $1 and course_id = $2 limit 1",
                userId,
                courseId);

            if (result.empty())
            {
                error = {drogon::k404NotFound,
                         "Курс или пользователь не найдены в базе"};
                return std::nullopt;
            }

            const auto &row = result.front();
            return UserCourse{
                row["user_id"].as<int>(),
                row["course_id"].as<int>(),
                row["wk_points"].as<double>(),
                row["wk_max_points"].as<double>(),
                row["wk_max_viewable_lessons"].as<int>(),
                row["wk_max_task_count"].as<int>(),
                row["wk_solved_task_count"].as<int>()};
        }
        catch (const std::exception &e)
        {
            error = {drogon::k500InternalServerError,
                     "Ошибка чтения данных курса: " + std::string(e.what())};
            return std::nullopt;
        }
    }

    std::optional<int> findMatchingLessonTrack(const UserCourse &course, ApiError &error) const
    {
        try
        {
            const auto result = dbClient_->execSqlSync(
                "select users_course_id, "
                "sum(coalesce(wk_points, 0)) as lesson_points, "
                "sum(coalesce(wk_solved_task_count, 0)) as lesson_solved "
                "from public.user_lessons "
                "where user_id = $1 "
                "group by users_course_id",
                course.userId);

            if (result.empty())
            {
                error = {drogon::k404NotFound,
                         "Для пользователя не найдены данные по урокам"};
                return std::nullopt;
            }

            double bestScore = std::numeric_limits<double>::max();
            std::optional<int> bestId;

            for (const auto &row : result)
            {
                const auto lessonPoints = row["lesson_points"].as<double>();
                const auto lessonSolved = row["lesson_solved"].as<int>();
                const auto exactPoints =
                    std::fabs(lessonPoints - course.earnedPoints) <= kPointEpsilon;
                const auto exactSolved = lessonSolved == course.solvedTaskCount;

                if (exactPoints && exactSolved)
                {
                    return row["users_course_id"].as<int>();
                }

                const auto score = std::fabs(lessonPoints - course.earnedPoints) +
                                   std::fabs(static_cast<double>(lessonSolved - course.solvedTaskCount)) *
                                       100.0;
                if (score < bestScore)
                {
                    bestScore = score;
                    bestId = row["users_course_id"].as<int>();
                }
            }

            if (!bestId.has_value())
            {
                error = {drogon::k404NotFound,
                         "Не удалось определить траекторию уроков пользователя"};
                return std::nullopt;
            }

            return bestId;
        }
        catch (const std::exception &e)
        {
            error = {drogon::k500InternalServerError,
                     "Ошибка чтения уроков курса: " + std::string(e.what())};
            return std::nullopt;
        }
    }

    std::optional<std::vector<LessonRecord>> loadLessons(int userId,
                                                         int usersCourseId,
                                                         ApiError &error) const
    {
        try
        {
            const auto result = dbClient_->execSqlSync(
                "select created_at, lesson_id, coalesce(wk_points, 0) as wk_points, "
                "coalesce(wk_solved_task_count, 0) as wk_solved_task_count, "
                "coalesce(video_viewed, false) as video_viewed "
                "from public.user_lessons "
                "where user_id = $1 and users_course_id = $2 "
                "order by created_at, lesson_id",
                userId,
                usersCourseId);

            std::vector<LessonRecord> lessons;
            lessons.reserve(result.size());
            for (const auto &row : result)
            {
                lessons.push_back(
                    LessonRecord{row["created_at"].as<std::string>(),
                                 row["lesson_id"].as<int>(),
                                 row["wk_points"].as<double>(),
                                 row["wk_solved_task_count"].as<int>(),
                                 row["video_viewed"].as<bool>()});
            }
            return lessons;
        }
        catch (const std::exception &e)
        {
            error = {drogon::k500InternalServerError,
                     "Ошибка загрузки уроков: " + std::string(e.what())};
            return std::nullopt;
        }
    }

    static QuarterSlice quarterSliceFor(const std::vector<LessonRecord> &lessons, int quarter)
    {
        const auto totalLessons = lessons.size();
        const auto baseSize = totalLessons / kQuarterCount;
        const auto remainder = totalLessons % kQuarterCount;

        size_t begin = 0;
        for (int idx = 1; idx < quarter; ++idx)
        {
            begin += baseSize + (idx <= static_cast<int>(remainder) ? 1 : 0);
        }

        const auto currentSize = baseSize + (quarter <= static_cast<int>(remainder) ? 1 : 0);
        return QuarterSlice{begin, begin + currentSize};
    }

    static QuarterMetrics calculateQuarterMetrics(const UserCourse &course,
                                                  const std::vector<LessonRecord> &lessons,
                                                  int quarter)
    {
        const auto slice = quarterSliceFor(lessons, quarter);
        const auto quarterLessons = static_cast<double>(slice.end - slice.begin);
        const auto totalLessons = static_cast<double>(lessons.size());

        double quarterPoints = 0.0;
        double quarterSolvedTasks = 0.0;
        double quarterViewedVideos = 0.0;

        for (size_t index = slice.begin; index < slice.end; ++index)
        {
            quarterPoints += lessons[index].points;
            quarterSolvedTasks += static_cast<double>(lessons[index].solvedTasks);
            quarterViewedVideos += lessons[index].videoViewed ? 1.0 : 0.0;
        }

        const auto quarterMaxPoints =
            totalLessons > 0.0 ? course.maxPoints * (quarterLessons / totalLessons) : 0.0;
        const auto quarterMaxTasks =
            totalLessons > 0.0 ? static_cast<double>(course.maxTaskCount) * (quarterLessons / totalLessons)
                               : 0.0;
        const auto quarterMaxVideos =
            totalLessons > 0.0
                ? static_cast<double>(course.maxViewableLessons) * (quarterLessons / totalLessons)
                : 0.0;

        const auto pointsProgress = safeRatio(quarterPoints, quarterMaxPoints);
        const auto tasksProgress = safeRatio(quarterSolvedTasks, quarterMaxTasks);
        const auto videosProgress = safeRatio(quarterViewedVideos, quarterMaxVideos);

        const auto fraction = std::clamp(kQuarterWeightPoints * pointsProgress +
                                             kQuarterWeightTasks * tasksProgress +
                                             kQuarterWeightVideos * videosProgress,
                                         0.0,
                                         1.0);

        return QuarterMetrics{quarter, fraction, roundPercent(fraction * 100.0)};
    }

    DbClientPtr dbClient_;
};

trantor::Logger::LogLevel parseLogLevel(const std::string &value)
{
    if (value == "TRACE")
    {
        return trantor::Logger::kTrace;
    }
    if (value == "DEBUG")
    {
        return trantor::Logger::kDebug;
    }
    if (value == "INFO")
    {
        return trantor::Logger::kInfo;
    }
    if (value == "WARN")
    {
        return trantor::Logger::kWarn;
    }
    if (value == "ERROR")
    {
        return trantor::Logger::kError;
    }
    if (value == "FATAL")
    {
        return trantor::Logger::kFatal;
    }
    return trantor::Logger::kWarn;
}

AppConfig loadConfig(const std::string &path)
{
    std::ifstream input(path);
    if (!input)
    {
        throw std::runtime_error("Не удалось открыть config.json: " + path);
    }

    Json::CharReaderBuilder builder;
    Json::Value root;
    std::string errors;
    if (!Json::parseFromStream(builder, input, &root, &errors))
    {
        throw std::runtime_error("Не удалось разобрать config.json: " + errors);
    }

    AppConfig config;

    const auto &listeners = root["listeners"];
    if (listeners.isArray() && !listeners.empty())
    {
        const auto &listener = listeners[0];
        config.listenAddress = listener.get("address", config.listenAddress).asString();
        config.listenPort =
            static_cast<uint16_t>(listener.get("port", config.listenPort).asUInt());
    }

    const auto &dbClients = root["db_clients"];
    if (dbClients.isArray() && !dbClients.empty())
    {
        const auto &db = dbClients[0];
        config.dbHost = db.get("host", config.dbHost).asString();
        config.dbPort = static_cast<uint16_t>(db.get("port", config.dbPort).asUInt());
        config.dbName = db.get("dbname", config.dbName).asString();
        config.dbUser = db.get("user", config.dbUser).asString();
        config.dbPassword = db.get("passwd", config.dbPassword).asString();
        config.dbConnections =
            static_cast<size_t>(db.get("number_of_connections",
                                       static_cast<Json::UInt64>(config.dbConnections))
                                    .asUInt64());
    }

    const auto &log = root["app"]["log"];
    if (log.isObject())
    {
        config.logLevel = parseLogLevel(log.get("log_level", "WARN").asString());
    }

    return config;
}

}  // namespace

int main(int argc, char *argv[]) {
    const std::string configPath = argc > 1 ? argv[1] : "config.json";
    const auto config = loadConfig(configPath);

    const auto pgConnectionString = "host=" + config.dbHost +
                                    " port=" + std::to_string(config.dbPort) +
                                    " dbname=" + config.dbName +
                                    " user=" + config.dbUser +
                                    " password=" + config.dbPassword;
    auto dbClient = DbClient::newPgClient(pgConnectionString, config.dbConnections);
    auto progressService = std::make_shared<ProgressService>(dbClient);

    drogon::app().registerHandler(
        "/api/v1/progress/quarter",
        [progressService](const HttpRequestPtr &request,
                          std::function<void(const HttpResponsePtr &)> &&callback) {
            ApiError error;
            const auto quarter = parseIntQueryParam(request, "quarter", error);
            if (!quarter.has_value())
            {
                callback(makeErrorResponse(error.status, error.reason));
                return;
            }

            const auto userId = parseIntQueryParam(request, "user_id", error);
            if (!userId.has_value())
            {
                callback(makeErrorResponse(error.status, error.reason));
                return;
            }

            const auto courseId = parseIntQueryParam(request, "course_id", error);
            if (!courseId.has_value())
            {
                callback(makeErrorResponse(error.status, error.reason));
                return;
            }

            const auto progress =
                progressService->computeQuarterProgress(*quarter, *userId, *courseId, error);
            if (!progress.has_value())
            {
                callback(makeErrorResponse(error.status, error.reason));
                return;
            }

            Json::Value json;
            json["quarter"] = progress->quarter;
            json["progress"] = progress->percent;
            json["user_id"] = *userId;
            json["course_id"] = *courseId;
            callback(makeJsonResponse(json));
        },
        {drogon::Get});

    drogon::app().registerHandler(
        "/api/v1/progress/course",
        [progressService](const HttpRequestPtr &request,
                          std::function<void(const HttpResponsePtr &)> &&callback) {
            ApiError error;
            const auto userId = parseIntQueryParam(request, "user_id", error);
            if (!userId.has_value())
            {
                callback(makeErrorResponse(error.status, error.reason));
                return;
            }

            const auto courseId = parseIntQueryParam(request, "course_id", error);
            if (!courseId.has_value())
            {
                callback(makeErrorResponse(error.status, error.reason));
                return;
            }

            const auto progress = progressService->computeCourseProgress(*userId, *courseId, error);
            if (!progress.has_value())
            {
                callback(makeErrorResponse(error.status, error.reason));
                return;
            }

            Json::Value json;
            json["progress"] = *progress;
            json["user_id"] = *userId;
            json["course_id"] = *courseId;
            callback(makeJsonResponse(json));
        },
        {drogon::Get});

    drogon::app().setLogLevel(config.logLevel);
    drogon::app().addListener(config.listenAddress, config.listenPort);
    drogon::app().run();
}
