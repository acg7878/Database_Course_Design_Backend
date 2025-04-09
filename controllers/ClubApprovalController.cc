#include "ClubApprovalController.h"
#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>
#include <drogon/orm/Exception.h>

void ClubApprovalController::submitApproval(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) const {
  auto dbClient = drogon::app().getDbClient();
  Json::Value response;

  // 从 Cookie 中获取当前登录用户的 user_id
  auto userIdCookie = req->getCookie("user_id");
  if (userIdCookie.empty()) {
    response["error"] = "未登录";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k401Unauthorized);
    callback(resp);
    return;
  }

  int user_id = std::stoi(userIdCookie);

  // 获取请求体中的字段
  auto json = req->getJsonObject();
  if (!json || !json->isMember("club_name") ||
      !json->isMember("club_introduction") || !json->isMember("contact_info") ||
      !json->isMember("activity_venue")) {
    response["error"] = "缺少必需字段: club_name, club_introduction, "
                        "contact_info 或 activity_venue";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k400BadRequest);
    callback(resp);
    return;
  }

  std::string club_name = (*json)["club_name"].asString();
  std::string club_introduction = (*json)["club_introduction"].asString();
  std::string contact_info = (*json)["contact_info"].asString();
  std::string activity_venue = (*json)["activity_venue"].asString();

  // 检查字段的合法性
  if (club_name.empty() || club_name.size() > 100) {
    response["error"] = "club_name 不能为空且长度不能超过 100 个字符";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k400BadRequest);
    callback(resp);
    return;
  }

  if (contact_info.empty() || contact_info.size() > 100) {
    response["error"] = "contact_info 不能为空且长度不能超过 100 个字符";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k400BadRequest);
    callback(resp);
    return;
  }

  if (activity_venue.empty() || activity_venue.size() > 100) {
    response["error"] = "activity_venue 不能为空且长度不能超过 100 个字符";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k400BadRequest);
    callback(resp);
    return;
  }

  try {
    // 插入审批记录到 club_approval 表
    dbClient->execSqlSync(
        "INSERT INTO club_approval (club_name, club_introduction, "
        "contact_info, activity_venue, applicant_id, approval_status, "
        "approval_opinion, approval_time) "
        "VALUES (?, ?, ?, ?, ?, '待审核', NULL, NOW())",
        club_name, club_introduction, contact_info, activity_venue, user_id);

    response["message"] = "审批申请已提交，等待管理员审批";
  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Database error: " << e.base().what();
    response["error"] = "数据库错误，无法提交审批";
  }

  auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
  resp->setStatusCode(drogon::k200OK);
  callback(resp);
}

void ClubApprovalController::approveClub(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    int approvalId) const {
  auto dbClient = drogon::app().getDbClient();
  Json::Value response;

  // 从 Cookie 中获取当前登录管理员的 user_id
  auto userIdCookie = req->getCookie("user_id");
  if (userIdCookie.empty()) {
    response["error"] = "未登录";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k401Unauthorized);
    callback(resp);
    return;
  }

  int admin_id = std::stoi(userIdCookie);

  try {
    // 验证用户是否为管理员
    auto userResult = dbClient->execSqlSync(
        "SELECT user_type FROM user WHERE user_id = ?", admin_id);

    if (userResult.empty() ||
        userResult[0]["user_type"].as<std::string>() != "管理员") {
      response["error"] = "无权限操作，只有管理员可以审批";
      auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
      resp->setStatusCode(k403Forbidden);
      callback(resp);
      return;
    }

    // 获取请求体中的审批状态和意见
    auto json = req->getJsonObject();
    if (!json || !json->isMember("approval_status") ||
        !json->isMember("approval_opinion")) {
      response["error"] = "缺少必需字段: approval_status 或 approval_opinion";
      auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
      resp->setStatusCode(k400BadRequest);
      callback(resp);
      return;
    }

    std::string approval_status = (*json)["approval_status"].asString();
    std::string approval_opinion = (*json)["approval_opinion"].asString();

    // 查询审批记录，获取所有相关字段
    auto approvalResult =
        dbClient->execSqlSync("SELECT club_name, club_introduction, "
                              "contact_info, activity_venue, applicant_id "
                              "FROM club_approval WHERE approval_id = ?",
                              approvalId);

    if (approvalResult.empty()) {
      response["error"] = "未找到对应的审批记录";
      auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
      resp->setStatusCode(k404NotFound);
      callback(resp);
      return;
    }

    std::string club_name = approvalResult[0]["club_name"].as<std::string>();
    std::string club_introduction =
        approvalResult[0]["club_introduction"].as<std::string>();
    std::string contact_info =
        approvalResult[0]["contact_info"].as<std::string>();
    std::string activity_venue =
        approvalResult[0]["activity_venue"].as<std::string>();
    int applicant_id = approvalResult[0]["applicant_id"].as<int>();

    // 如果审批通过，创建社团并设置申请用户为社长
    if (approval_status == "通过") {
      // 插入社团记录到 club 表
      auto clubResult = dbClient->execSqlSync(
          "INSERT INTO club (club_name, club_introduction, contact_info, "
          "activity_venue, founder_id) "
          "VALUES (?, ?, ?, ?, ?)",
          club_name, club_introduction, contact_info, activity_venue,
          applicant_id);

      int club_id = clubResult.insertId();
      // 检查申请用户是否为管理员
      auto applicantResult = dbClient->execSqlSync(
          "SELECT user_type FROM user WHERE user_id = ?", applicant_id);
      if (!applicantResult.empty() &&
          applicantResult[0]["user_type"].as<std::string>() != "管理员") {
        // 更新 user 表中的 user_type 为 '社长'
        dbClient->execSqlSync(
            "UPDATE user SET user_type = '社长' WHERE user_id = ?",
            applicant_id);
      }
      // 将申请者添加到社团成员里并设置为社长
      dbClient->execSqlSync("INSERT INTO club_member (user_id, club_id, "
                            "join_date, member_role) "
                            "VALUES (?, ?, NOW(), '社长')",
                            applicant_id, club_id);
    }

    // 更新审批记录
    dbClient->execSqlSync("UPDATE club_approval SET approval_status = ?, "
                          "approval_opinion = ? WHERE approval_id = ?",
                          approval_status, approval_opinion, approvalId);

    response["message"] = "审批成功";
  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Database error: " << e.base().what();
    response["error"] = "数据库错误，无法完成审批";
  }

  auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
  resp->setStatusCode(drogon::k200OK);
  callback(resp);
}

void ClubApprovalController::getApprovalList(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) const {
  auto dbClient = drogon::app().getDbClient();
  Json::Value response;

  // 从 Cookie 中获取当前登录用户的 user_id
  auto userIdCookie = req->getCookie("user_id");
  if (userIdCookie.empty()) {
    response["error"] = "未登录";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k401Unauthorized);
    callback(resp);
    return;
  }

  int user_id = std::stoi(userIdCookie);

  try {
    // 查询用户类型
    auto userResult = dbClient->execSqlSync(
        "SELECT user_type FROM user WHERE user_id = ?", user_id);

    if (userResult.empty()) {
      response["error"] = "用户不存在";
      auto resp = HttpResponse::newHttpJsonResponse(response);
      resp->setStatusCode(k404NotFound);
      callback(resp);
      return;
    }

    std::string user_type = userResult[0]["user_type"].as<std::string>();

    // 如果是管理员，查询所有审批记录
    std::string query;
    if (user_type == "管理员") {
      query = "SELECT ca.approval_id, ca.club_name, ca.applicant_id, "
              "u.username AS applicant_name, "
              "ca.approval_status, ca.approval_opinion, ca.approval_time "
              "FROM club_approval ca "
              "JOIN user u ON ca.applicant_id = u.user_id";
    } else {
      // 如果是普通用户，只查询自己的审批记录
      query = "SELECT ca.approval_id, ca.club_name, ca.applicant_id, "
              "u.username AS applicant_name, "
              "ca.approval_status, ca.approval_opinion, ca.approval_time "
              "FROM club_approval ca "
              "JOIN user u ON ca.applicant_id = u.user_id "
              "WHERE ca.applicant_id = ?";
    }

    // 执行查询
    auto result = (user_type == "管理员")
                      ? dbClient->execSqlSync(query)
                      : dbClient->execSqlSync(query, user_id);

    Json::Value approvals(Json::arrayValue);
    for (const auto &row : result) {
      Json::Value approval;
      approval["approval_id"] = row["approval_id"].as<int>();
      approval["club_name"] = row["club_name"].as<std::string>();
      approval["applicant_id"] = row["applicant_id"].as<int>();
      approval["applicant_name"] = row["applicant_name"].as<std::string>();
      approval["approval_status"] = row["approval_status"].as<std::string>();
      approval["approval_opinion"] =
          row["approval_opinion"].isNull()
              ? ""
              : row["approval_opinion"].as<std::string>();
      approval["approval_time"] = row["approval_time"].isNull()
                                      ? ""
                                      : row["approval_time"].as<std::string>();
      approvals.append(approval);
    }

    response["approvals"] = approvals;
  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Database error: " << e.base().what();
    response["error"] = "数据库错误，无法获取审批记录";
  }

  auto resp = HttpResponse::newHttpJsonResponse(response);
  resp->setStatusCode(drogon::k200OK);
  callback(resp);
}