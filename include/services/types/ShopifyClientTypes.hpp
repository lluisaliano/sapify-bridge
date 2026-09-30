#pragma once

#include <string>
#include <vector>
#include <optional>

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
                    struct SelectedOptions {
                        std::string name{};
                        std::string value{};
                    };
                    struct InventoryItem {
                        std::string id{};
                    };
                    struct VariantNode {
                        std::string id{};
                        std::string sku{};
                        std::vector<SelectedOptions> selectedOptions{};
                        InventoryItem inventoryItem{};
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

// Metafields objects from metafield object definition query
struct MetaobjectDefinitionResponse {
    struct Data {
        struct MetaobjectDefinition {
            struct Metaobjects {
                struct Node {
                    std::string id;
                    std::string handle;
                    std::string displayName;
                };

                struct PageInfo {
                    bool hasNextPage{};
                    std::optional<std::string> endCursor;
                };

                std::vector<Node> nodes;
                PageInfo pageInfo;
            };

            std::string type;
            Metaobjects metaobjects;
        };

        std::optional<MetaobjectDefinition> metaobjectDefinition;
    };

    struct Error {
        std::string message;
    };

    std::optional<Data> data;
    std::optional<std::vector<Error>> errors;
};



} // namespace sapify
