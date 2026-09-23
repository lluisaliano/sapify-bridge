#pragma once

#include <string>
#include <vector>

namespace sapify {
// Fetch Product Data Response Type
struct FetchProductDataResponse {
    struct Data {
        struct Products {
            struct ProductNode {
                struct Media {
                    struct MediaNode {
                        struct Preview {
                            struct Image {
                                std::string url{};
                            };
                            Image image{};
                        };

                        std::string id{};
                        std::string alt{};
                        std::string mediaContentType{};
                        Preview preview{};
                    };
                    std::vector<MediaNode> nodes{};
                };

                struct Variants {
                    struct VariantNode {
                        std::string id{};
                        std::string sku{};
                    };
                    std::vector<VariantNode> nodes{};
                };

                std::string id{};
                std::string title{};
                std::string productType{};
                Media media{};
                Variants variants{};
            };
            std::vector<ProductNode> nodes{};
        };
        Products products{};
    };
    Data data{};
};

// Used to do a query request/mutation on graphql method
template <typename T>
struct ShopifyRequest {
    std::string_view query;
    T variables;
};

// Used to find items on shopify on fetchProductData method
struct SearchQuery {
    std::string searchQuery;
};
} // namespace sapify
