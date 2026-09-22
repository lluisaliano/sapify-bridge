#pragma once

#include "controllers/types/PushTypes.hpp"
#include "services/types/ShopifyClientTypes.hpp"
#include <algorithm>
#include <ranges>
#include <unordered_map>

namespace sapify {

inline std::unordered_map<std::string, std::string> getShopyIdFromMedia(
    FetchProductDataResponse::Data::Products::ProductNode &itemShopy) {
    auto &nodes = itemShopy.media.nodes;

    std::unordered_map<std::string, std::string> shopyHasMedia;

    for (auto &node : nodes) {
        std::string url = std::move(node.preview.image.url);
        std::string id = node.id;
        // This following code assumes that shopify images filename do not
        // contain any _ or . before the desired ones
        auto reversed = url | std::views::reverse;
        auto underScore =
            std::ranges::find(reversed, '_');        // Has _ before variant
        auto dot = std::ranges::find(reversed, '.'); // Ends with .jpg

        if (underScore == reversed.end() || dot == reversed.end()) {
            continue;
        }

        auto start = underScore.base();
        auto end = dot.base() - 1;

        std::string newString(start, end);
        shopyHasMedia.insert(std::move(newString), std::move(id));
    }

    return shopyHasMedia;
}

inline void
addImages(std::vector<UploadItemVariables::File> &files,
          std::unordered_map<std::string, std::string> &shopyImagesMap,
          ItemCab &itemCab, const std::string &variant) {

    // Check if image already existed on shopify
    auto it = shopyImagesMap.find(variant);
    std::optional<std::string> id =
        it != shopyImagesMap.end() ? std::optional{it->second} : std::nullopt;
    std::string source = std::format(
        "{}/{}/{}_{}.jpg/products", UploadItemVariables::PICTURES_BASE_URL,
        itemCab.ARTICULO, itemCab.ARTICULO, variant);
    // ---------------- ADD ALT!!
    // Add it to files
    files.push_back(
        {.id = id,
         .filename = std::format("{}_{}", itemCab.ARTICULO, variant),
         .originalSource = std::move(source),
         .alt = "",
         .contentType = "IMAGE",
         .duplicateResolutionMode = "REPLACE"});
}
} // namespace sapify
