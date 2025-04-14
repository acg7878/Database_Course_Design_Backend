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
    ADD_METHOD_TO(ActivityRegistrationController::reviewRegistration, "/activity/register/review", Post);
    ADD_METHOD_TO(ActivityRegistrationController::getApprovedRegistrationsByUser, "/activity/registration/approved", Post);
    ADD_METHOD_TO(ActivityRegistrationController::setPaymentStatus, "/activity/register/payment", Post);
    METHOD_LIST_END

    void registerActivity(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) const;
    void cancelRegistration(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) const;
    void getRegistrationList(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) const;
    void reviewRegistration(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) const;
    void getApprovedRegistrationsByUser(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) const;
    void setPaymentStatus(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) const;
};