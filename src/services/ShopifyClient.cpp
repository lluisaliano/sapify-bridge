#include "services/ShopifyClient.hpp"
#include "services/types/Queryies.hpp"
#include "services/types/ShopifyClientTypes.hpp"

#include <chrono>
#include <cstdint>
#include <drogon/HttpClient.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>
#include <drogon/utils/coroutine.h>
#include <format>
#include <glaze/json.hpp>
#include <glaze/json/write.hpp>
#include <print>
#include <stdexcept>
#include <string>
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
template <typename ResponseFormat>
drogon::Task<std::pair<ResponseFormat, std::string>>
ShopifyClient::graphql(const std::string_view query,
                       const glz::generic &variables) {

    // Check if token is valid, we get it from here as it is cleanr than taking
    // if from the private member
    const auto token = co_await ensureAccessToken();

    const auto baseUrl{"https://" + m_shopDomain};
    // Get path from .env variable too
    const auto path{"/admin/api/2026-07/graphql.json"};

    // Set HTTP client
    auto client{drogon::HttpClient::newHttpClient(baseUrl)};

    glz::generic body = {{"query", query}, {"variables", variables}};
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

/* Fetch Product Data
 * @param Product Reference: "124412_001"
 */
drogon::Task<FetchProductDataResponse>
ShopifyClient::fetchProductData(const std::string_view product) {
    auto [res, resJson] = co_await graphql<FetchProductDataResponse>(articleSearchQuery, {{"searchQuery", std::format("product_type:'{}'", product)}});
    co_return res;
}

drogon::Task<std::string> ShopifyClient::ensureAccessToken() {
    if (hasValidToken()) {
        co_return m_accessToken;
    }
    co_return co_await getAccessToken();
}

drogon::Task<std::string> ShopifyClient::getAccessToken() {

    const auto baseUrl = "https://" + m_shopDomain;
    auto client = drogon::HttpClient::newHttpClient(baseUrl);

    const auto body = std::format(
        "grant_type=client_credentials&client_id={}&client_secret={}",
        m_clientId, m_clientSecret);

    auto request = drogon::HttpRequest::newHttpRequest();
    request->setMethod(drogon::Post);
    request->setPath("/admin/oauth/access_token");
    request->setContentTypeCode(drogon::CT_APPLICATION_X_FORM);
    request->setBody(body);

    auto response = co_await client->sendRequestCoro(request);

    if (!response) {
        throw std::runtime_error("Shopify token request failed");
    }

    auto status = response->getStatusCode();

    if (status < drogon::HttpStatusCode::k200OK ||
        status >= drogon::HttpStatusCode::k300MultipleChoices) {
        throw std::runtime_error(
            "Shopify request returned a non2-xxx status, maybe wrong tokens "
            "used");
    }

    AccessTokenResult res;

    const auto currTime = std::chrono::system_clock::now();
    const auto localTime = std::chrono::zoned_time{"Europe/Madrid", currTime};

    std::println("{} INFO: Requested token for store {}", localTime,
                 m_storeName);

    if (glz::read_json(res, response->getBody())) {
        throw std::runtime_error("Invalid Shopify token to JSON");
    }

    m_accessToken = res.access_token;
    m_tokenExpiresAt = std::chrono::steady_clock::now() +
                       std::chrono::seconds{res.expires_in} -
                       std::chrono::minutes{5};

    co_return m_accessToken;
};
} // namespace sapify
