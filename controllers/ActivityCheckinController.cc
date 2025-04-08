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
        // 检查用户是否已报名该活动
        auto registrationResult = dbClient->execSqlSync(
            "SELECT COUNT(*) AS count FROM activity_registration WHERE user_id = ? AND activity_id = ?",
            user_id, activity_id);

        if (registrationResult[0]["count"].as<int>() == 0) {
            response["error"] = "您尚未报名该活动，无法签到";
            auto resp = HttpResponse::newHttpJsonResponse(response);
            resp->setStatusCode(k403Forbidden);
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
            resp->setStatusCode(k400BadRequest);
            callback(resp);
            return;
        }

        // 插入签到记录
        dbClient->execSqlSync(
            "INSERT INTO activity_checkin (user_id, activity_id, checkin_time) VALUES (?, ?, NOW())",
            user_id, activity_id);

        response["message"] = "签到成功";
    } catch (const drogon::orm::DrogonDbException &e) {
        response["error"] = "数据库错误，无法签到";
    }

    auto resp = HttpResponse::newHttpJsonResponse(response);
    callback(resp);
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
  callback(resp);
}