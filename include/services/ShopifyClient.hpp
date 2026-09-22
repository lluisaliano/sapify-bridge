#pragma once

#include "services/types/ShopifyClientTypes.hpp"
#include <chrono>
#include <drogon/HttpClient.h>
#include <drogon/utils/coroutine.h>
#include <glaze/core/feature_test.hpp>
#include <glaze/json.hpp>
#include <glaze/json/generic_fwd.hpp>
#include <string>
#include <string_view>

namespace sapify {

class ShopifyClient {
  public:
    ShopifyClient(std::string_view shopDomain, std::string_view storeName,
                  std::string_view clientId, std::string_view clientSecret,
                  std::string_view apiVersion);

    // ------------ This will go private
    // Ensure accesToken exists
    drogon::Task<std::string> ensureAccessToken();

    // Fetch Product Data
    drogon::Task<FetchProductDataResponse>
    fetchProductData(const std::string_view product);

    // Return accessToken
    std::string_view token() const { return m_accessToken; };

  private:
    std::string m_shopDomain{};
    std::string m_storeName{};
    std::string m_clientId{};
    std::string m_clientSecret{};
    std::string m_apiVersion{};
    std::string m_accessToken{};
    std::chrono::steady_clock::time_point m_tokenExpiresAt;

  private:
    // This function is for testing only
    drogon::Task<std::string> getAccessToken();

    // Check if token exists
    bool hasValidToken() const {
        return !m_accessToken.empty() &&
               std::chrono::steady_clock::now() < m_tokenExpiresAt;
    }

  public:
       // Launch a graphql query to shopify
      // Send queries to Shopify admin api using graphql
      template <typename ResponseFormat = glz::generic, typename Variables>
      drogon::Task<std::pair<ResponseFormat, std::string>>
      graphql(std::string_view query,
                             const Variables &variables) {

          // Check if token is valid, we get it from here as it is cleanr than taking
          // if from the private member
          const auto token = co_await ensureAccessToken();

          const auto baseUrl{"https://" + m_shopDomain};
          // Get path from .env variable too
          const auto path{"/admin/api/2026-07/graphql.json"};

          // Set HTTP client
          auto client{drogon::HttpClient::newHttpClient(baseUrl)};

          ShopifyRequest<Variables> body{query, variables};
          std::string bodyJson;
          if (glz::write_json(body, bodyJson)) {
              throw std::runtime_error("Failed to serialize Shopify request");
          }

          // Prepare request to shopify endpoint
          const auto request = drogon::HttpRequest::newHttpRequest();
          request->setMethod(drogon::Post);
          request->setPath(path);
          request->setContentTypeCode(drogon::CT_APPLICATION_JSON);
          request->addHeader("X-Shopify-Access-Token", token);

          request->setBody(std::move(bodyJson));

          auto result = co_await client->sendRequestCoro(request);

          if (!result) {
              throw std::runtime_error(std::format(
                  "Empty Shopify Graphql Response for store {}", m_storeName));
          }

          const auto status = result->getStatusCode();

          if (status < drogon::HttpStatusCode::k200OK ||
              status >= drogon::HttpStatusCode::k300MultipleChoices) {
              throw std::runtime_error(
                  std::format("Shopify Graphql returned non-2xx status for store {}",
                              m_storeName));
          }

          ResponseFormat response;
          // Copy JSON Body. We have to construct a string here to return the json
          // without its lifetime ending. The compiler may optimize NRVO
          const std::string responseJson{result->body()};

          // TODO This steap of parsing json here may be not needed
          if (glz::read_json(response, responseJson)) {
              throw std::runtime_error("Invalid JSON in Shopify response");
          }
          // Return response and responseJson to avoid serializing again. Use move to
          // construct the pair
          co_return std::pair{std::move(response), std::move(responseJson)};
      }
};
} // namespace sapify
