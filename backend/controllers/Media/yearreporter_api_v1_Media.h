#pragma once

#include <drogon/HttpController.h>

namespace api::v1
{
class media : public drogon::HttpController<media>
{
  public:
    METHOD_LIST_BEGIN
    METHOD_ADD(media::quarter, "/quarter", drogon::Get);
    METHOD_ADD(media::year, "/year", drogon::Get);
    METHOD_LIST_END

    void quarter(const drogon::HttpRequestPtr &request,
                 std::function<void(const drogon::HttpResponsePtr &)> &&callback);

    void year(const drogon::HttpRequestPtr &request,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};
}  // namespace api::v1
