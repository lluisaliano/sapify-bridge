#pragma once

#include <string_view>

namespace sapify {

// Query used to get product data
// searchQuery uses productType which contains article
constexpr std::string_view articleSearchQuery = R"(
  query GetProductByArticle($searchQuery: String!) {
    products(first: 3, query: $searchQuery) {
      nodes {
        id
        title
        productType

        media(first: 20) {
          nodes {
            id
            alt
            mediaContentType
            preview {
              image {
                url
              }
            }
          }
        }

        variants(first: 50) {
          nodes {
            id
            sku
            selectedOptions {
              name
              value
            }
            inventoryItem {
              id
            }
          }
        }
      }
    }
  }
)";

// First is only used for the response after update
constexpr std::string_view articleUploadQuery = R"(
  mutation UpsertProductFull(
    $input: ProductSetInput!
    $identifier: ProductSetIdentifiers
    $synchronous: Boolean
  ) {
    productSet(
      input: $input
      identifier: $identifier
      synchronous: $synchronous
    ) {
      product {
        id
        title
        productType
        vendor
        descriptionHtml
        status
        handle
        seo {
          title
          description
        }
        metafields(first: 20) {
          nodes {
            namespace
            key
            value
          }
        }
        media(first: 20) {
          nodes {
            id
            alt
            mediaContentType
            preview {
              image {
                url
              }
            }
          }
        }
        variants(first: 50) {
          nodes {
            id
            sku
            title
            price
            compareAtPrice
            inventoryQuantity
          }
        }
      }
      productSetOperation {
        id
        status
        userErrors {
          code
          field
          message
        }
      }
      userErrors {
        field
        message
        code
      }
    }
  }
)";

constexpr std::string_view articleMetafieldsQuery = R"(
  mutation SetProductMetafields($metafields: [MetafieldsSetInput!]!) {
    metafieldsSet(metafields: $metafields) {
      metafields {
        id
        namespace
        key
        value
        type
      }
      userErrors {
        field
        message
        code
      }
    }
  }
)";

} // namespace sapify
