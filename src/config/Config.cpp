#include "config/Config.hpp"
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>

namespace sapify {

// Block external linking
namespace {
// Get env variable or return fallback
std::string getEnv(std::string_view name, std::string_view fallback) {
  // Get env needs null terminated strings
  const char *value{std::getenv(name.data())};

  if (value == nullptr || std::string_view{value}.empty())
    return std::string{fallback};

  return value;
}

// Parse port number
std::uint16_t parsePort(const std::string &value) {
  std::size_t parsedChars{0};
  const auto port{std::stoul(value, &parsedChars)};

  if (parsedChars != value.size() ||
      port > std::numeric_limits<std::uint16_t>::max()) {
    throw std::runtime_error("Invalid APP_PORT value " + value);
  }

  return static_cast<std::uint16_t>(port);
}

// This will only be use on dev environments, because docker exports envs
// globally and they will be catched with environ
void loadDotEnv(const std::string &envPath) {
  std::ifstream file(envPath);

  // If file does not exist, we just return
  if (!file) {
    return;
  }

  for (std::string line; std::getline(file, line);) {
    // Discard empty lines or comments
    if (line.empty() || line[0] == '#') {
      continue;
    }

    const auto separator = line.find('=');
    // If there is no separator
    if (separator == std::string::npos) {
      continue;
    }

    // Get Key and value (does not include separator)
    const auto key = line.substr(0, separator);
    // If not second arg, we get till end of line
    const auto value = line.substr(separator + 1);
    // If key does exist, we set the value
    if (!key.empty()) {
      // If env already exists return
      if (std::getenv(key.c_str()) != nullptr) {
        return;
      }
      // Else set new env, with 0 we set non replace
      setenv(key.c_str(), value.c_str(), 0);
    }
  }
}

} // namespace

// Get environment variables to create the config object
Config Config::fromEnv() {
  // We will load .env if it exists, which will only exist in dev environments
  loadDotEnv(".env");
  return {.host = getEnv("APP_HOST", "0.0.0.0"),
          .port = parsePort(getEnv("APP_PORT", "8080")),
          .logLevel = getEnv("LOG_LEVEL", "info")};
}

} // namespace sapify
