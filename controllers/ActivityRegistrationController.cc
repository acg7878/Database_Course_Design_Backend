#include "ActivityRegistrationController.h"
#include <drogon/HttpResponse.h>
#include <drogon/orm/Exception.h>

void ActivityRegistrationController::registerActivity(
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

  // 获取请求体中的 activity_id
  auto json = req->getJsonObject();
  if (!json || !json->isMember("activity_id")) {
    response["error"] = "缺少必需字段: activity_id";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k400BadRequest);
    callback(resp);
    return;
  }

  int activity_id = (*json)["activity_id"].asInt();

  try {
    // 查询是否已有报名记录
    auto result = dbClient->execSqlSync(
        "SELECT registration_status FROM activity_registration WHERE user_id = "
        "? AND activity_id = ?",
        user_id, activity_id);

    if (!result.empty()) {
      std::string registration_status =
          result[0]["registration_status"].as<std::string>();

      if (registration_status == "rejected") {
        response["error"] = "您的报名已被拒绝，无法再次报名";
        auto resp = HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
      } else if (registration_status == "accepted") {
        response["message"] = "您已报名成功，无需再次报名";
        auto resp = HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k200OK);
        callback(resp);
        return;
      } else if (registration_status == "pending") {
        response["message"] = "您的报名已在审核中，无需重复报名";
        auto resp = HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k200OK);
        callback(resp);
        return;
      } 
    }

    // 如果没有记录，插入新报名记录，状态为 pending
    dbClient->execSqlSync(
        "INSERT INTO activity_registration (user_id, activity_id, "
        "registration_date, registration_status) VALUES (?, ?, NOW(), "
        "'pending')",
        user_id, activity_id);

    response["message"] = "报名成功，等待审核";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k200OK);
    callback(resp);
  } catch (const drogon::orm::DrogonDbException &e) {
    response["error"] = "数据库错误，无法报名";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k500InternalServerError);
    callback(resp);
  }
}

void ActivityRegistrationController::cancelRegistration(
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

  // 获取请求体中的 activity_id
  auto json = req->getJsonObject();
  if (!json || !json->isMember("activity_id")) {
    response["error"] = "缺少必需字段: activity_id";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k400BadRequest);
    callback(resp);
    return;
  }

  int activity_id = (*json)["activity_id"].asInt();

  try {
    // 查询报名记录
    auto result = dbClient->execSqlSync(
        "SELECT registration_status FROM activity_registration WHERE user_id = "
        "? AND activity_id = ?",
        user_id, activity_id);

    if (result.empty()) {
      response["error"] = "未找到报名记录";
      auto resp = HttpResponse::newHttpJsonResponse(response);
      resp->setStatusCode(k404NotFound);
      callback(resp);
      return;
    }

    std::string registration_status =
        result[0]["registration_status"].as<std::string>();

    // 检查报名状态
    if (registration_status == "pending" || registration_status == "accepted") {
      // 更新报名状态为 cancel
      auto updateResult = dbClient->execSqlSync(
          "UPDATE activity_registration SET registration_status = 'cancel' "
          "WHERE user_id = ? AND activity_id = ?",
          user_id, activity_id);

      response["message"] = "报名已取消";
      auto resp = HttpResponse::newHttpJsonResponse(response);
      resp->setStatusCode(k200OK);
      callback(resp);
    } else {
      if(registration_status == "rejected")response["error"] = "当前状态无法取消报名,你已经被社长拒绝参加了";
      if(registration_status == "cancel")response["error"] = "当前状态无法取消报名,你已经自己取消过报名了";
      auto resp = HttpResponse::newHttpJsonResponse(response);
      resp->setStatusCode(k400BadRequest);
      callback(resp);
    }
  } catch (const drogon::orm::DrogonDbException &e) {
    response["error"] = "数据库错误，无法取消报名";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k500InternalServerError);
    callback(resp);
  }
}

void ActivityRegistrationController::getRegistrationList(
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
    // 查询用户是否是社长
    auto clubResult = dbClient->execSqlSync(
        "SELECT c.club_id FROM club c WHERE c.founder_id = ?", user_id);

    Json::Value registrations(Json::arrayValue);

    if (!clubResult.empty()) {
      // 当前用户是社长，查询其所有社团的报名记录
      for (const auto &clubRow : clubResult) {
        int club_id = clubRow["club_id"].as<int>();

        auto result = dbClient->execSqlSync(
            "SELECT r.registration_id, r.user_id, r.activity_id, "
            "r.registration_date, r.payment_status "
            "FROM activity_registration r "
            "JOIN club_activity a ON r.activity_id = a.activity_id "
            "WHERE a.club_id = ?",
            club_id);

        for (const auto &row : result) {
          Json::Value registration;
          registration["registration_id"] = row["registration_id"].as<int>();
          registration["user_id"] = row["user_id"].as<int>();
          registration["activity_id"] = row["activity_id"].as<int>();
          registration["registration_date"] =
              row["registration_date"].as<std::string>();
          registration["payment_status"] =
              row["payment_status"].as<std::string>();
          registrations.append(registration);
        }
      }
    } else {
      // 当前用户是普通社员，只查询自己的报名记录
      auto result =
          dbClient->execSqlSync("SELECT registration_id, user_id, activity_id, "
                                "registration_date, payment_status "
                                "FROM activity_registration WHERE user_id = ?",
                                user_id);

      for (const auto &row : result) {
        Json::Value registration;
        registration["registration_id"] = row["registration_id"].as<int>();
        registration["user_id"] = row["user_id"].as<int>();
        registration["activity_id"] = row["activity_id"].as<int>();
        registration["registration_date"] =
            row["registration_date"].as<std::string>();
        registration["payment_status"] =
            row["payment_status"].as<std::string>();
        registrations.append(registration);
      }
    }

    response["registrations"] = registrations;
    response["message"] = "报名记录获取成功";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k200OK); // 成功返回 200 OK
    callback(resp);
  } catch (const drogon::orm::DrogonDbException &e) {
    response["error"] = "数据库错误，无法获取报名列表";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k500InternalServerError); // 服务器内部错误
    callback(resp);
  }
}


void ActivityRegistrationController::reviewRegistration(
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

    // 获取请求体中的 registration_id 和 registration_status
    auto json = req->getJsonObject();
    if (!json || !json->isMember("registration_id") || !json->isMember("registration_status")) {
        response["error"] = "缺少必需字段: registration_id 或 registration_status";
        auto resp = HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k400BadRequest); // 错误请求
        callback(resp);
        return;
    }

    int registration_id = (*json)["registration_id"].asInt();
    std::string registration_status = (*json)["registration_status"].asString();

    // 检查状态是否合法
    if (registration_status != "accepted" && registration_status != "rejected") {
        response["error"] = "无效的状态值: registration_status 必须是 'accepted' 或 'rejected'";
        auto resp = HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k400BadRequest); // 错误请求
        callback(resp);
        return;
    }

    try {
        // 更新报名状态
        auto result = dbClient->execSqlSync(
            "UPDATE activity_registration SET registration_status = ? WHERE registration_id = ?",
            registration_status, registration_id);

        if (result.affectedRows() == 0) {
            response["error"] = "未找到对应的报名记录";
            auto resp = HttpResponse::newHttpJsonResponse(response);
            resp->setStatusCode(k404NotFound); // 未找到
            callback(resp);
            return;
        }

        response["message"] = "报名状态更新成功";
        auto resp = HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k200OK); // 成功
        callback(resp);
    } catch (const drogon::orm::DrogonDbException &e) {
        LOG_ERROR << "Database error: " << e.base().what();
        response["error"] = "数据库错误，无法更新报名状态";
        auto resp = HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k500InternalServerError); // 服务器内部错误
        callback(resp);
    }
}

void ActivityRegistrationController::getApprovedRegistrationsByUser(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) const {
    auto dbClient = drogon::app().getDbClient();
    Json::Value response;

    // 从请求体中获取 user_id
    auto json = req->getJsonObject();
    if (!json || !json->isMember("user_id")) {
        response["error"] = "缺少必需字段: user_id";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k400BadRequest); // 错误请求
        callback(resp);
        return;
    }

    int userId = (*json)["user_id"].asInt();

    try {
        // 查询用户报名且报名状态为 accepted 的所有活动，并返回 payment_status
        auto result = dbClient->execSqlSync(
            "SELECT a.activity_id, a.activity_title, a.activity_time, a.activity_location, a.activity_description, "
            "r.payment_status "
            "FROM activity_registration r "
            "JOIN club_activity a ON r.activity_id = a.activity_id "
            "WHERE r.user_id = ? AND r.registration_status = 'accepted'",
            userId);

        if (result.empty()) {
            response["error"] = "未找到任何已报名且通过的活动";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
            resp->setStatusCode(k404NotFound); // 未找到
            callback(resp);
            return;
        }

        Json::Value approvedActivities(Json::arrayValue);

        // 遍历查询结果
        for (const auto &row : result) {
            Json::Value activity;
            activity["activity_id"] = row["activity_id"].as<int>();
            activity["activity_title"] = row["activity_title"].as<std::string>();
            activity["activity_time"] = row["activity_time"].as<std::string>();
            activity["activity_location"] = row["activity_location"].as<std::string>();
            activity["activity_description"] = row["activity_description"].as<std::string>();
            activity["payment_status"] = row["payment_status"].as<std::string>(); // 直接返回数据库中的值

            approvedActivities.append(activity);
        }

        response["approved_activities"] = approvedActivities;
        response["message"] = "已报名且通过的活动列表获取成功";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k200OK); // 成功返回 200 OK
        callback(resp);
    } catch (const drogon::orm::DrogonDbException &e) {
        LOG_ERROR << "Database error: " << e.base().what();
        response["error"] = "数据库错误，无法获取活动列表";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k500InternalServerError); // 服务器内部错误
        callback(resp);
    }
}

void ActivityRegistrationController::setPaymentStatus(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) const {
    auto dbClient = drogon::app().getDbClient();
    Json::Value response;

    // 从请求体中获取 registration_id 和 payment_status
    auto json = req->getJsonObject();
    if (!json || !json->isMember("registration_id") || !json->isMember("payment_status")) {
        response["error"] = "缺少必需字段: registration_id 或 payment_status";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k400BadRequest); // 错误请求
        callback(resp);
        return;
    }

    int registrationId = (*json)["registration_id"].asInt();
    std::string paymentStatus = (*json)["payment_status"].asString();

    // 验证 payment_status 是否为合法值
    if (paymentStatus != "已缴费" && paymentStatus != "未缴费") {
        response["error"] = "无效的 payment_status 值，必须为 '已缴费' 或 '未缴费'";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k400BadRequest); // 错误请求
        callback(resp);
        return;
    }

    try {
        // 查询当前报名记录的缴费状态
        auto result = dbClient->execSqlSync(
            "SELECT payment_status FROM activity_registration WHERE registration_id = ?",
            registrationId);

        if (result.empty()) {
            response["error"] = "未找到对应的报名记录";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
            resp->setStatusCode(k404NotFound); // 未找到
            callback(resp);
            return;
        }

        std::string currentStatus = result[0]["payment_status"].as<std::string>();

        // 如果当前状态已经是 "已缴费"，且请求的状态也是 "已缴费"，则不允许重复设置
        if (currentStatus == "已缴费" && paymentStatus == "已缴费") {
            response["error"] = "已缴费，无法重复缴费";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
            resp->setStatusCode(k400BadRequest); // 错误请求
            callback(resp);
            return;
        }

        // 更新报名记录的缴费状态
        dbClient->execSqlSync(
            "UPDATE activity_registration SET payment_status = ? WHERE registration_id = ?",
            paymentStatus, registrationId);

        response["message"] = "缴费状态更新成功";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k200OK); // 成功返回 200 OK
        callback(resp);
    } catch (const drogon::orm::DrogonDbException &e) {
        LOG_ERROR << "Database error: " << e.base().what();
        response["error"] = "数据库错误，无法更新缴费状态";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k500InternalServerError); // 服务器内部错误
        callback(resp);
    }
}