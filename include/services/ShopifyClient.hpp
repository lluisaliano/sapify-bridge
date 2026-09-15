#pragma once

#include <chrono>
#include <glaze/core/feature_test.hpp>
#include <glaze/json/generic_fwd.hpp>
#include <string_view>
#include <glaze/json.hpp>

namespace sapify {

class ShoppifyClient {
public:
  ShoppifyClient(std::string_view shopDomain, std::string_view storeName, std::string_view clientId, std::string_view clientSecret, std::string_view apiVersion);
  glz::generic graphql(const std::string &query,
                         const glz::generic &variables) const;

  // Compute AccesTOken if needed
  void ensureAccessToken() const {
    if (hasValidToken()) {
      return;
    }
    getAccessToken();
  }

private:
  std::string m_shopDomain{};
  std::string m_storeName{};
  std::string m_clientId{};
  std::string m_clientSecret{};
  std::string m_apiVersion{};
  mutable std::string m_accessToken{};
  mutable std::chrono::system_clock::time_point m_tokenExpiresAt;

private:
  // Get access token from shopify
  void getAccessToken() const;

  // Check if token exists
  bool hasValidToken() const { return !m_accessToken.empty(); }
};
} // namespace sapify
