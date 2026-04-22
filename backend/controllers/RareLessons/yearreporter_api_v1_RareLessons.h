#pragma once

#include <drogon/HttpController.h>

namespace api::v1
{
class rare_lessons : public drogon::HttpController<rare_lessons>
{
  public:
    METHOD_LIST_BEGIN
    METHOD_ADD(rare_lessons::quarter, "/quarter", drogon::Get);
    METHOD_ADD(rare_lessons::year, "/year", drogon::Get);
    METHOD_LIST_END

    void quarter(const drogon::HttpRequestPtr &request,
                 std::function<void(const drogon::HttpResponsePtr &)> &&callback);

    void year(const drogon::HttpRequestPtr &request,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};
}  // namespace api::v1
