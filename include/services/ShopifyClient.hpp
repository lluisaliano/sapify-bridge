#pragma once

#include <chrono>
#include <functional>
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
  glz::generic graphql(const std::string &query, const glz::generic &variables);

  using TokenSuccessCallback = std::function<void(std::string)>;
  using ErrorCallback = std::function<void(std::string)>;

  // Compute AccesTOken if needed
  void ensureAccessToken() {
    if (hasValidToken()) {
      return;
    }
    getAccessToken();
  }

  void ensureAccessTokenAsync(TokenSuccessCallback onSuccess,
                              ErrorCallback onError);

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
  // Get access token from shopify
  void getAccessToken();
  // This function is for testing only
  void getAccessTokenAsync(TokenSuccessCallback onSuccess,
                           ErrorCallback onError);

  // Check if token exists
  bool hasValidToken() const {
    return !m_accessToken.empty() &&
           std::chrono::steady_clock::now() < m_tokenExpiresAt;
  }
};
} // namespace sapify
