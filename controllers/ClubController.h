#pragma once
#include <drogon/HttpController.h>
#include <stdexcept>

using namespace drogon;

// 社团结构体，包含社团相关字段
struct Club {
    std::string club_name;
    std::string club_introduction;
    std::string contact_info;
    std::string activity_venue;
    int founder_id; // 创建者用户 ID
};

class ClubController : public drogon::HttpController<ClubController> {
public:
    METHOD_LIST_BEGIN
    // 创建社团接口
    ADD_METHOD_TO(ClubController::create, "/club/create", Post);
    // 获取社团列表接口
    ADD_METHOD_TO(ClubController::list, "/club/list", Get);
    // 获取社团详情接口
    ADD_METHOD_TO(ClubController::detail, "/club/detail/{1}", Get); // {1} 表示路径参数 club_id
    // 添加获取当前用户拥有的社团接口
    ADD_METHOD_TO(ClubController::ownedClubs, "/club/owned", Get);
    METHOD_LIST_END

    // 创建社团方法
    void create(const HttpRequestPtr &req,
                std::function<void(const HttpResponsePtr &)> &&callback,
                Club club) const;

    // 获取社团列表方法
    void list(const HttpRequestPtr &req,
              std::function<void(const HttpResponsePtr &)> &&callback) const;

    // 获取社团详情方法
    void detail(const HttpRequestPtr &req,
                std::function<void(const HttpResponsePtr &)> &&callback,
                int club_id) const;

    // 获取当前用户拥有的社团方法
    void ownedClubs(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback) const;
};

// 自定义从请求中解析 Club 对象的方法
namespace drogon {
template <> inline Club fromRequest(const HttpRequest &req) {
    auto json = req.getJsonObject();
    Club value;

    if (json == nullptr) {
        throw std::runtime_error("请求体格式错误，请使用 JSON");
    }

    // 解析社团名称
    if (json->isMember("club_name")) {
        const auto &temp = (*json)["club_name"].asString();
        if (temp.empty()) {
            throw std::runtime_error("社团名称不能为空");
        }
        value.club_name = temp;
    } else {
        throw std::runtime_error("缺少必备字段: club_name");
    }

    // 解析社团简介
    value.club_introduction = json->get("club_introduction", "").asString();

    // 解析联系方式
    value.contact_info = json->get("contact_info", "").asString();

    // 解析活动场地
    value.activity_venue = json->get("activity_venue", "").asString();

    // 解析创建者 ID
    if (json->isMember("founder_id")) {
        value.founder_id = (*json)["founder_id"].asInt();
    } else {
        throw std::runtime_error("缺少必备字段: founder_id");
    }

    return value;
}
} // namespace drogon