#include "config/Config.hpp"

#include <drogon/HttpAppFramework.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>
#include <drogon/drogon.h>
#include <nlohmann/json.hpp>

#include <exception>
#include <nlohmann/json_fwd.hpp>
#include <print>
#include <string>

// Server runner
namespace sapify {

using Callback = std::function<void(drogon::HttpResponsePtr&)>;

int run() {
    const auto config = sapify::Config::fromEnv();

    drogon::app().registerHandler("/health", [](const drogon::HttpRequestPtr&, Callback&& callback) {
        const nlohmann::json body = {
            {"status", "ok"},
            {"service", "sapify-bridge"},
        };

        // Return a JSON
        auto response = drogon::HttpResponse::newHttpJsonResponse(body.dump());
        callback(response);
    },
    {drogon::Get});

    std::println("Starting sapify-bridge on {}:{} with log level {}", config.host, config.port, config.logLevel);

    // Start drogon server, configur static files route
    drogon::app().addListener(config.host, config.port).setDocumentRoot("./static").run();

    return 0;
}
}

int main() {
    try {
        return sapify::run();
    } catch (const std::exception& error) {
        std::println(std::cerr, "Fatal error: {}", error.what());
        return 1;
    }
}
