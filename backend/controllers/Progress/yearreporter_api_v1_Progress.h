#pragma once

#include <drogon/HttpController.h>

namespace api::v1
{
class progress : public drogon::HttpController<progress>
{
  public:
    METHOD_LIST_BEGIN
    METHOD_ADD(progress::quarter, "/quarter", drogon::Get);
    METHOD_ADD(progress::course, "/course", drogon::Get);
    METHOD_LIST_END

    void quarter(const drogon::HttpRequestPtr &request,
                 std::function<void(const drogon::HttpResponsePtr &)> &&callback);

    void course(const drogon::HttpRequestPtr &request,
                std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};
}  // namespace api::v1
