#include "ClubApprovalController.h"
#include <drogon/HttpResponse.h>
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
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k401Unauthorized);
    callback(resp);
    return;
  }

  int user_id = std::stoi(userIdCookie);

  // 获取请求体中的 club_name 和 club_introduction
  auto json = req->getJsonObject();
  if (!json || !json->isMember("club_name") ||
      !json->isMember("club_introduction")) {
    response["error"] = "缺少必需字段: club_name 或 club_introduction";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k400BadRequest);
    callback(resp);
    return;
  }

  std::string club_name = (*json)["club_name"].asString();
  std::string club_introduction = (*json)["club_introduction"].asString();

  try {
    // 插入审批记录到 club_approval 表
    dbClient->execSqlSync("INSERT INTO club_approval (club_name, applicant_id, "
                          "approval_status, approval_opinion) "
                          "VALUES (?, ?, '待审核', NULL)",
                          club_name, user_id);

    response["message"] = "审批申请已提交，等待管理员审批";
  } catch (const drogon::orm::DrogonDbException &e) {
    response["error"] = "数据库错误，无法提交审批";
  }

  auto resp = HttpResponse::newHttpJsonResponse(response);
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
    auto resp = HttpResponse::newHttpJsonResponse(response);
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
      auto resp = HttpResponse::newHttpJsonResponse(response);
      resp->setStatusCode(k403Forbidden);
      callback(resp);
      return;
    }

    // 获取请求体中的审批状态和意见
    auto json = req->getJsonObject();
    if (!json || !json->isMember("approval_status") ||
        !json->isMember("approval_opinion")) {
      response["error"] = "缺少必需字段: approval_status 或 approval_opinion";
      auto resp = HttpResponse::newHttpJsonResponse(response);
      resp->setStatusCode(k400BadRequest);
      callback(resp);
      return;
    }

    std::string approval_status = (*json)["approval_status"].asString();
    std::string approval_opinion = (*json)["approval_opinion"].asString();

    // 查询审批记录，获取 club_name 和申请用户的 user_id
    auto approvalResult =
        dbClient->execSqlSync("SELECT club_name, applicant_id FROM "
                              "club_approval WHERE approval_id = ?",
                              approvalId);

    if (approvalResult.empty()) {
      response["error"] = "未找到对应的审批记录";
      auto resp = HttpResponse::newHttpJsonResponse(response);
      resp->setStatusCode(k404NotFound);
      callback(resp);
      return;
    }

    std::string club_name = approvalResult[0]["club_name"].as<std::string>();
    int applicant_id = approvalResult[0]["applicant_id"].as<int>();

    // 如果审批通过，创建社团并设置申请用户为社长
    if (approval_status == "通过") {
      // 插入社团记录到 club 表
      auto clubResult = dbClient->execSqlSync(
          "INSERT INTO club (club_name, founder_id) VALUES (?, ?)", club_name,
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
    }

    // 更新审批记录
    dbClient->execSqlSync(
        "UPDATE club_approval SET approval_status = ?, approval_opinion = ?, "
        "approval_time = NOW() WHERE approval_id = ?",
        approval_status, approval_opinion, approvalId);

    response["message"] = "审批成功";
  } catch (const drogon::orm::DrogonDbException &e) {
    response["error"] = "数据库错误，无法完成审批";
  }

  auto resp = HttpResponse::newHttpJsonResponse(response);
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
      query = "SELECT approval_id, club_name, applicant_id, approval_status, "
              "approval_opinion, approval_time FROM club_approval";
    } else {
      // 如果是普通用户，只查询自己的审批记录
      query = "SELECT approval_id, club_name, applicant_id, approval_status, "
              "approval_opinion, approval_time FROM club_approval WHERE "
              "applicant_id = ?";
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
    response["error"] = "数据库错误，无法获取审批记录";
  }

  auto resp = HttpResponse::newHttpJsonResponse(response);
  callback(resp);
}