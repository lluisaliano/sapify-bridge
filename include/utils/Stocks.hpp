#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace sapify::stockUtils {

using LocationID = std::string;               // Shopify Location GID
using LocationNameShopify = std::string_view; // Shopify location name
using LocationNameSAP = std::string_view;     // SAP warehouse code
using LocationMap = std::unordered_map<
    LocationNameSAP, std::pair<LocationNameShopify, LocationID>>;

// Update this map if a Shopify location or SAP warehouse is added or removed.
// SAP warehouses without a matching Shopify location are intentionally absent.
inline const LocationMap &getLocationMap () {
    static const LocationMap map{
        {"ALM0C",    {"0C",    "gid://shopify/Location/103819673944"}}, // All stock together to pass to OMS
        // -- Disabled
        {"ALM0CINT", {"0CINT", "gid://shopify/Location/103819673944"}}, // OC + M01 + M02 - Menorca , not used yet...
        {"ALLGD",    {"LGD",   "gid://shopify/Location/103817412952"}}, // Londres
        // -- End Disabled
        // Click and collect stores only
        {"ES001",    {"M01",   "gid://shopify/Location/103820132696"}}, // Ferreries
        {"ES002",    {"M02",   "gid://shopify/Location/103820165464"}}, // Maó
        {"ES012",    {"T12",   "gid://shopify/Location/103819936088"}}, // Hermosilla
        {"ES014",    {"T14",   "gid://shopify/Location/103819968856"}}, // San Sebastian
        {"",         {"T54",   "gid://shopify/Location/103820001624"}}, // Goya
        {"ES058",    {"T58",   "gid://shopify/Location/103820001624"}}, // Born
        {"ES060",    {"T60",   "gid://shopify/Location/103820099928"}}, // Passeig de Gracia
        {"ES067",    {"T67",   "gid://shopify/Location/103820493144"}}, // Ayala
        {"",         {"T68",   "gid://shopify/Location/103820558680"}}, // Illa Diagonal
        {"ES073",    {"T73",   "gid://shopify/Location/103820788056"}}, // Rambla Catalunya
        {"ES078",    {"T78",   "gid://shopify/Location/103819837784"}}, // Unió
    };

    return map;
}

} // namespace sapify::stockUtils
