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
#include <glaze/json/generic_fwd.hpp>
#include <glaze/json/write.hpp>
#include <mutex>
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
    if (storeName != "mascaro" && storeName != "prettyballerinas") {
        throw std::runtime_error("Store Name is not correct, it has to be or "
                                 "mascaro or prettyballerinas");
    }
}

/* Fetch Product Data
 * @param Product Reference: "124412_001"
 */
drogon::Task<FetchProductDataResponse>
ShopifyClient::fetchProductData(const std::string_view product) {

    SearchQuery query{.searchQuery = std::format("product_type:'{}'", product)};
    auto [res, resJson] =
        co_await graphql<FetchProductDataResponse, SearchQuery>(
            articleSearchQuery, query);
    co_return res;
}

drogon::Task<std::string> ShopifyClient::ensureAccessToken() {
    // A class mutex to avoid multiple token request
    std::lock_guard lock{m_tokenMutex};
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

// Get Metafield objects for color range object
drogon::Task<std::unordered_map<std::string, std::string>>
ShopifyClient::getColorRangeObjects() {
    std::string_view COLOR_RANGE_OBJECT_QUERY = R"(
           query {
             metaobjectDefinition(id: "gid://shopify/MetaobjectDefinition/19368542552") {
               type
               metaobjects(first: 250) {
                 nodes { id handle displayName }
               }
             }
           }
           )";

    auto [res, resString] = co_await graphql<MetaobjectDefinitionResponse>(COLOR_RANGE_OBJECT_QUERY);

    // Define return map
    std::unordered_map<std::string, std::string> metafieldObjects;

    // This should never be empty, but if it is, we just return empty map
    if (!res.data || !res.data->metaobjectDefinition) {
        co_return {};
    }

    for (auto& object : res.data->metaobjectDefinition->metaobjects.nodes) {
        metafieldObjects.emplace(std::move(object.handle), std::move(object.id)); // Handle returns blancos, rosados..
    }

    co_return metafieldObjects;
}
} // namespace sapify
