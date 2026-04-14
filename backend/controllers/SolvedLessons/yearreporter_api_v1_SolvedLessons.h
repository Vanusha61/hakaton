#pragma once

#include <drogon/HttpController.h>

namespace api::v1
{
class solved_lessons : public drogon::HttpController<solved_lessons>
{
  public:
    METHOD_LIST_BEGIN
    METHOD_ADD(solved_lessons::quarter, "/quarter", drogon::Get);
    METHOD_ADD(solved_lessons::year, "/year", drogon::Get);
    METHOD_LIST_END

    void quarter(const drogon::HttpRequestPtr &request,
                 std::function<void(const drogon::HttpResponsePtr &)> &&callback);

    void year(const drogon::HttpRequestPtr &request,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};
}  // namespace api::v1
