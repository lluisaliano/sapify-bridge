#include "services/ShopifyClient.hpp"

#include <drogon/HttpClient.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpTypes.h>
#include <format>
#include <glaze/json.hpp>
#include <glaze/json/write.hpp>
#include <stdexcept>
#include <string_view>

namespace sapify {

ShoppifyClient::ShoppifyClient(std::string_view shopDomain,
                               std::string_view storeName,
                               std::string_view clientId,
                               std::string_view clientSecret,
                               std::string_view apiVersion)
    : m_shopDomain{shopDomain}, m_storeName{storeName}, m_clientId{clientId},
      m_clientSecret{clientSecret}, m_apiVersion{apiVersion} {
  // We have to fetch API to get corresponding accessToken
  // Vary this when adding Pretty
}

// Send queries to Shopify admin api using graphql
glz::generic ShoppifyClient::graphql(const std::string &query,
                                     const glz::generic &variables) const {

  ensureAccessToken();

  const auto baseUrl{"https://" + m_shopDomain};
  // Get path from .env variable too
  const auto path{"/admin/api/2026-07/graphql.json"};

  // Set HTTP client
  const auto client{drogon::HttpClient::newHttpClient(baseUrl)};

  glz::generic body = {{"query", query}, {"variables", variables}};
  auto payload = glz::write_json(body);
  if (!payload.has_value()) {
      throw std::runtime_error("Failed to serialize Shopify request");
  }

  // Prepare request to shopify endpoint
  const auto request = drogon::HttpRequest::newHttpRequest();
  request->setMethod(drogon::Post);
  request->setPath(path);
  request->setContentTypeCode(drogon::CT_APPLICATION_JSON);
  request->addHeader("X-Shopify-Access-Token", m_accessToken);
  // Serialize Json
  request->setBody(std::move(*payload));

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

  glz::generic response;
  const std::string_view responseBody{result.second->body()};

  if (glz::read_json(response, responseBody)) {
      throw std::runtime_error("Invalid JSON in Shopify response");
  }

  return response;
}

void ShoppifyClient::getAccessToken() const {
  const auto baseUrl = "https://" + m_shopDomain;

  auto client = drogon::HttpClient::newHttpClient(baseUrl);

  glz::generic body = {
      {"client_id", m_clientId},
      {"client_secret", m_clientSecret},
      {"grant_type", "client_credentials"},
  };

  auto payload = glz::write_json(body);
  if (!payload.has_value()) {
      throw std::runtime_error("Invalid JSON in getting access token");
  }

  auto request = drogon::HttpRequest::newHttpRequest();
  request->setMethod(drogon::Post);
  // Path to get access token, may change in the future??
  request->setPath("/admin/oauth/access_token");
  request->setContentTypeCode(drogon::CT_APPLICATION_JSON);
  request->setBody(std::move(*payload));

  auto result = client->sendRequest(request);
};
} // namespace sapify
