#pragma once

#include <cstdint>
#include <string>

namespace sapify {
// Server config
struct Config {
    std::string host;
    std::uint16_t port;
    std::string logLevel;
    std::string mascaroDomain;
    std::string mascaroClientId;
    std::string mascaroClientSecret;
    std::string shopifyApiVersion;
    std::string apiSAP;
    std::string picturesBaseURL;

    // Get config from environment variables
    static const Config fromEnv();
};

inline const auto config = sapify::Config::fromEnv();

} // namespace sapify
