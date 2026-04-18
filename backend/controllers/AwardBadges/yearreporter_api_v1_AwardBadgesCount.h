#pragma once

#include <drogon/HttpController.h>

namespace api::v1
{
class award_badges_count : public drogon::HttpController<award_badges_count>
{
  public:
    METHOD_LIST_BEGIN
    METHOD_ADD(award_badges_count::quarter, "/quarter", drogon::Get);
    METHOD_ADD(award_badges_count::year, "/year", drogon::Get);
    METHOD_LIST_END

    void quarter(const drogon::HttpRequestPtr &request,
                 std::function<void(const drogon::HttpResponsePtr &)> &&callback);

    void year(const drogon::HttpRequestPtr &request,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};
}  // namespace api::v1
