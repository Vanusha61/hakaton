#pragma once

#include <drogon/HttpController.h>

namespace api::v1
{
class study_recommendations : public drogon::HttpController<study_recommendations>
{
  public:
    METHOD_LIST_BEGIN
    METHOD_ADD(study_recommendations::quarter, "/quarter", drogon::Get);
    METHOD_ADD(study_recommendations::year, "/year", drogon::Get);
    METHOD_LIST_END

    void quarter(const drogon::HttpRequestPtr &request,
                 std::function<void(const drogon::HttpResponsePtr &)> &&callback);

    void year(const drogon::HttpRequestPtr &request,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};
}  // namespace api::v1
