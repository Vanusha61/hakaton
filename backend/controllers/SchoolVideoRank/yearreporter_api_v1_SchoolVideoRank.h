#pragma once

#include <drogon/HttpController.h>

namespace api::v1
{
class school_video_rank : public drogon::HttpController<school_video_rank>
{
  public:
    METHOD_LIST_BEGIN
    METHOD_ADD(school_video_rank::quarter, "/quarter", drogon::Get);
    METHOD_ADD(school_video_rank::year, "/year", drogon::Get);
    METHOD_LIST_END

    void quarter(const drogon::HttpRequestPtr &request,
                 std::function<void(const drogon::HttpResponsePtr &)> &&callback);

    void year(const drogon::HttpRequestPtr &request,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};
}  // namespace api::v1
