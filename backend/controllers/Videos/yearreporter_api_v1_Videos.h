#pragma once

#include <drogon/HttpController.h>

namespace api::v1
{
class videos : public drogon::HttpController<videos>
{
  public:
    METHOD_LIST_BEGIN
    METHOD_ADD(videos::quarter, "/quarter", drogon::Get);
    METHOD_ADD(videos::year, "/year", drogon::Get);
    METHOD_LIST_END

    void quarter(const drogon::HttpRequestPtr &request,
                 std::function<void(const drogon::HttpResponsePtr &)> &&callback);

    void year(const drogon::HttpRequestPtr &request,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};
}  // namespace api::v1
