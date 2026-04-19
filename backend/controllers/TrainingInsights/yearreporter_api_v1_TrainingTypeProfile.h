#pragma once

#include <drogon/HttpController.h>

namespace api::v1
{
class training_type_profile : public drogon::HttpController<training_type_profile>
{
  public:
    METHOD_LIST_BEGIN
    METHOD_ADD(training_type_profile::year, "/year", drogon::Get);
    METHOD_LIST_END

    void year(const drogon::HttpRequestPtr &request,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};
}  // namespace api::v1
