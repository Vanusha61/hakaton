#pragma once

#include <drogon/HttpController.h>

namespace api::v1
{
class video_attention : public drogon::HttpController<video_attention>
{
  public:
    METHOD_LIST_BEGIN
    METHOD_ADD(video_attention::quarter, "/quarter", drogon::Get);
    METHOD_ADD(video_attention::year, "/year", drogon::Get);
    METHOD_LIST_END

    void quarter(const drogon::HttpRequestPtr &request,
                 std::function<void(const drogon::HttpResponsePtr &)> &&callback);

    void year(const drogon::HttpRequestPtr &request,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};
}  // namespace api::v1
