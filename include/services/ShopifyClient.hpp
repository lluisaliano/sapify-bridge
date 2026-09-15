#pragma once

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

  drogon::Task<glz::generic> graphql(const std::string &query,
                                     const glz::generic &variables);

  drogon::Task<std::string> ensureAccessToken();

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
