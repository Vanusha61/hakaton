#pragma once

#include <drogon/HttpController.h>

namespace api::v1
{
class lesson_tasks : public drogon::HttpController<lesson_tasks>
{
  public:
    METHOD_LIST_BEGIN
    METHOD_ADD(lesson_tasks::quarter, "/quarter", drogon::Get);
    METHOD_ADD(lesson_tasks::year, "/year", drogon::Get);
    METHOD_LIST_END

    void quarter(const drogon::HttpRequestPtr &request,
                 std::function<void(const drogon::HttpResponsePtr &)> &&callback);

    void year(const drogon::HttpRequestPtr &request,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};
}  // namespace api::v1
