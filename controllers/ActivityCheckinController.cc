#include "ActivityCheckinController.h"
#include <drogon/HttpResponse.h>
#include <drogon/orm/Exception.h>

void ActivityCheckinController::checkin(
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

    // 获取请求体中的 activity_id
    auto json = req->getJsonObject();
    if (!json || !json->isMember("activity_id")) {
        response["error"] = "缺少必需字段: activity_id";
        auto resp = HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k400BadRequest); // 错误请求
        callback(resp);
        return;
    }

    int activity_id = (*json)["activity_id"].asInt();

    try {
        // 检查用户是否已报名该活动
        auto registrationResult = dbClient->execSqlSync(
            "SELECT COUNT(*) AS count FROM activity_registration WHERE user_id = ? AND activity_id = ?",
            user_id, activity_id);

        if (registrationResult[0]["count"].as<int>() == 0) {
            response["error"] = "您尚未报名该活动，无法签到";
            auto resp = HttpResponse::newHttpJsonResponse(response);
            resp->setStatusCode(k403Forbidden); // 禁止访问
            callback(resp);
            return;
        }

        // 检查是否已经签到
        auto checkinResult = dbClient->execSqlSync(
            "SELECT COUNT(*) AS count FROM activity_checkin WHERE user_id = ? AND activity_id = ?",
            user_id, activity_id);

        if (checkinResult[0]["count"].as<int>() > 0) {
            response["error"] = "您已签到过该活动";
            auto resp = HttpResponse::newHttpJsonResponse(response);
            resp->setStatusCode(k400BadRequest); // 错误请求
            callback(resp);
            return;
        }

        // 插入签到记录
        dbClient->execSqlSync(
            "INSERT INTO activity_checkin (user_id, activity_id, checkin_time) VALUES (?, ?, NOW())",
            user_id, activity_id);

        response["message"] = "签到成功";
        auto resp = HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k200OK); // 成功返回 200 OK
        callback(resp);
    } catch (const drogon::orm::DrogonDbException &e) {
        response["error"] = "数据库错误，无法签到";
        auto resp = HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k500InternalServerError); // 服务器内部错误
        callback(resp);
    }
}

void ActivityCheckinController::getCheckinList(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    int activityId) const {
  auto dbClient = drogon::app().getDbClient();
  Json::Value response;

  try {
    // 查询签到记录
    auto result =
        dbClient->execSqlSync("SELECT checkin_id, user_id, checkin_time FROM "
                              "activity_checkin WHERE activity_id = ?",
                              activityId);

    Json::Value checkins(Json::arrayValue);
    for (const auto &row : result) {
      Json::Value checkin;
      checkin["checkin_id"] = row["checkin_id"].as<int>();
      checkin["user_id"] = row["user_id"].as<int>();
      checkin["checkin_time"] = row["checkin_time"].as<std::string>();
      checkins.append(checkin);
    }

    response["checkins"] = checkins;
  } catch (const drogon::orm::DrogonDbException &e) {
    response["error"] = "数据库错误，无法获取签到记录";
  }

  auto resp = HttpResponse::newHttpJsonResponse(response);
  resp->setStatusCode(k200OK);
  callback(resp);
}

void ActivityCheckinController::getRegisteredActivitiesByUser(
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
        // 查询用户报名的所有活动，且报名状态为 accepted
        auto result = dbClient->execSqlSync(
            "SELECT r.registration_id, a.activity_id, a.activity_title, a.activity_time, "
            "a.activity_location, a.activity_description, r.payment_status "
            "FROM activity_registration r "
            "JOIN club_activity a ON r.activity_id = a.activity_id "
            "WHERE r.user_id = ? AND r.registration_status = 'accepted'",
            userId);

        if (result.empty()) {
            response["error"] = "未报名任何活动/活动报名审核中";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
            resp->setStatusCode(k404NotFound); // 未找到
            callback(resp);
            return;
        }

        Json::Value registeredActivities(Json::arrayValue);

        // 遍历查询结果
        for (const auto &row : result) {
            Json::Value activity;
            int activityId = row["activity_id"].as<int>();

            activity["registration_id"] = row["registration_id"].as<int>();
            activity["activity_id"] = activityId;
            activity["activity_title"] = row["activity_title"].as<std::string>();
            activity["activity_time"] = row["activity_time"].as<std::string>();
            activity["activity_location"] = row["activity_location"].as<std::string>();
            activity["activity_description"] = row["activity_description"].as<std::string>();
            activity["payment_status"] = row["payment_status"].as<std::string>();

            // 查询签到表，检查是否有签到记录
            auto checkinResult = dbClient->execSqlSync(
                "SELECT COUNT(*) AS count FROM activity_checkin WHERE activity_id = ? AND user_id = ?",
                activityId, userId);

            // 如果有签到记录，设置 checkin_status 为 true，否则为 false
            activity["checkin_status"] = checkinResult[0]["count"].as<int>() > 0;

            registeredActivities.append(activity);
        }

        response["registered_activities"] = registeredActivities;
        response["message"] = "报名活动列表获取成功";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k200OK); // 成功返回 200 OK
        callback(resp);
    } catch (const drogon::orm::DrogonDbException &e) {
        LOG_ERROR << "Database error: " << e.base().what();
        response["error"] = "数据库错误，无法获取报名活动列表";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k500InternalServerError); // 服务器内部错误
        callback(resp);
    }
}