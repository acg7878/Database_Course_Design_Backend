#include "UserController.h"
#include <drogon/Cookie.h>
#include <drogon/HttpResponse.h>
#include <drogon/orm/Exception.h>
#include <iostream>


// 用户注册功能实现
void UserController::m_register(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback, User user) const {
  auto dbClient = drogon::app().getDbClient();
  Json::Value json;

  try {
    // 检查用户名是否已存在
    auto result = dbClient->execSqlSync(
        "SELECT COUNT(*) AS count FROM user WHERE username = ?", user.username);
    if (result[0]["count"].as<int>() > 0) {
      json["error"] = "用户名已存在，注册失败";
      auto resp = drogon::HttpResponse::newHttpJsonResponse(json);
      resp->setStatusCode(k400BadRequest);
      callback(resp);
      return;
    }

    // 插入用户数据到数据库
    dbClient->execSqlSync("INSERT INTO user (username, password, email, phone, "
                          "user_type) VALUES (?, ?, ?, ?, ?)",
                          user.username, user.password, user.email, user.phone,
                          user.user_type);
      std::cout << "注册成功" <<std::endl;
    json["message"] = "注册成功";
  } catch (const drogon::orm::DrogonDbException &e) {
    json["error"] = "数据库错误，注册失败";
  }

  // 返回响应
  auto resp = drogon::HttpResponse::newHttpJsonResponse(json);
  resp->setStatusCode(drogon::k200OK); 
  callback(resp);
}

void UserController::login(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) const {
  auto dbClient = drogon::app().getDbClient();
  auto json = req->getJsonObject();
  Json::Value response;

  if (!json || !json->isMember("username") || !json->isMember("password")) {
    response["error"] = "请求参数缺失";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k400BadRequest);
    callback(resp);
    return;
  }

  std::string username = (*json)["username"].asString();
  std::string password = (*json)["password"].asString();

  try {
    auto result = dbClient->execSqlSync(
        "SELECT * FROM user WHERE username = ? AND password = ?", username,
        password);
    if (!result.empty()) {
      int user_id = result[0]["user_id"].as<int>();
      std::cout << "登录成功" << std::endl;
      response["message"] = "登录成功";
      response["user_id"] = user_id;

      auto resp = HttpResponse::newHttpJsonResponse(response);
      // 设置 Cookie
      Cookie userIdCookie("user_id", std::to_string(user_id));
      userIdCookie.setHttpOnly(true);
      userIdCookie.setPath("/");
      //userIdCookie.setMaxAge(3600); // 1小时

      Cookie loginStatus("is_logged_in", "true");
      loginStatus.setHttpOnly(true);
      loginStatus.setPath("/");
      //loginStatus.setMaxAge(3600);

      resp->addCookie(userIdCookie);
      resp->addCookie(loginStatus);
      resp->setStatusCode(k200OK);
      callback(resp);
    } else {
      response["error"] = "用户名或密码错误";
      auto resp = HttpResponse::newHttpJsonResponse(response);
      resp->setStatusCode(k401Unauthorized);
      callback(resp);
    }
  } catch (...) {
    response["error"] = "数据库错误";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k500InternalServerError);
    callback(resp);
  }
}

void UserController::logout(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) const {
  Json::Value response;
  response["message"] = "登出成功";

  auto resp = HttpResponse::newHttpJsonResponse(response);
  // 清除 Cookie
  Cookie userIdCookie("user_id", "");
  userIdCookie.setPath("/");
  userIdCookie.setMaxAge(0); // 删除

  Cookie loginStatus("is_logged_in", "");
  loginStatus.setPath("/");
  loginStatus.setMaxAge(0);
  resp->setStatusCode(drogon::k200OK); 
  resp->addCookie(userIdCookie);
  resp->addCookie(loginStatus);

  callback(resp);
}

void UserController::info(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) const {
  auto dbClient = drogon::app().getDbClient();
  Json::Value response;

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
    auto result =
        dbClient->execSqlSync("SELECT * FROM user WHERE user_id = ?", user_id);
    if (!result.empty()) {
      response["user_id"] = result[0]["user_id"].as<int>();
      response["username"] = result[0]["username"].as<std::string>();
      response["email"] = result[0]["email"].as<std::string>();
      response["phone"] = result[0]["phone"].as<std::string>();
      response["user_type"] = result[0]["user_type"].as<std::string>();
    } else {
      response["error"] = "用户不存在";
      auto resp = HttpResponse::newHttpJsonResponse(response);
      resp->setStatusCode(k404NotFound);
      callback(resp);
      return;
    }
  } catch (...) {
    response["error"] = "数据库错误";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k500InternalServerError);
    callback(resp);
    return;
  }

  auto resp = HttpResponse::newHttpJsonResponse(response);
  resp->setStatusCode(drogon::k200OK); 
  callback(resp);
}

void UserController::update(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) const {
  auto dbClient = drogon::app().getDbClient();
  auto json = req->getJsonObject();
  Json::Value response;

  auto userIdCookie = req->getCookie("user_id");
  if (userIdCookie.empty()) {
    response["error"] = "未登录";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k401Unauthorized);
    callback(resp);
    return;
  }

  int user_id = std::stoi(userIdCookie);

  std::string username = (*json)["username"].asString();
  std::string password = (*json)["password"].asString();
  std::string email = (*json)["email"].asString();
  std::string phone = (*json)["phone"].asString();

  try {
    dbClient->execSqlSync(
        "UPDATE user SET username = ?, password = ?, email = ?, phone = ? WHERE user_id = ?",
        username, password, email, phone, user_id);
    response["message"] = "更新成功";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(drogon::k200OK); 
    callback(resp);
  } catch (...) {
    response["error"] = "数据库错误";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k500InternalServerError);
    callback(resp);
  }
}

void UserController::remove(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) const {
  auto dbClient = drogon::app().getDbClient();
  Json::Value response;

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
    dbClient->execSqlSync("DELETE FROM user WHERE user_id = ?", user_id);
    response["message"] = "删除成功";

    // 同时登出（清除 cookie）
    auto resp = HttpResponse::newHttpJsonResponse(response);
    Cookie userIdCookie("user_id", "");
    userIdCookie.setPath("/");
    userIdCookie.setMaxAge(0);
    Cookie loginStatus("is_logged_in", "");
    loginStatus.setPath("/");
    loginStatus.setMaxAge(0);
    resp->addCookie(userIdCookie);
    resp->addCookie(loginStatus);
    resp->setStatusCode(drogon::k200OK); 
    callback(resp);
  } catch (...) {
    response["error"] = "数据库错误";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k500InternalServerError);
    callback(resp);
  }
}

void UserController::getUserRole(
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
    // 查询用户的权限信息
    auto userResult = dbClient->execSqlSync(
        "SELECT user_type FROM user WHERE user_id = ?", user_id);

    if (userResult.empty()) {
      response["error"] = "用户不存在";
      auto resp = HttpResponse::newHttpJsonResponse(response);
      resp->setStatusCode(k404NotFound);
      callback(resp);
      return;
    }

    // 获取用户权限
    std::string userType = userResult[0]["user_type"].as<std::string>();
    response["user_type"] = userType;

    // 如果用户是社长，查询其管理的社团
    if (userType == "社长") {
      auto clubResult = dbClient->execSqlSync(
          "SELECT club_id, club_name FROM club WHERE founder_id = ?", user_id);

      Json::Value clubs(Json::arrayValue);
      for (const auto &row : clubResult) {
        Json::Value club;
        club["club_id"] = row["club_id"].as<int>();
        club["club_name"] = row["club_name"].as<std::string>();
        clubs.append(club);
      }
      response["managed_clubs"] = clubs;
    }

    response["message"] = "获取用户权限成功";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k200OK);
    callback(resp);
  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Database error: " << e.base().what();
    response["error"] = "数据库错误，无法获取用户权限";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k500InternalServerError);
    callback(resp);
  }
}
