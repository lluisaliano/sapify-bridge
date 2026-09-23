#pragma once

#include "controllers/types/PushTypes.hpp"
#include "services/types/ShopifyClientTypes.hpp"
#include <algorithm>
#include <ranges>
#include <unordered_map>

namespace sapify {

/*
 * This, recieves an item from shopify struct with information and
 * return a map where keys are variants names ("pers", "side"...) and
 * values are shopify Media ids for this variants
 */
inline std::unordered_map<std::string, std::string> getShopyIdFromMedia(
    FetchProductDataResponse::Data::Products::ProductNode &itemShopy) {
    auto &nodes = itemShopy.media.nodes;

    std::unordered_map<std::string, std::string> shopyHasMedia;

    // For each image
    for (auto &node : nodes) {
        // Get the URL, we move it as we do not need it anywhere else
        std::string url = std::move(node.preview.image.url);
        // Get the media ID
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
        shopyHasMedia.insert({std::move(newString), std::move(id)});
    }

    return shopyHasMedia;
}

/*
 * This recieves:
 * @param files vector to fill the data
 * @param shopyImagesMap map with variant as key, and shopify media id with value
 * @param itemCab data
 * @param variant image variant
 */
inline void
addImages(std::vector<UploadItemVariables::File> &files,
          std::unordered_map<std::string, std::string> &shopyImagesMap,
          ItemCab &itemCab, const std::string &variant) {

    // Check if image already existed on shopify
    auto it = shopyImagesMap.find(variant);
    // If so add its id, otherwise add nullopt
    std::optional<std::string> id =
        it != shopyImagesMap.end() ? std::optional{it->second} : std::nullopt;
    // Create image URL
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
