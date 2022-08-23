#include "EchoCtrl.h"

void EchoCtrl::asyncHandleHttpRequest(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto base_time = std::chrono::hours(1000000);
    std::int64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>((std::chrono::system_clock::now() - base_time).time_since_epoch()).count();
    std::string message = req->getParameter("message");
    nosql::RedisClientPtr redis_ptr = drogon::app().getFastRedisClient();
    redis_ptr->execCommandAsync([callback, message](const nosql::RedisResult &r)
                                {
                                    auto resp = HttpResponse::newHttpResponse();
                                    resp->setStatusCode(k200OK);
                                    resp->setContentTypeCode(CT_APPLICATION_JSON);
                                    resp->setBody("{\"message\":\"" + message + "\"}");
                                    callback(resp); },
                                [](const std::exception &err)
                                {
                                    LOG_ERROR << err.what();
                                },
                                "ZADD echo2 %lld %s", timestamp, drogon::utils::getUuid());
}
