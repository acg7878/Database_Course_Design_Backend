#include "ClubMemberController.h"
#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>
#include <drogon/orm/Exception.h>

// 申请加入社团
void ClubMemberController::apply(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) const {
    auto dbClient = drogon::app().getDbClient();
    Json::Value response;

    try {
        // 使用自定义解析方法解析 ClubMember 对象
        auto clubMember = drogon::fromRequest<ClubMember>(*req);

        // 检查是否已存在重复申请
        auto result = dbClient->execSqlSync(
            "SELECT member_id FROM club_member WHERE user_id = ? AND club_id = ? AND member_status = ?",
            clubMember.user_id,
            clubMember.club_id,
            "pending"
        );

        if (!result.empty()) {
            response["error"] = "重复申请，您已提交过申请，正在等待审核";
            auto resp = HttpResponse::newHttpJsonResponse(response);
            resp->setStatusCode(k400BadRequest);
            callback(resp);
            return;
        }

        // 插入申请记录到数据库，使用 NOW() 自动填入 join_date
        dbClient->execSqlSync(
            "INSERT INTO club_member (user_id, club_id, join_date, member_status) VALUES (?, ?, NOW(), ?)",
            clubMember.user_id,
            clubMember.club_id,
            "pending"
        );
        
        response["message"] = "申请已提交，等待审核";
    } catch (const std::runtime_error &e) {
        response["error"] = e.what();
        auto resp = HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    } catch (const drogon::orm::DrogonDbException &e) {
        response["error"] = "数据库错误，无法提交申请";
        auto resp = HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
        return;
    }

    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(drogon::k200OK); // 成功返回 200 OK
    callback(resp);
}

// 审核加入申请
void ClubMemberController::approve(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) const {
    auto dbClient = drogon::app().getDbClient();
    Json::Value response;

    // 从 Cookie 中获取当前登录用户的 user_id
    auto user_id_cookie = req->getCookie("user_id");
    if (user_id_cookie.empty()) {
        response["error"] = "未登录，无法操作";
        auto resp = HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k401Unauthorized); // 未授权
        callback(resp);
        return;
    }

    int user_id = std::stoi(user_id_cookie); // 将 user_id 转换为整数

    // 检查请求体是否包含必要字段
    auto json = req->getJsonObject();
    if (!json || !json->isMember("user_id") || !json->isMember("club_id") || !json->isMember("status")) {
        response["error"] = "缺少必备字段: user_id、club_id 或 status";
        auto resp = HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k400BadRequest); // 错误请求
        callback(resp);
        return;
    }

    int target_user_id = (*json)["user_id"].asInt();
    int club_id = (*json)["club_id"].asInt();
    std::string status = (*json)["status"].asString();

    try {
        // 检查当前用户是否是该社团的社长
        auto roleResult = dbClient->execSqlSync(
            "SELECT member_role FROM club_member WHERE user_id = ? AND club_id = ?",
            user_id,
            club_id
        );

        if (roleResult.empty() || roleResult[0]["member_role"].as<std::string>() != "社长") {
            response["error"] = "无权限操作，只有社长可以审核申请";
            auto resp = HttpResponse::newHttpJsonResponse(response);
            resp->setStatusCode(k403Forbidden); // 禁止访问
            callback(resp);
            return;
        }

        // 检查是否存在待审核的申请记录
        auto result = dbClient->execSqlSync(
            "SELECT member_status FROM club_member WHERE user_id = ? AND club_id = ? AND member_status = ?",
            target_user_id,
            club_id,
            "pending"
        );

        if (result.empty()) {
            response["error"] = "未找到待审核的申请记录";
            auto resp = HttpResponse::newHttpJsonResponse(response);
            resp->setStatusCode(k400BadRequest);
            callback(resp);
            return;
        }

        // 更新申请状态
        dbClient->execSqlSync(
            "UPDATE club_member SET member_status = ? WHERE user_id = ? AND club_id = ?",
            status,
            target_user_id,
            club_id
        );

        response["message"] = "申请状态已更新";
    } catch (const drogon::orm::DrogonDbException &e) {
        response["error"] = "数据库错误，无法更新申请状态";
        auto resp = HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k500InternalServerError); // 服务器内部错误
        callback(resp);
        return;
    }

    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(drogon::k200OK); // 成功返回 200 OK
    callback(resp);
}

// 移除成员
void ClubMemberController::remove(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) const {
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
        dbClient->execSqlSync("DELETE FROM club_member WHERE member_id = ?", member_id);

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
void ClubMemberController::list(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int club_id) const {
    auto dbClient = drogon::app().getDbClient();
    Json::Value response;

    try {
        // 查询社团成员列表
        auto result = dbClient->execSqlSync(
            "SELECT user.user_id, user.username, club_member.member_status, club_member.member_role FROM club_member "
            "JOIN user ON club_member.user_id = user.user_id WHERE club_member.club_id = ?",
            club_id
        );

        Json::Value members(Json::arrayValue);
        for (const auto &row : result) {
            Json::Value member;
            member["user_id"] = row["user_id"].as<int>();
            member["username"] = row["username"].as<std::string>();
            member["member_status"] = row["member_status"].as<std::string>();
            member["member_role"] = row["member_role"].as<std::string>();
            members.append(member);
        }

        response["members"] = members;
    } catch (const drogon::orm::DrogonDbException &e) {
        response["error"] = "数据库错误，无法获取成员列表";
        auto resp = HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
        return;
    }

    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(drogon::k200OK); // 成功返回 200 OK
    callback(resp);
}