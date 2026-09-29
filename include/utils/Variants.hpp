#pragma once

#include "services/types/ShopifyClientTypes.hpp"
#include <unordered_map>

namespace sapify {
namespace variantsUtils {
using ItemShopy = FetchProductDataResponse::Data::Products::ProductNode;
[[nodiscard]] inline std::unordered_map<std::string, std::string>
buildMapWithVariantGID(const ItemShopy &itemShopy) {
    std::unordered_map<std::string, std::string> resultMap;

    const auto &variants = itemShopy.variants.nodes;
    // Reserve memory
    resultMap.reserve(variants.size());

    for (auto &variant : variants) {
        // Check if data is ok
        if (variant.selectedOptions.empty())
            continue;

        // Copy could be avoiding by moving, but maybe we will need this
        // somewhere else and this will not be a pain point
        auto value =
            variant.selectedOptions[0]
                .value; // This gets talla value. 34 or U or L... There is only
                        // one selectedOptions in variants nodes
        auto id = variant.id; // Get variant id

        // Copied strings are moved to the resultmap
        resultMap.emplace(std::move(value), std::move(id));
    }

    return resultMap;
}
} // namespace variants
} // namespace sapify
