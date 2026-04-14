#pragma once

#include <drogon/HttpController.h>

namespace api::v1
{
class first_video_starter : public drogon::HttpController<first_video_starter>
{
  public:
    METHOD_LIST_BEGIN
    METHOD_ADD(first_video_starter::quarter, "/quarter", drogon::Get);
    METHOD_ADD(first_video_starter::year, "/year", drogon::Get);
    METHOD_LIST_END

    void quarter(const drogon::HttpRequestPtr &request,
                 std::function<void(const drogon::HttpResponsePtr &)> &&callback);

    void year(const drogon::HttpRequestPtr &request,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};
}  // namespace api::v1
