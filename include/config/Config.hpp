#pragma once

#include <cstdint>
#include <string>

namespace sapify {
    // Server config
    struct Config {
        std::string host;
        std::uint16_t port;
        std::string logLevel;

        // Get config from environment variables
        static Config fromEnv();
    };
}
