#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

struct ClubMember {
  int user_id;        // 用户 ID
  int club_id;        // 社团 ID
  std::string status; // 成员状态（如 "pending", "approved", "rejected"）
};

class ClubMemberController
    : public drogon::HttpController<ClubMemberController> {
public:
  METHOD_LIST_BEGIN
  // 申请加入社团
  ADD_METHOD_TO(ClubMemberController::apply, "/club/member/apply", Post);
  // 审核加入申请
  ADD_METHOD_TO(ClubMemberController::approve, "/club/member/approve", Post);
  // 移除成员
  ADD_METHOD_TO(ClubMemberController::remove, "/club/member/remove", Delete);
  // 获取社团成员列表
  ADD_METHOD_TO(ClubMemberController::list, "/club/member/list/{1}",
                Get); // {1} 表示 club_id
  // 获取用户作为社长的所有社团下的申请列表
  ADD_METHOD_TO(ClubMemberController::getAllApplications, "/club/member/all_applications", Get);
  METHOD_LIST_END

  // 申请加入社团方法
  void apply(const HttpRequestPtr &req,
             std::function<void(const HttpResponsePtr &)> &&callback) const;

  // 审核加入申请方法
  void approve(const HttpRequestPtr &req,
               std::function<void(const HttpResponsePtr &)> &&callback) const;

  // 移除成员方法
  void remove(const HttpRequestPtr &req,
              std::function<void(const HttpResponsePtr &)> &&callback) const;

  // 获取社团成员列表方法
  void list(const HttpRequestPtr &req,
            std::function<void(const HttpResponsePtr &)> &&callback,
            int club_id) const;

  // 获取用户作为社长的所有社团下的申请列表方法
  void getAllApplications(const HttpRequestPtr &req,
                          std::function<void(const HttpResponsePtr &)> &&callback) const;
};

namespace drogon {
template <> inline ClubMember fromRequest(const HttpRequest &req) {
  auto json = req.getJsonObject();
  ClubMember value;

  if (json == nullptr) {
    throw std::runtime_error("请求体格式错误，请使用 JSON");
  }

  // 解析 user_id
  if (json->isMember("user_id")) {
    value.user_id = (*json)["user_id"].asInt();
  } else {
    throw std::runtime_error("缺少必备字段: user_id");
  }

  // 解析 club_id
  if (json->isMember("club_id")) {
    value.club_id = (*json)["club_id"].asInt();
  } else {
    throw std::runtime_error("缺少必备字段: club_id");
  }

  // 解析 status（可选字段，默认为 "pending"）
  value.status = json->get("status", "pending").asString();

  return value;
}
} // namespace drogon