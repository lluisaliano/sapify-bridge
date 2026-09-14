#include "services/ShopifyClient.hpp"

#include <drogon/HttpClient.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpTypes.h>
#include <format>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string_view>

namespace sapify {

ShoppifyClient::ShoppifyClient(std::string_view shopDomain,
                               std::string_view accessToken,
                               std::string_view storeName)
    : m_shopDomain{shopDomain}, m_accessToken{accessToken},
      m_storeName{storeName} {}

// Send queries to Shopify admin api using graphql
nlohmann::json ShoppifyClient::graphql(const std::string &query,
                                       const nlohmann::json &variables) const {
  const auto baseUrl{"https://" + m_shopDomain};
  // Get path from .env variable too
  const auto path{"/admin/api/2026-07/graphql.json"};

  // Set HTTP client
  const auto client{drogon::HttpClient::newHttpClient(baseUrl)};

  nlohmann::json body = {{"query", query}, {"variables", variables}};

  // Prepare request to shopify endpoint
  const auto request = drogon::HttpRequest::newHttpRequest();
  request->setMethod(drogon::Post);
  request->setPath(path);
  request->setContentTypeCode(drogon::CT_APPLICATION_JSON);
  request->addHeader("X-Shopify-Access-Token", m_accessToken);
  // Serialize Json
  request->setBody(body.dump());

  const auto result = client->sendRequest(request);

  if (result.first != drogon::ReqResult::Ok || result.second == nullptr) {
    throw std::runtime_error(std::format(
        "Shopify Graphql request failed for store {}", m_storeName));
  }

  const auto status = result.second->getStatusCode();

  if (status < 200 || status >= 300) {
    throw std::runtime_error(std::format(
        "Shopify Graphql returned non-2xx status for store {}", m_storeName));
  }

  return nlohmann::json::parse(result.second->body());
}
} // namespace sapify
