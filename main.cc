#include <drogon/drogon.h>
#include <json/value.h>

int main() {
    // 加载配置文件
    drogon::app().loadConfigFile("../config.json");

    // 全局异常处理
    // drogon::app().setExceptionHandler(
    //     [](const std::exception &e, const drogon::HttpRequestPtr &req,
    //        std::function<void(drogon::HttpResponsePtr &)> &&callback) {
    //         Json::Value json;
    //         json["error"] = e.what();
    //         auto resp = drogon::HttpResponse::newHttpJsonResponse(json);
    //         callback(resp);
    //     });

    // CORS：处理 OPTIONS 请求
    // drogon::app().registerSyncAdvice([](const drogon::HttpRequestPtr &req) -> drogon::HttpResponsePtr {
    //     if (req->method() == drogon::HttpMethod::Options) {
    //         auto resp = drogon::HttpResponse::newHttpResponse();
    //         const auto &origin = req->getHeader("Origin");
    //         const auto &method = req->getHeader("Access-Control-Request-Method");
    //         const auto &headers = req->getHeader("Access-Control-Request-Headers");

    //         if (!origin.empty())
    //             resp->addHeader("Access-Control-Allow-Origin", origin);
    //         if (!method.empty())
    //             resp->addHeader("Access-Control-Allow-Methods", method);
    //         if (!headers.empty())
    //             resp->addHeader("Access-Control-Allow-Headers", headers);

    //         resp->addHeader("Access-Control-Allow-Credentials", "true");
    //         return resp;
    //     }
    //     return {};
    // });

    // // CORS：处理实际请求添加响应头
    // drogon::app().registerPostHandlingAdvice([](const drogon::HttpRequestPtr &req, const drogon::HttpResponsePtr &resp) {
    //     const auto &origin = req->getHeader("Origin");
    //     if (!origin.empty()) {
    //         resp->addHeader("Access-Control-Allow-Origin", origin);
    //         resp->addHeader("Access-Control-Allow-Credentials", "true");
    //     }
    // });

    // 启动服务
    drogon::app().run();
    return 0;
}
