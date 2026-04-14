#pragma once

#include <drogon/HttpController.h>

namespace api::v1
{
class points : public drogon::HttpController<points>
{
  public:
    METHOD_LIST_BEGIN
    METHOD_ADD(points::quarter, "/quarter", drogon::Get);
    METHOD_ADD(points::year, "/year", drogon::Get);
    METHOD_LIST_END

    void quarter(const drogon::HttpRequestPtr &request,
                 std::function<void(const drogon::HttpResponsePtr &)> &&callback);

    void year(const drogon::HttpRequestPtr &request,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};
}  // namespace api::v1
