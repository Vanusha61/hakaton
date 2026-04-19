#pragma once

#include <drogon/HttpController.h>

namespace api::v1
{
class training_difficulty_stats : public drogon::HttpController<training_difficulty_stats>
{
  public:
    METHOD_LIST_BEGIN
    METHOD_ADD(training_difficulty_stats::quarter, "/quarter", drogon::Get);
    METHOD_ADD(training_difficulty_stats::year, "/year", drogon::Get);
    METHOD_LIST_END

    void quarter(const drogon::HttpRequestPtr &request,
                 std::function<void(const drogon::HttpResponsePtr &)> &&callback);

    void year(const drogon::HttpRequestPtr &request,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};
}  // namespace api::v1
