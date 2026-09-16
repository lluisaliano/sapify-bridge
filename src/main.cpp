#include "config/Config.hpp"
#include "controllers/PushController.hpp"
#include "services/ShopifyClient.hpp"

#include <drogon/HttpAppFramework.h>
#include <drogon/HttpController.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>
#include <drogon/drogon.h>
#include <exception>
#include <functional>
#include <glaze/json/write.hpp>
#include <memory>
#include <print>
#include <string>
#include <string_view>

// Server runner
namespace sapify {

struct HealthResponse {
    std::string_view status;
};

struct ErrorResponse {
    std::string error;
};

using Callback = std::function<void(const drogon::HttpResponsePtr &)>;

int run() {
    const auto config = sapify::Config::fromEnv();

    // Create shopify client
    auto client = std::make_shared<ShopifyClient>(
        config->mascaroDomain, "pretty", config->mascaroClientId,
        config->mascaroClientSecret, config->shopifyApiVersion);

    // Push Controller (push information to sap) registration. We register it
    // here to pass client and config to parameters
    auto pushController = std::make_shared<PushController>(config, client);
    drogon::app().registerController(pushController);

    drogon::app().registerHandler(
        "/health",
        [](const drogon::HttpRequestPtr &, Callback &&callback) {
            HealthResponse res{"ok"};

            std::string resJson;

            auto err = glz::write_json(res, resJson);
            if (err) {
                std::println(std::cerr, "Could not serialize response json");
            }

            // Return a JSON
            auto response = drogon::HttpResponse::newHttpResponse();
            response->setContentTypeCode(drogon::CT_APPLICATION_JSON);
            response->setBody(std::move(resJson));
            callback(response);
        },
        {drogon::Get});

    // Return Token handler
    drogon::app().registerHandler(
        "/token",
        // Config is taken by reference but it lives outside the event loop
        [&client](
            drogon::HttpRequestPtr) -> drogon::Task<drogon::HttpResponsePtr> {
            auto token = co_await client->ensureAccessToken();

            // Place token into json
            glz::generic res{token};
            std::string resJson;
            auto response = drogon::HttpResponse::newHttpResponse();
            if (glz::write_json(res, resJson)) {
                // Error failed to serialize
                response->setStatusCode(
                    drogon::HttpStatusCode::k500InternalServerError);
                co_return response;
            }
            response->setContentTypeCode(drogon::CT_APPLICATION_JSON);
            response->setBody(std::move(resJson));

            co_return response;
        });

    std::println("Starting sapify-bridge on {}:{} with log level {}",
                 config->host, config->port, config->logLevel);

    // Start drogon server, configur static files route
    drogon::app()
        .addListener(config->host, config->port)
        .setDocumentRoot("./static")
        .run();

    return 0;
}
} // namespace sapify

int main() {
    try {
        return sapify::run();
    } catch (const std::exception &error) {
        std::println(std::cerr, "Fatal error: {}", error.what());
        return 1;
    }
}
