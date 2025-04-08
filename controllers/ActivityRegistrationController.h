#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

class ActivityRegistrationController : public drogon::HttpController<ActivityRegistrationController>
{
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(ActivityRegistrationController::registerActivity, "/activity/register", Post);
    ADD_METHOD_TO(ActivityRegistrationController::cancelRegistration, "/activity/register/cancel", Post);
    ADD_METHOD_TO(ActivityRegistrationController::getRegistrationList, "/activity/register/list", Get);
    METHOD_LIST_END

    void registerActivity(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) const;
    void cancelRegistration(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) const;
    void getRegistrationList(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) const;
};