#pragma once
#include <drogon/HttpController.h>

using namespace drogon;

struct ClubActivity {
  int activity_id;                  // 活动 ID
  int club_id;                      // 所属社团 ID
  std::string activity_title;       // 活动标题
  trantor::Date activity_time;      // 活动时间
  std::string activity_location;    // 活动地点
  std::string registration_method;  // 报名方式
  std::string activity_description; // 活动描述
  trantor::Date publish_time;       // 发布时间
};

class ClubActivityController
    : public drogon::HttpController<ClubActivityController> {
public:
  METHOD_LIST_BEGIN
  ADD_METHOD_TO(ClubActivityController::createActivity, "/activity/create",
                Post);
  ADD_METHOD_TO(ClubActivityController::getActivityList, "/activity/list/{1}",
                Get);
  ADD_METHOD_TO(ClubActivityController::getActivityDetail,
                "/activity/detail/{1}", Get);
  ADD_METHOD_TO(ClubActivityController::updateActivity, "/activity/update/{1}",
                Put);
  ADD_METHOD_TO(ClubActivityController::deleteActivity, "/activity/delete/{1}",
                Delete);
  // 获取当前用户所属社团的所有活动
  ADD_METHOD_TO(ClubActivityController::getAllActivitiesByUser,
                "/club/activity/all_by_user", Get);
  ADD_METHOD_TO(ClubActivityController::getAllActivitiesByClub,
                "/club/activity/all_by_club", Post);
  ADD_METHOD_TO(ClubActivityController::getActivityRegistrationsByClub,
                "/club/activity/registrations", Post);

  METHOD_LIST_END

  void createActivity(const HttpRequestPtr &req,
                      std::function<void(const HttpResponsePtr &)> &&callback,
                      ClubActivity activity) const;

  void getActivityList(const HttpRequestPtr &req,
                       std::function<void(const HttpResponsePtr &)> &&callback,
                       int clubId) const;
  void
  getActivityDetail(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback,
                    int activityId) const;
  void updateActivity(const HttpRequestPtr &req,
                      std::function<void(const HttpResponsePtr &)> &&callback,
                      int activityId) const;
  void deleteActivity(const HttpRequestPtr &req,
                      std::function<void(const HttpResponsePtr &)> &&callback,
                      int activityId) const;

  // 获取当前用户所属社团的所有活动方法
  void getAllActivitiesByUser(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback) const;
  void getAllActivitiesByClub(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback) const;
  void getActivityRegistrationsByClub(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback) const;
};

namespace drogon {
template <> inline ClubActivity fromRequest(const HttpRequest &req) {
  auto json = req.getJsonObject();
  ClubActivity value;

  if (json == nullptr) {
    throw std::runtime_error("请求体格式错误，请使用 JSON");
  }

  // 解析活动标题
  if (json->isMember("activity_title")) {
    const auto &temp = (*json)["activity_title"].asString();
    if (temp.empty()) {
      throw std::runtime_error("活动标题不能为空");
    }
    value.activity_title = temp;
  } else {
    throw std::runtime_error("缺少必备字段: activity_title");
  }

  // 解析活动时间
  if (json->isMember("activity_time")) {
    const auto &temp = (*json)["activity_time"].asString();
    if (temp.empty()) {
      throw std::runtime_error("活动时间不能为空");
    }
    value.activity_time = trantor::Date::fromDbStringLocal(temp);
  } else {
    throw std::runtime_error("缺少必备字段: activity_time");
  }

  // 解析活动地点
  value.activity_location = json->get("activity_location", "").asString();

  // 解析报名方式
  value.registration_method = json->get("registration_method", "").asString();

  // 解析活动描述
  value.activity_description = json->get("activity_description", "").asString();

  // 解析社团 ID
  if (json->isMember("club_id")) {
    value.club_id = (*json)["club_id"].asInt();
  } else {
    throw std::runtime_error("缺少必备字段: club_id");
  }

  return value;
}
} // namespace drogon