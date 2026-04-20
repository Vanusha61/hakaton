#pragma once

#include <drogon/HttpController.h>

namespace api::v1
{
class average_score : public drogon::HttpController<average_score>
{
  public:
    METHOD_LIST_BEGIN
    METHOD_ADD(average_score::quarter, "/quarter", drogon::Get);
    METHOD_ADD(average_score::year, "/year", drogon::Get);
    METHOD_LIST_END

    void quarter(const drogon::HttpRequestPtr &request,
                 std::function<void(const drogon::HttpResponsePtr &)> &&callback);

    void year(const drogon::HttpRequestPtr &request,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};
}  // namespace api::v1
