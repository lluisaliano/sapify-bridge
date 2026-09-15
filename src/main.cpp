#include "config/Config.hpp"
#include "services/ShopifyClient.hpp"

#include <drogon/HttpAppFramework.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>
#include <drogon/drogon.h>
#include <exception>
#include <functional>
#include <glaze/json/write.hpp>
#include <print>
#include <string>
#include <string_view>

// Server runner
namespace sapify {

struct HealthResponse {
  std::string_view status;
};

struct TokenResponse {
  std::string accessToken;
};

struct ErrorResponse {
  std::string error;
};

using Callback = std::function<void(const drogon::HttpResponsePtr &)>;

int run() {
  const auto config = sapify::Config::fromEnv();

  // Create shopify client
  ShopifyClient client{config.mascaroDomain, "pretty", config.mascaroClientId,
                       config.mascaroClientSecret, config.shopifyApiVersion};

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
      "/token", [&](const drogon::HttpRequestPtr &, Callback &&callback) {
        client.ensureAccessTokenAsync(
            [callback](std::string token) {
              TokenResponse res{std::move(token)};
              std::string resJson;

              auto response = drogon::HttpResponse::newHttpResponse();
              if (const auto er = glz::write_json(res, resJson)) {
                std::println(std::cerr, "Error on serializing token response");
                response->setStatusCode(drogon::k500InternalServerError);
                callback(response);
                return;
              }

              response->setContentTypeCode(drogon::CT_APPLICATION_JSON);
              response->setBody(std::move(resJson));
              callback(response);
            },
            [callback](std::string error) {
              ErrorResponse res{std::move(error)};
              std::string resJson;

              auto response = drogon::HttpResponse::newHttpResponse();
              response->setStatusCode(drogon::k500InternalServerError);

              if (const auto er = glz::write_json(res, resJson)) {
                std::println(std::cerr, "Error on serializing error response");
                callback(response);
                return;
              }

              response->setContentTypeCode(drogon::CT_APPLICATION_JSON);
              response->setBody(std::move(resJson));
              callback(response);
            });
      });

  std::println("Starting sapify-bridge on {}:{} with log level {}", config.host,
               config.port, config.logLevel);

  // Start drogon server, configur static files route
  drogon::app()
      .addListener(config.host, config.port)
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
