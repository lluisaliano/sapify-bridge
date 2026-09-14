#pragma once

#include <nlohmann/json_fwd.hpp>
#include <string_view>

namespace sapify {

class ShoppifyClient {
public:
  ShoppifyClient(std::string_view shopDomain, std::string_view accessToken, std::string_view storeName);
  nlohmann::json graphql(const std::string &query,
                         const nlohmann::json &variables) const;

private:
  std::string m_shopDomain{};
  std::string m_accessToken{};
  std::string m_storeName{};
};
} // namespace sapify
