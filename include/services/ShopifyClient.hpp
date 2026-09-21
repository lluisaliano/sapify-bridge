#pragma once

#include "services/types/ShopifyClientTypes.hpp"
#include <chrono>
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

    // Launch a graphql query to shopify
    template <typename ResponseFormat = glz::generic>
    drogon::Task<std::pair<ResponseFormat, std::string>>
    graphql(const std::string_view query, const glz::generic &variables);

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
};
} // namespace sapify
