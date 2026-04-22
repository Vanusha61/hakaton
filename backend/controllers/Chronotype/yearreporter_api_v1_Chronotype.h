#pragma once

#include <drogon/HttpController.h>

namespace api::v1
{
class chronotype : public drogon::HttpController<chronotype>
{
  public:
    METHOD_LIST_BEGIN
    METHOD_ADD(chronotype::quarter, "/quarter", drogon::Get);
    METHOD_ADD(chronotype::year, "/year", drogon::Get);
    METHOD_LIST_END

    void quarter(const drogon::HttpRequestPtr &request,
                 std::function<void(const drogon::HttpResponsePtr &)> &&callback);

    void year(const drogon::HttpRequestPtr &request,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};
}  // namespace api::v1
