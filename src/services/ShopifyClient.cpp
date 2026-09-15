#include "services/ShopifyClient.hpp"

#include <chrono>
#include <cstdint>
#include <drogon/HttpClient.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpTypes.h>
#include <format>
#include <glaze/json.hpp>
#include <glaze/json/write.hpp>
#include <stdexcept>
#include <string_view>

namespace sapify {

struct AccessTokenResult {
      std::string access_token;
      std::string scope;
      std::uint32_t expires_in;
    };

ShopifyClient::ShopifyClient(std::string_view shopDomain,
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
glz::generic ShopifyClient::graphql(const std::string &query,
                                     const glz::generic &variables) {

  // Check if token is valid
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

  if (status < drogon::HttpStatusCode::k200OK ||
      status >= drogon::HttpStatusCode::k300MultipleChoices) {
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

void ShopifyClient::getAccessToken() {
  const auto baseUrl = "https://" + m_shopDomain;

  auto client = drogon::HttpClient::newHttpClient(baseUrl);

  const auto body =
      std::format("grant_type=client_credentials&client_id={}&client_secret={}",
                  m_clientId, m_clientSecret);

  auto request = drogon::HttpRequest::newHttpRequest();
  request->setMethod(drogon::Post);
  // Path to get access token, may change in the future??
  request->setPath("/admin/oauth/access_token");
  request->setContentTypeCode(drogon::CT_APPLICATION_X_FORM);
  request->setBody(std::move(body));

  auto [reqResult, response] = client->sendRequest(request);

  if (reqResult != drogon::ReqResult::Ok || !response) {
    throw std::runtime_error("Shopify request on getting token failed");
  }

  const auto status = response->getStatusCode();

  if (status < drogon::HttpStatusCode::k200OK ||
      status >= drogon::HttpStatusCode::k300MultipleChoices) {
    throw std::runtime_error(
        "Shopify request returned a non2-xxx status, maybe wrong tokens used");
  }

  const auto responseBody = response->getBody();

  AccessTokenResult res;
  if (auto err = glz::read_json(res, responseBody)) {
    throw std::runtime_error("Failed on reading Shopify authentication json");
  };

  m_accessToken = res.access_token;
  // We will delete token 5 minutes after expiration
  m_tokenExpiresAt = std::chrono::steady_clock::now() +
                     std::chrono::seconds{res.expires_in} -
                     std::chrono::minutes{5};
  return;
}

void ShopifyClient::ensureAccessTokenAsync(TokenSuccessCallback onSuccess,
                                           ErrorCallback onError) {
  if (hasValidToken()) {
    onSuccess(m_accessToken);
    return;
  }

  getAccessTokenAsync(std::move(onSuccess), std::move(onError));
}

void ShopifyClient::getAccessTokenAsync(TokenSuccessCallback onSuccess,
                                        ErrorCallback onError) {
  const auto baseUrl = "https://" + m_shopDomain;

  auto client = drogon::HttpClient::newHttpClient(baseUrl);

  const auto body =
      std::format("grant_type=client_credentials&client_id={}&client_secret={}",
                  m_clientId, m_clientSecret);

  auto request = drogon::HttpRequest::newHttpRequest();
  request->setMethod(drogon::Post);
  request->setPath("/admin/oauth/access_token");
  request->setContentTypeCode(drogon::CT_APPLICATION_X_FORM);
  request->setBody(body);

  client->sendRequest(
      request,
      [this, onSuccess = std::move(onSuccess),
       onError = std::move(onError)](drogon::ReqResult reqResult,
                                     const drogon::HttpResponsePtr &response) {
        if (reqResult != drogon::ReqResult::Ok || !response) {
          onError("Shopify request on getting token failed");
          return;
        }

        const auto status = response->getStatusCode();

        if (status < drogon::HttpStatusCode::k200OK ||
            status >= drogon::HttpStatusCode::k300MultipleChoices) {
          onError("Shopify request returned a non2-xxx status, maybe wrong tokens used");
          return;
        }

        const auto responseBody = response->getBody();

        AccessTokenResult res;
        if (auto err = glz::read_json(res, responseBody)) {
          onError("Failed on reading Shopify authentication json");
          return;
        }

        m_accessToken = res.access_token;
        m_tokenExpiresAt = std::chrono::steady_clock::now() +
                           std::chrono::seconds{res.expires_in} -
                           std::chrono::minutes{5};

        onSuccess(m_accessToken);
      });
}
} // namespace sapify
