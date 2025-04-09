#include "ClubMemberController.h"
#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>
#include <drogon/orm/Exception.h>

// 申请加入社团
void ClubMemberController::apply(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) const {
  auto dbClient = drogon::app().getDbClient();
  Json::Value response;

  try {
    // 使用自定义解析方法解析 ClubMember 对象
    auto clubMember = drogon::fromRequest<ClubMember>(*req);

    // 检查用户是否已经是该社团的成员
    auto memberCheckResult = dbClient->execSqlSync(
        "SELECT member_id FROM club_member WHERE user_id = ? AND club_id = ?",
        clubMember.user_id, clubMember.club_id);

    if (!memberCheckResult.empty()) {
      response["error"] = "您已经是该社团的成员，无法重复申请";
      auto resp = HttpResponse::newHttpJsonResponse(response);
      resp->setStatusCode(k400BadRequest);
      callback(resp);
      return;
    }

    // 检查是否已存在重复申请
    auto result = dbClient->execSqlSync(
        "SELECT apply_id FROM club_member_apply WHERE user_id = ? AND club_id "
        "= ? AND status = ?",
        clubMember.user_id, clubMember.club_id, "pending");

    if (!result.empty()) {
      response["error"] = "重复申请，您已提交过申请，正在等待审核";
      auto resp = HttpResponse::newHttpJsonResponse(response);
      resp->setStatusCode(k400BadRequest);
      callback(resp);
      return;
    }

    // 插入申请记录到 club_member_apply 表
    dbClient->execSqlSync("INSERT INTO club_member_apply (user_id, club_id, "
                          "apply_date, status) VALUES (?, ?, NOW(), ?)",
                          clubMember.user_id, clubMember.club_id, "pending");

    response["message"] = "申请已提交，等待审核";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k200OK);
    callback(resp);
  } catch (const std::runtime_error &e) {
    response["error"] = e.what();
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k400BadRequest);
    callback(resp);
  } catch (const drogon::orm::DrogonDbException &e) {
    response["error"] = "数据库错误，无法提交申请";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k500InternalServerError);
    callback(resp);
  }
}

// 审核加入申请
void ClubMemberController::approve(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) const {
  auto dbClient = drogon::app().getDbClient();
  Json::Value response;

  auto json = req->getJsonObject();
  if (!json || !json->isMember("apply_id") || !json->isMember("status")) {
    response["error"] = "缺少必备字段: apply_id 或 status";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k400BadRequest);
    callback(resp);
    return;
  }

  int apply_id = (*json)["apply_id"].asInt();
  std::string status = (*json)["status"].asString();

  try {
    // 查询申请记录
    auto result =
        dbClient->execSqlSync("SELECT user_id, club_id FROM club_member_apply "
                              "WHERE apply_id = ? AND status = 'pending'",
                              apply_id);

    if (result.empty()) {
      response["error"] = "未找到待审核的申请记录";
      auto resp = HttpResponse::newHttpJsonResponse(response);
      resp->setStatusCode(k404NotFound);
      callback(resp);
      return;
    }

    int user_id = result[0]["user_id"].as<int>();
    int club_id = result[0]["club_id"].as<int>();

    // 更新申请状态
    dbClient->execSqlSync(
        "UPDATE club_member_apply SET status = ? WHERE apply_id = ?", status,
        apply_id);

    // 如果审核通过，将用户加入 club_member 表
    if (status == "approved") {
      dbClient->execSqlSync(
          "INSERT INTO club_member (user_id, club_id, join_date, member_role) "
          "VALUES (?, ?, NOW(), '社员')",
          user_id, club_id);
    }

    response["message"] = "申请状态已更新";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k200OK);
    callback(resp);
  } catch (const drogon::orm::DrogonDbException &e) {
    response["error"] = "数据库错误，无法更新申请状态";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k500InternalServerError);
    callback(resp);
  }
}

// 移除成员
void ClubMemberController::remove(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) const {
  auto dbClient = drogon::app().getDbClient();
  auto json = req->getJsonObject();
  Json::Value response;

  // 检查请求体是否包含必要字段
  if (!json || !json->isMember("member_id")) {
    response["error"] = "缺少必备字段: member_id";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k400BadRequest);
    callback(resp);
    return;
  }

  auto member_id = (*json)["member_id"].asInt();

  try {
    // 删除成员记录
    dbClient->execSqlSync("DELETE FROM club_member WHERE member_id = ?",
                          member_id);

    response["message"] = "成员已移除";
  } catch (const drogon::orm::DrogonDbException &e) {
    response["error"] = "数据库错误，无法移除成员";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k500InternalServerError);
    callback(resp);
    return;
  }

  auto resp = HttpResponse::newHttpJsonResponse(response);
  resp->setStatusCode(drogon::k200OK); // 成功返回 200 OK
  callback(resp);
}

// 获取社团成员列表
void ClubMemberController::list(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    int club_id) const {
  auto dbClient = drogon::app().getDbClient();
  Json::Value response;

  try {
    // 查询社团成员列表，包含 email 和 phone 字段
    auto result =
        dbClient->execSqlSync("SELECT user.user_id, user.username, user.email, user.phone, "
                              "club_member.member_role FROM club_member "
                              "JOIN user ON club_member.user_id = user.user_id "
                              "WHERE club_member.club_id = ?",
                              club_id);

    Json::Value members(Json::arrayValue);
    for (const auto &row : result) {
      Json::Value member;
      member["user_id"] = row["user_id"].as<int>();
      member["username"] = row["username"].as<std::string>();
      member["email"] = row["email"].isNull() ? "" : row["email"].as<std::string>();
      member["phone"] = row["phone"].isNull() ? "" : row["phone"].as<std::string>();
      member["member_role"] = row["member_role"].as<std::string>();
      members.append(member);
    }

    response["members"] = members;
    response["message"] = "社团成员列表获取成功";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k200OK);
    callback(resp);
  } catch (const drogon::orm::DrogonDbException &e) {
    response["error"] = "数据库错误，无法获取成员列表";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k500InternalServerError);
    callback(resp);
  }
}

// 获取所有申请列表
void ClubMemberController::getAllApplications(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) const {
  auto dbClient = drogon::app().getDbClient();
  Json::Value response;

  // 从 Cookie 中获取当前登录用户的 user_id
  auto userIdCookie = req->getCookie("user_id");
  if (userIdCookie.empty()) {
    response["error"] = "未登录";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k401Unauthorized); // 未授权
    callback(resp);
    return;
  }

  int user_id = std::stoi(userIdCookie);

  try {
    // 查询用户作为社长的所有社团
    auto clubResult = dbClient->execSqlSync(
        "SELECT club_id, club_name FROM club WHERE founder_id = ?", user_id);

    if (clubResult.empty()) {
      response["error"] = "您没有管理的社团";
      auto resp = HttpResponse::newHttpJsonResponse(response);
      resp->setStatusCode(drogon::k200OK); // 未找到
      callback(resp);
      return;
    }

    Json::Value applications(Json::arrayValue);

    // 遍历每个社团，查询其申请列表
    for (const auto &clubRow : clubResult) {
      int club_id = clubRow["club_id"].as<int>();
      std::string club_name = clubRow["club_name"].as<std::string>();

      // 联表查询申请记录和用户名
      auto applicationResult =
          dbClient->execSqlSync("SELECT a.apply_id, a.user_id, u.username AS "
                                "user_name, a.apply_date, a.status "
                                "FROM club_member_apply a "
                                "JOIN user u ON a.user_id = u.user_id "
                                "WHERE a.club_id = ?",
                                club_id);

      for (const auto &applicationRow : applicationResult) {
        Json::Value application;
        application["club_id"] = club_id;
        application["club_name"] = club_name;
        application["apply_id"] = applicationRow["apply_id"].as<int>();
        application["user_id"] = applicationRow["user_id"].as<int>();
        application["user_name"] =
            applicationRow["user_name"].as<std::string>();
        application["apply_date"] =
            applicationRow["apply_date"].as<std::string>();
        application["status"] = applicationRow["status"].as<std::string>();
        applications.append(application);
      }
    }

    response["applications"] = applications;
    response["message"] = "申请列表获取成功";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k200OK); // 成功返回 200 OK
    callback(resp);
  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Database error: " << e.base().what();
    response["error"] = "数据库错误，无法获取申请列表";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k500InternalServerError); // 服务器内部错误
    callback(resp);
  }
}