#include "StatusCtrl.h"
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>

std::string exec(const char *cmd)
{
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe)
    {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
    {
        result += buffer.data();
    }
    return result;
}

void StatusCtrl::asyncHandleHttpRequest(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto path = req->getPath();
    if (path == "/status")
    {
        std::string cpu_info = exec("top -b n1 | grep -Po '[0-9.]+ id,'| awk '{print 100-$1}'");
        std::string mem_info = exec("free -m | grep Mem | awk '{print ($3/$2)*100}'");
        HttpViewData data;
        data.insert("cpu_info", cpu_info + "%");
        data.insert("mem_info", mem_info + "%");
        auto resp = HttpResponse::newHttpViewResponse("ListParameters.csp", data);
        callback(resp);
    }
    else if (path == "/cpu")
    {
        std::string cpu_info = exec("top -b n1 | grep -Po '[0-9.]+ id,'| awk '{print 100-$1}'");
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k200OK);
        resp->setContentTypeCode(CT_TEXT_HTML);
        resp->setBody(cpu_info);
        callback(resp);
    }
    else if (path == "/mem")
    {
        std::string mem_info = exec("free -m | grep Mem | awk '{print ($3/$2)*100}'");
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k200OK);
        resp->setContentTypeCode(CT_TEXT_HTML);
        resp->setBody(mem_info);
        callback(resp);
    }
    else if (path == "/rps")
    {
        auto base_time = std::chrono::hours(1000000);
        std::chrono::duration<int64_t> q_time = std::chrono::minutes(1);
        auto q = req->getParameter("q");
        if (q == "1s")
            q_time = std::chrono::seconds(1);
        else if (q == "1m")
            q_time = std::chrono::minutes(1);
        else if (q == "10m")
            q_time = std::chrono::minutes(10);
        auto now = std::chrono::system_clock::now();
        std::int64_t starttime = std::chrono::duration_cast<std::chrono::milliseconds>((now - base_time - q_time).time_since_epoch()).count();
        std::int64_t endtime = std::chrono::duration_cast<std::chrono::milliseconds>((now - base_time).time_since_epoch()).count();

        nosql::RedisClientPtr redis_ptr = drogon::app().getFastRedisClient();
        redis_ptr->execCommandAsync([callback](const nosql::RedisResult &r)
                                    {
                                        if (r.type() == nosql::RedisResultType::kNil)
                                            LOG_INFO << "Cannot find variable associated with the key 'name'";
                                        else{
                                            auto resp = HttpResponse::newHttpResponse();
                                            resp->setStatusCode(k200OK);
                                            resp->setContentTypeCode(CT_TEXT_HTML);
                                            resp->setBody(r.asString());
                                            callback(resp); } },
                                    [](const std::exception &err)
                                    {
                                        LOG_ERROR << err.what();
                                    },
                                    "ZCOUNT echo2 %lld %lld", starttime, endtime);
    }
}
