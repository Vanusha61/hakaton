#pragma once

#include <drogon/HttpController.h>

namespace api::v1
{
class perseverance : public drogon::HttpController<perseverance>
{
  public:
    METHOD_LIST_BEGIN
    METHOD_ADD(perseverance::year, "/year", drogon::Get);
    METHOD_LIST_END

    void year(const drogon::HttpRequestPtr &request,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};
}  // namespace api::v1
