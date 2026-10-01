#pragma once

#include "controllers/types/PushTypes.hpp"
#include "services/ShopifyClient.hpp"
#include "utils/Metafields.hpp"

#include <drogon/utils/coroutine.h>
#include <unordered_map>

// IMPORTANT! Mapping of product sap to avoid loading logic on PushController.cpp
//  ALSO: Add userErrors {code, field, message} to shopify responses
namespace sapify::productSet {

class SkipProductSet {};

drogon::Task<UploadItemsSetVariables> getProductSet(
    ShopifyClient &shopifyClient, ItemCab &itemCab,
    const metafieldsUtils::shoeColorRangeObjects &shoeColorRangesMetaObjects,
    const std::unordered_map<std::string, ItemsDet> &detMap);

} // namespace sapify::productSet
