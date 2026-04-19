#pragma once

#include <drogon/HttpController.h>

namespace api::v1
{
class award_badges_rarest : public drogon::HttpController<award_badges_rarest>
{
  public:
    METHOD_LIST_BEGIN
    METHOD_ADD(award_badges_rarest::year, "/year", drogon::Get);
    METHOD_LIST_END

    void year(const drogon::HttpRequestPtr &request,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};
}  // namespace api::v1
