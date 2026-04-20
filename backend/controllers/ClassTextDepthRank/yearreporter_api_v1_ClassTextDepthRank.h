#pragma once

#include <drogon/HttpController.h>

namespace api::v1
{
class class_text_depth_rank : public drogon::HttpController<class_text_depth_rank>
{
  public:
    METHOD_LIST_BEGIN
    METHOD_ADD(class_text_depth_rank::quarter, "/quarter", drogon::Get);
    METHOD_ADD(class_text_depth_rank::year, "/year", drogon::Get);
    METHOD_LIST_END

    void quarter(const drogon::HttpRequestPtr &request,
                 std::function<void(const drogon::HttpResponsePtr &)> &&callback);

    void year(const drogon::HttpRequestPtr &request,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};
}  // namespace api::v1
