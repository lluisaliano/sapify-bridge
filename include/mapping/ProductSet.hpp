#pragma once

#include "controllers/types/PushTypes.hpp"
#include "services/ShopifyClient.hpp"
#include "utils/Metafields.hpp"

#include <drogon/utils/coroutine.h>
#include <expected>
#include <unordered_map>

// Mapping of SAP Product to Shopify
namespace sapify::productSet {

//  ALSO: Add userErrors {code, field, message} to shopify responses
// Define Errors here of mutation
enum class ProductSetError {
    Continue,
};

// Result Type
using ProductSetResult =
    std::expected<UploadItemsSetVariables, ProductSetError>;

// Mapping Function
drogon::Task<ProductSetResult> getProductSet(
    ShopifyClient &shopifyClient, ItemCab &itemCab,
    const metafieldsUtils::shoeColorRangeObjects &shoeColorRangesMetaObjects,
    const std::unordered_map<std::string, ItemsDet> &detMap);

} // namespace sapify::productSet
