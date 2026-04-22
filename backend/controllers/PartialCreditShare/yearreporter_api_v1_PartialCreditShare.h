#pragma once

#include <drogon/HttpController.h>

namespace api::v1
{
class partial_credit_share : public drogon::HttpController<partial_credit_share>
{
  public:
    METHOD_LIST_BEGIN
    METHOD_ADD(partial_credit_share::year, "/year", drogon::Get);
    METHOD_LIST_END

    void year(const drogon::HttpRequestPtr &request,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};
}  // namespace api::v1
