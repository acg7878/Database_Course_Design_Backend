#pragma once
#include <drogon/HttpController.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>
#include <functional>
#include <stdexcept>

using namespace drogon;

// 用户结构体，包含注册所需的字段
struct User {
  std::string username;
  std::string password;
  std::string email;
  std::string phone;
  std::string user_type; // 用户类型：社员、社长、管理员
};

class UserController : public drogon::HttpController<UserController> {
public:
  METHOD_LIST_BEGIN
  // 注册接口
  ADD_METHOD_TO(UserController::m_register, "/user/register", Post);
  // 登录接口
  ADD_METHOD_TO(UserController::login, "/user/login", Post);
  // 获取用户信息接口
  ADD_METHOD_TO(UserController::info, "/user/info", Get);
  // 更新用户信息接口
  ADD_METHOD_TO(UserController::update, "/user/update", Put);
  // 删除用户接口
  ADD_METHOD_TO(UserController::remove, "/user/delete", Delete);
  // 退出登录接口
  ADD_METHOD_TO(UserController::logout, "/user/logout", Post);
  // 获取用户权限接口
  ADD_METHOD_TO(UserController::getUserRole, "/user/role", Get);
  METHOD_LIST_END

  // 注册方法
  void m_register(const HttpRequestPtr &req,
                  std::function<void(const HttpResponsePtr &)> &&callback,
                  User user) const;

  // 登录方法
  void login(const HttpRequestPtr &req,
             std::function<void(const HttpResponsePtr &)> &&callback) const;

  // 获取用户信息方法
  void info(const HttpRequestPtr &req,
            std::function<void(const HttpResponsePtr &)> &&callback) const;

  // 更新用户信息方法
  void update(const HttpRequestPtr &req,
              std::function<void(const HttpResponsePtr &)> &&callback) const;

  // 删除用户方法
  void remove(const HttpRequestPtr &req,
              std::function<void(const HttpResponsePtr &)> &&callback) const;

  // 退出登录方法
  void logout(const HttpRequestPtr &req,
              std::function<void(const HttpResponsePtr &)> &&callback) const;

  // 获取用户权限方法
  void getUserRole(const HttpRequestPtr &req,
                   std::function<void(const HttpResponsePtr &)> &&callback) const;
};

// 自定义从请求中解析 User 对象的方法
namespace drogon {
template <> inline User fromRequest(const HttpRequest &req) {
  auto json = req.getJsonObject();
  User value;

  if (json == nullptr) {
    throw std::runtime_error("请求体格式错误，请使用 JSON");
  }

  // 解析用户名
  if (json->isMember("username")) {
    const auto &temp = (*json)["username"].asString();
    if (temp.size() < 3) {
      throw std::runtime_error("用户名长度过短");
    }
    value.username = temp;
  } else {
    throw std::runtime_error("缺少必备字段: username");
  }

  // 解析密码
  if (json->isMember("password")) {
    const auto &temp = (*json)["password"].asString();
    if (temp.size() < 6) {
      throw std::runtime_error("密码长度过短");
    }
    value.password = temp;
  } else {
    throw std::runtime_error("缺少必备字段: password");
  }

  // 解析邮箱
  if (json->isMember("email")) {
    value.email = (*json)["email"].asString();
  } else {
    value.email = ""; // 默认值
  }

  // 解析手机号
  if (json->isMember("phone")) {
    value.phone = (*json)["phone"].asString();
  } else {
    value.phone = ""; // 默认值
  }

  // 解析用户类型
  if (json->isMember("user_type")) {
    const auto &temp = (*json)["user_type"].asString();
    if (temp != "社员" && temp != "社长" && temp != "管理员") {
      throw std::runtime_error("用户类型无效");
    }
    value.user_type = temp;
  } else {
    value.user_type = "社员"; // 默认值
  }

  return value;
}
} // namespace drogon