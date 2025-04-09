#include "ClubController.h"
#include <drogon/HttpResponse.h>
#include <drogon/orm/Exception.h>

// 创建社团
void ClubController::create(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, Club club) const {
    auto dbClient = drogon::app().getDbClient();
    Json::Value response;

    try {
        // 检查社团名称是否已存在
        auto result = dbClient->execSqlSync("SELECT COUNT(*) AS count FROM club WHERE club_name = ?", club.club_name);
        if (result[0]["count"].as<int>() > 0) {
            response["error"] = "社团名称已存在，创建失败";
            auto resp = HttpResponse::newHttpJsonResponse(response);
            resp->setStatusCode(k400BadRequest);
            callback(resp);
            return;
        }

        // 插入社团数据到数据库
        dbClient->execSqlSync(
            "INSERT INTO club (club_name, club_introduction, contact_info, activity_venue, founder_id) VALUES (?, ?, ?, ?, ?)",
            club.club_name,
            club.club_introduction,
            club.contact_info,
            club.activity_venue,
            club.founder_id
        );

        response["message"] = "社团创建成功";
    } catch (const drogon::orm::DrogonDbException &e) {
        response["error"] = "数据库错误，无法创建社团";
    }

    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(drogon::k200OK);
    callback(resp);
}

// 获取社团列表
void ClubController::list(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) const {
    auto dbClient = drogon::app().getDbClient();
    Json::Value response;

    try {
        // 查询所有社团
        auto result = dbClient->execSqlSync("SELECT club_id, club_name, club_introduction FROM club");
        Json::Value clubs(Json::arrayValue);

        for (const auto &row : result) {
            Json::Value club;
            club["club_id"] = row["club_id"].as<int>();
            club["club_name"] = row["club_name"].as<std::string>();
            club["club_introduction"] = row["club_introduction"].as<std::string>();
            clubs.append(club);
        }

        response["clubs"] = clubs;
    } catch (const drogon::orm::DrogonDbException &e) {
        response["error"] = "数据库错误，无法获取社团列表";
    }

    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(drogon::k200OK);
    callback(resp);
}

// 获取社团详情
void ClubController::detail(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int club_id) const {
    auto dbClient = drogon::app().getDbClient();
    Json::Value response;

    try {
        // 查询社团详情
        auto result = dbClient->execSqlSync("SELECT * FROM club WHERE club_id = ?", club_id);

        if (!result.empty()) {
            response["club_id"] = result[0]["club_id"].as<int>();
            response["club_name"] = result[0]["club_name"].as<std::string>();
            response["club_introduction"] = result[0]["club_introduction"].as<std::string>();
            response["contact_info"] = result[0]["contact_info"].as<std::string>();
            response["activity_venue"] = result[0]["activity_venue"].as<std::string>();
            response["founder_id"] = result[0]["founder_id"].as<int>();
        } else {
            response["error"] = "社团不存在";
            auto resp = HttpResponse::newHttpJsonResponse(response);
            resp->setStatusCode(k404NotFound);
            callback(resp);
            return;
        }
    } catch (const drogon::orm::DrogonDbException &e) {
        response["error"] = "数据库错误，无法获取社团详情";
    }

    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(drogon::k200OK);
    callback(resp);
}

// 获取用户拥有的社团
void ClubController::ownedClubs(
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
        // 查询当前用户拥有的社团
        auto result = dbClient->execSqlSync(
            "SELECT club_id, club_name, club_introduction, contact_info, activity_venue "
            "FROM club WHERE founder_id = ?",
            user_id);

        Json::Value clubs(Json::arrayValue);
        for (const auto &row : result) {
            Json::Value club;
            club["club_id"] = row["club_id"].as<int>();
            club["club_name"] = row["club_name"].as<std::string>();
            clubs.append(club);
        }

        response["owned_clubs"] = clubs;
        response["message"] = "获取用户拥有的社团成功";
        auto resp = HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k200OK);
        callback(resp);
    } catch (const drogon::orm::DrogonDbException &e) {
        LOG_ERROR << "Database error: " << e.base().what();
        response["error"] = "数据库错误，无法获取用户拥有的社团";
        auto resp = HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
    }
}