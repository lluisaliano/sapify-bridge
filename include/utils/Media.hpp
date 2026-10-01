#pragma once

#include "controllers/types/PushTypes.hpp"

namespace sapify {
/*
 * This recieves:
 * @param files vector to fill the data
 * @param shopyImagesMap map with variant as key, and shopify media id with
 * value
 * @param itemCab data
 * @param variant image variant
 */
inline void addImages(std::vector<UploadItemVariables::File> &files,
                      ItemCab &itemCab, const std::string &variant) {

    // -- Image does not have to be checked, it can just be added again
    // Check if image already existed on shopify
    // auto it = shopyImagesMap.find(variant);
    // // If so add its id, otherwise add nullopt
    // std::optional<std::string> id =
    //     it != shopyImagesMap.end() ? std::optional{it->second} :
    //     std::nullopt;

    // Create image URL
    std::string source =
        std::format("{}/{}/{}_{}.jpg", UploadItemVariables::PICTURES_BASE_URL,
                    itemCab.ARTICULO, itemCab.ARTICULO, variant);
    // ---------------- ADD ALT!!
    // Add it to files
    files.push_back({
        .filename = std::format("{}_{}.jpg", itemCab.ARTICULO, variant), // Important, keep .jpg or this fail! Change name for SEO porpuses
        .originalSource = std::move(source),
        .alt = "",
        .contentType = "IMAGE",
    });
}
} // namespace sapify
