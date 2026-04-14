#pragma once

#include <drogon/HttpController.h>

namespace api::v1
{
class rarest_visited_lesson : public drogon::HttpController<rarest_visited_lesson>
{
  public:
    METHOD_LIST_BEGIN
    METHOD_ADD(rarest_visited_lesson::quarter, "/quarter", drogon::Get);
    METHOD_ADD(rarest_visited_lesson::year, "/year", drogon::Get);
    METHOD_LIST_END

    void quarter(const drogon::HttpRequestPtr &request,
                 std::function<void(const drogon::HttpResponsePtr &)> &&callback);

    void year(const drogon::HttpRequestPtr &request,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};
}  // namespace api::v1
