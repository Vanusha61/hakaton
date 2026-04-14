#pragma once

#include <drogon/HttpController.h>

namespace api::v1
{
class attendance : public drogon::HttpController<attendance>
{
  public:
    METHOD_LIST_BEGIN
    METHOD_ADD(attendance::quarter, "/quarter", drogon::Get);
    METHOD_ADD(attendance::year, "/year", drogon::Get);
    METHOD_LIST_END

    void quarter(const drogon::HttpRequestPtr &request,
                 std::function<void(const drogon::HttpResponsePtr &)> &&callback);

    void year(const drogon::HttpRequestPtr &request,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};
}  // namespace api::v1
