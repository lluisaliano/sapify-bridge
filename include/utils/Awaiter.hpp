#pragma once

#include <drogon/HttpClient.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/utils/coroutine.h>

namespace sapify {
// Define fetch function to use with drogon::when_all. As this method does not
// suppport awaiting on HttpResponseAwaiter because can not deduce the result of the awaiter
inline drogon::Task<drogon::HttpResponsePtr>
fetch(drogon::HttpClientPtr &client, drogon::HttpRequestPtr &req) {
    co_return co_await client->sendRequestCoro(req);
}
} // namespace sapify
