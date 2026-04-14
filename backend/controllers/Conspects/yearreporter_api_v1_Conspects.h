#pragma once

#include <drogon/HttpController.h>

namespace api::v1
{
class conspects : public drogon::HttpController<conspects>
{
  public:
    METHOD_LIST_BEGIN
    METHOD_ADD(conspects::quarter, "/quarter", drogon::Get);
    METHOD_ADD(conspects::year, "/year", drogon::Get);
    METHOD_LIST_END

    void quarter(const drogon::HttpRequestPtr &request,
                 std::function<void(const drogon::HttpResponsePtr &)> &&callback);

    void year(const drogon::HttpRequestPtr &request,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};
}  // namespace api::v1
