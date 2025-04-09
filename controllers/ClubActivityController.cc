#include "ClubActivityController.h"
#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>
#include <drogon/orm/Exception.h>

// 创建活动
void ClubActivityController::createActivity(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    ClubActivity activity) const {
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

    try {
        // 验证用户是否是社团的创始人
        auto roleResult = dbClient->execSqlSync(
            "SELECT founder_id FROM club WHERE club_id = ?",
            activity.club_id);

        if (roleResult.empty() || roleResult[0]["founder_id"].as<int>() != user_id) {
            response["error"] = "无权限操作，只有社团创始人可以创建活动";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
            resp->setStatusCode(k403Forbidden);
            callback(resp);
            return;
        }

        // 检查活动标题是否已存在（同一社团内不能有重复标题）
        auto result = dbClient->execSqlSync(
            "SELECT COUNT(*) AS count FROM club_activity WHERE club_id = ? AND activity_title = ?",
            activity.club_id,
            activity.activity_title);
        if (result[0]["count"].as<int>() > 0) {
            response["error"] = "活动标题已存在，创建失败";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
            resp->setStatusCode(k400BadRequest);
            callback(resp);
            return;
        }

        // 插入活动数据到数据库，使用 NOW() 设置发布时间
        dbClient->execSqlSync(
            "INSERT INTO club_activity (club_id, activity_title, activity_time, activity_location, registration_method, activity_description, publish_time) "
            "VALUES (?, ?, ?, ?, ?, ?, NOW())",
            activity.club_id,
            activity.activity_title,
            activity.activity_time.toDbStringLocal(),
            activity.activity_location,
            activity.registration_method,
            activity.activity_description);

        response["message"] = "活动创建成功";
    } catch (const drogon::orm::DrogonDbException &e) {
        response["error"] = "数据库错误，无法创建活动";
    }

    // 返回响应
    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(drogon::k200OK);
    callback(resp);
}

// 获取活动列表
void ClubActivityController::getActivityList(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback, int clubId) const {
    auto dbClient = drogon::app().getDbClient();
    Json::Value response;

    try {
        // 查询指定社团的所有活动
        auto result = dbClient->execSqlSync(
            "SELECT activity_id, activity_title, activity_time FROM club_activity "
            "WHERE club_id = ?",
            clubId);
        Json::Value activities(Json::arrayValue);

        for (const auto &row : result) {
            Json::Value activity;
            activity["activity_id"] = row["activity_id"].as<int>();
            activity["activity_title"] = row["activity_title"].as<std::string>();
            activity["activity_time"] = row["activity_time"].as<std::string>();
            activities.append(activity);
        }

        response["activities"] = activities;
        response["message"] = "活动列表获取成功";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k200OK); // 成功返回 200 OK
        callback(resp);
    } catch (const drogon::orm::DrogonDbException &e) {
        response["error"] = "数据库错误，无法获取活动列表";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k500InternalServerError); // 服务器内部错误
        callback(resp);
    }
}

// 获取活动详情
void ClubActivityController::getActivityDetail(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    int activityId) const {
    auto dbClient = drogon::app().getDbClient();
    Json::Value response;

    try {
        // 查询活动详情
        auto result = dbClient->execSqlSync(
            "SELECT * FROM club_activity WHERE activity_id = ?", activityId);

        if (!result.empty()) {
            response["activity_id"] = result[0]["activity_id"].as<int>();
            response["club_id"] = result[0]["club_id"].as<int>();
            response["activity_title"] =
                result[0]["activity_title"].as<std::string>();
            response["activity_time"] = result[0]["activity_time"].as<std::string>();
            response["activity_location"] =
                result[0]["activity_location"].as<std::string>();
            response["registration_method"] =
                result[0]["registration_method"].as<std::string>();
            response["activity_description"] =
                result[0]["activity_description"].as<std::string>();
            response["publish_time"] = result[0]["publish_time"].as<std::string>();

            auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
            resp->setStatusCode(k200OK); // 成功返回 200 OK
            callback(resp);
        } else {
            response["error"] = "活动不存在";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
            resp->setStatusCode(k404NotFound); // 未找到
            callback(resp);
        }
    } catch (const drogon::orm::DrogonDbException &e) {
        response["error"] = "数据库错误，无法获取活动详情";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k500InternalServerError); // 服务器内部错误
        callback(resp);
    }
}

void ClubActivityController::updateActivity(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    int activityId) const {
    auto dbClient = drogon::app().getDbClient();
    auto json = req->getJsonObject();
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

    if (!json) {
        response["error"] = "请求体格式错误，请使用 JSON";
        auto resp = HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    // 获取请求体中的字段
    std::string activity_title = (*json)["activity_title"].asString();
    std::string activity_time = (*json)["activity_time"].asString();
    std::string activity_location = (*json)["activity_location"].asString();
    std::string registration_method = (*json)["registration_method"].asString();
    std::string activity_description = (*json)["activity_description"].asString();

    try {
        // 验证用户是否是社团的创始人
        auto roleResult = dbClient->execSqlSync(
            "SELECT c.founder_id FROM club c "
            "JOIN club_activity a ON c.club_id = a.club_id "
            "WHERE a.activity_id = ?",
            activityId);

        if (roleResult.empty() || roleResult[0]["founder_id"].as<int>() != user_id) {
            response["error"] = "无权限操作，只有社团创始人可以更新活动";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
            resp->setStatusCode(k403Forbidden);
            callback(resp);
            return;
        }

        // 更新活动信息
        dbClient->execSqlSync(
            "UPDATE club_activity SET activity_title = ?, activity_time = ?, "
            "activity_location = ?, registration_method = ?, activity_description = ? "
            "WHERE activity_id = ?",
            activity_title, activity_time, activity_location, registration_method,
            activity_description, activityId);

        response["message"] = "活动更新成功";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(drogon::k200OK);
        callback(resp);
    } catch (const drogon::orm::DrogonDbException &e) {
        response["error"] = "数据库错误，无法更新活动";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
    }
}

// 删除活动
void ClubActivityController::deleteActivity(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    int activityId) const {
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

    try {
        // 验证用户是否是社团的创始人
        auto roleResult = dbClient->execSqlSync(
            "SELECT c.founder_id FROM club c "
            "JOIN club_activity a ON c.club_id = a.club_id "
            "WHERE a.activity_id = ?",
            activityId);

        if (roleResult.empty() || roleResult[0]["founder_id"].as<int>() != user_id) {
            response["error"] = "无权限操作，只有社团创始人可以删除活动";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
            resp->setStatusCode(k403Forbidden);
            callback(resp);
            return;
        }

        // 删除活动
        dbClient->execSqlSync("DELETE FROM club_activity WHERE activity_id = ?", activityId);
        response["message"] = "活动删除成功";
    } catch (const drogon::orm::DrogonDbException &e) {
        response["error"] = "数据库错误，无法删除活动";
    }

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(drogon::k200OK);
    callback(resp);
}
