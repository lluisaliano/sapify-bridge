#pragma once

#include <algorithm>
#include <cctype>
#include <charconv>
#include <format>
#include <string>
#include <string_view>
#include <unordered_map>
#include <ranges>

namespace sapify {
namespace text {
// Convert text to Title Case
inline std::string &titleCase(std::string &string) {
    bool newWord{true};
    // We have to cast to unsigned char to use isspace. toupper and tolower
    // returns an int, so we have to cast This transforms in place
    std::ranges::transform(
        string, string.begin(), [&newWord](unsigned char c) -> char {
            if (std::isspace(c)) {
                newWord = true;
                return static_cast<char>(c);
            } else {
                auto result = newWord ? std::toupper(c) : std::tolower(c);

                newWord = false;
                return static_cast<char>(result);
            }
        });

    return string;
}

// This generates handles URL. We should keep track of colisions... or let it to
// shopify
// We use utf handling here to convert cases to ascii
inline std::string handleUTF(std::string_view string) {
    static const std::unordered_map<std::string_view, char> map = {
        {"á", 'a'}, {"é", 'e'}, {"í", 'i'}, {"ó", 'o'}, {"ú", 'u'},
        {"Á", 'a'}, {"É", 'e'}, {"Í", 'i'}, {"Ó", 'o'}, {"Ú", 'u'},
        {"ñ", 'n'}, {"Ñ", 'n'}, {"ü", 'u'}, {"Ü", 'u'},
    };
    std::string out;
    out.reserve(string.size());

    for (auto i{0uz}; i < string.size();) {
        unsigned char c = static_cast<unsigned char>(
            string[i]); // We cast to unsigned char because UTF stores
                        // characters using values greater than 128 which are
                        // not negative
        std::size_t len =
            (c < 0x80) ? 1 : 2; // If len == 2, then we have a utf char.
        auto it = map.find(string.substr(i, len)); // Get
        // Map char to corresponding lower case ASCII
        if (it != map.end()) {
            out += it->second;
            // Return normally
        } else if (len == 1) {
            out += static_cast<char>(c);
            // In case len == 2 and char is not mapped, we just add it hoping
            // the best...
        } else {
            out += static_cast<char>(c);
        }
        // Increase loop depending on size
        i += len;
    }

    // This will move if it can not copy ellide
    return out;
}

inline void trim(std::string& string) {
    // Define function to check if char is a space
    auto isSpace = [](unsigned char c){return std::isspace(c);};

    string.erase(string.begin(), std::ranges::find_if_not(string, isSpace));
    // find if not searches the string backwards using string | std::views::reverse. It find the first character which is not a space.
    // .base() returns an non reversed iterator to one character forward this one
    // "hola   " find if not returns iterator to a. But it is reversed. .base returns an iterator to a + 1 which is not reversed
    string.erase(std::ranges::find_if_not(string | std::views::reverse, isSpace).base(), string.end());
}

inline std::string handleText(std::string_view string) {
    std::string asciiText = handleUTF(string);
    std::string handle;
    // Get capacity
    handle.reserve(asciiText.size());

    // Use unsigned char as std <cctype> require it to operate
    for (auto c : asciiText) {
        unsigned char cleanChar = static_cast<unsigned char>(c);
        // If we have not a space, cast it to lower case
        if (!std::isspace(cleanChar)) {
            handle += static_cast<char>(std::tolower(cleanChar));
            // If it is a space
        } else {
            // Add the hyphen
           handle += '-';
        }
    }

    // If we added something and last char is a '-', we take it out
    if (!handle.empty() && handle.back() == '-') {
        handle.pop_back();
    }

    return handle;
}
} // namespace text

namespace tallas {
// This functions converts tallas from 360 to 36 and 365 to 36.5
inline std::string normalizeTalla(std::string_view talla) {
    // Returns U
    if (talla == "U") {
        return "U";
    }
    // Convert to integer
    int value;
    std::from_chars(talla.data(), talla.data() + talla.size(), value);
    // Normal Talla,  returns 35.5
    if (value > 100) {
        return std::format("{:.1f}", value / 10.0);
        // Kid talla, just returns 25, 26...
    } else {
        return std::string{talla};
    }
}
}
} // namespace sapify
