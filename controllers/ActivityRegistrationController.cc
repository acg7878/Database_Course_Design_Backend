#include "ActivityRegistrationController.h"
#include <drogon/HttpResponse.h>
#include <drogon/orm/Exception.h>

void ActivityRegistrationController::registerActivity(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) const
{
    auto dbClient = drogon::app().getDbClient();
    Json::Value response;

    // 从 Cookie 中获取当前登录用户的 user_id
    auto userIdCookie = req->getCookie("user_id");
    if (userIdCookie.empty())
    {
        response["error"] = "未登录";
        auto resp = HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k401Unauthorized);
        callback(resp);
        return;
    }

    int user_id = std::stoi(userIdCookie);

    // 获取请求体中的 activity_id
    auto json = req->getJsonObject();
    if (!json || !json->isMember("activity_id"))
    {
        response["error"] = "缺少必需字段: activity_id";
        auto resp = HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    int activity_id = (*json)["activity_id"].asInt();

    try
    {
        // 检查是否已经报名
        auto result = dbClient->execSqlSync(
            "SELECT COUNT(*) AS count FROM activity_registration WHERE user_id = ? AND activity_id = ?",
            user_id, activity_id);

        if (result[0]["count"].as<int>() > 0)
        {
            response["error"] = "您已报名该活动";
            auto resp = HttpResponse::newHttpJsonResponse(response);
            resp->setStatusCode(k400BadRequest);
            callback(resp);
            return;
        }

        // 插入报名记录
        dbClient->execSqlSync(
            "INSERT INTO activity_registration (user_id, activity_id, registration_date) VALUES (?, ?, NOW())",
            user_id, activity_id);

        response["message"] = "报名成功";
    }
    catch (const drogon::orm::DrogonDbException &e)
    {
        response["error"] = "数据库错误，无法报名";
    }

    auto resp = HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

void ActivityRegistrationController::cancelRegistration(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) const
{
    auto dbClient = drogon::app().getDbClient();
    Json::Value response;

    // 从 Cookie 中获取当前登录用户的 user_id
    auto userIdCookie = req->getCookie("user_id");
    if (userIdCookie.empty())
    {
        response["error"] = "未登录";
        auto resp = HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k401Unauthorized);
        callback(resp);
        return;
    }

    int user_id = std::stoi(userIdCookie);

    // 获取请求体中的 activity_id
    auto json = req->getJsonObject();
    if (!json || !json->isMember("activity_id"))
    {
        response["error"] = "缺少必需字段: activity_id";
        auto resp = HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    int activity_id = (*json)["activity_id"].asInt();

    try
    {
        // 删除报名记录
        auto result = dbClient->execSqlSync(
            "DELETE FROM activity_registration WHERE user_id = ? AND activity_id = ?",
            user_id, activity_id);

        if (result.affectedRows() == 0)
        {
            response["error"] = "未找到报名记录，无法取消报名";
            auto resp = HttpResponse::newHttpJsonResponse(response);
            resp->setStatusCode(k404NotFound);
            callback(resp);
            return;
        }

        response["message"] = "取消报名成功";
    }
    catch (const drogon::orm::DrogonDbException &e)
    {
        response["error"] = "数据库错误，无法取消报名";
    }

    auto resp = HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

void ActivityRegistrationController::getRegistrationList(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) const
{
    auto dbClient = drogon::app().getDbClient();
    Json::Value response;

    // 从 Cookie 中获取当前登录用户的 user_id
    auto userIdCookie = req->getCookie("user_id");
    if (userIdCookie.empty())
    {
        response["error"] = "未登录";
        auto resp = HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k401Unauthorized); // 未授权
        callback(resp);
        return;
    }

    int user_id = std::stoi(userIdCookie);

    try
    {
        // 查询用户是否是社长
        auto clubResult = dbClient->execSqlSync(
            "SELECT c.club_id FROM club c WHERE c.founder_id = ?", user_id);

        Json::Value registrations(Json::arrayValue);

        if (!clubResult.empty())
        {
            // 当前用户是社长，查询其所有社团的报名记录
            for (const auto &clubRow : clubResult)
            {
                int club_id = clubRow["club_id"].as<int>();

                auto result = dbClient->execSqlSync(
                    "SELECT r.registration_id, r.user_id, r.activity_id, r.registration_date, r.payment_status "
                    "FROM activity_registration r "
                    "JOIN club_activity a ON r.activity_id = a.activity_id "
                    "WHERE a.club_id = ?",
                    club_id);

                for (const auto &row : result)
                {
                    Json::Value registration;
                    registration["registration_id"] = row["registration_id"].as<int>();
                    registration["user_id"] = row["user_id"].as<int>();
                    registration["activity_id"] = row["activity_id"].as<int>();
                    registration["registration_date"] = row["registration_date"].as<std::string>();
                    registration["payment_status"] = row["payment_status"].as<std::string>();
                    registrations.append(registration);
                }
            }
        }
        else
        {
            // 当前用户是普通社员，只查询自己的报名记录
            auto result = dbClient->execSqlSync(
                "SELECT registration_id, user_id, activity_id, registration_date, payment_status "
                "FROM activity_registration WHERE user_id = ?",
                user_id);

            for (const auto &row : result)
            {
                Json::Value registration;
                registration["registration_id"] = row["registration_id"].as<int>();
                registration["user_id"] = row["user_id"].as<int>();
                registration["activity_id"] = row["activity_id"].as<int>();
                registration["registration_date"] = row["registration_date"].as<std::string>();
                registration["payment_status"] = row["payment_status"].as<std::string>();
                registrations.append(registration);
            }
        }

        response["registrations"] = registrations;
        response["message"] = "报名记录获取成功";
        auto resp = HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k200OK); // 成功返回 200 OK
        callback(resp);
    }
    catch (const drogon::orm::DrogonDbException &e)
    {
        response["error"] = "数据库错误，无法获取报名列表";
        auto resp = HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k500InternalServerError); // 服务器内部错误
        callback(resp);
    }
}