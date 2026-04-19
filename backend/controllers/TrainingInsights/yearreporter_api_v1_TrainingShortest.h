#pragma once

#include <drogon/HttpController.h>

namespace api::v1
{
class training_shortest : public drogon::HttpController<training_shortest>
{
  public:
    METHOD_LIST_BEGIN
    METHOD_ADD(training_shortest::quarter, "/quarter", drogon::Get);
    METHOD_ADD(training_shortest::year, "/year", drogon::Get);
    METHOD_LIST_END

    void quarter(const drogon::HttpRequestPtr &request,
                 std::function<void(const drogon::HttpResponsePtr &)> &&callback);

    void year(const drogon::HttpRequestPtr &request,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};
}  // namespace api::v1
