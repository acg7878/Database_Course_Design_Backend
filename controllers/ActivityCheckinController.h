#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

class ActivityCheckinController : public drogon::HttpController<ActivityCheckinController>
{
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(ActivityCheckinController::checkin, "/activity/checkin", Post);
    ADD_METHOD_TO(ActivityCheckinController::getCheckinList, "/activity/checkin/list/{activity_id}", Get);
    ADD_METHOD_TO(ActivityCheckinController::getRegisteredActivitiesByUser, "/activity/registration/user", Post);

    METHOD_LIST_END

    void checkin(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) const;
    void getCheckinList(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int activityId) const;
    void getRegisteredActivitiesByUser(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) const;
};