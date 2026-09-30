#pragma once

#include <cstddef>
#include <string_view>
namespace sapify {
    // Implement heterogeneous lookup
    struct StringHash {
        using is_transparent = void;

        // Accept string_view to hash all keys equally. First we have to create the string_view which is fast
        std::size_t operator()(std::string_view text) const noexcept {
           return std::hash<std::string_view>{}(text);
        }
    };
}
