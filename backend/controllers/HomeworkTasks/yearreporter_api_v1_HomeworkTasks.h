#pragma once

#include <drogon/HttpController.h>

namespace api::v1
{
class homework_tasks : public drogon::HttpController<homework_tasks>
{
  public:
    METHOD_LIST_BEGIN
    METHOD_ADD(homework_tasks::quarter, "/quarter", drogon::Get);
    METHOD_ADD(homework_tasks::year, "/year", drogon::Get);
    METHOD_LIST_END

    void quarter(const drogon::HttpRequestPtr &request,
                 std::function<void(const drogon::HttpResponsePtr &)> &&callback);

    void year(const drogon::HttpRequestPtr &request,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};
}  // namespace api::v1
