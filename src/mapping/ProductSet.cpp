#include "mapping/ProductSet.hpp"

#include "db/CloudImages.hpp"
#include "utils/Media.hpp"
#include "utils/Text.hpp"
#include "utils/Variants.hpp"

#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace sapify::productSet {

drogon::Task<UploadItemsSetVariables> getProductSet(
    ShopifyClient &shopifyClient, ItemCab &itemCab,
    const metafieldsUtils::shoeColorRangeObjects &,
    const std::unordered_map<std::string, ItemsDet> &detMap) {
    auto response = co_await shopifyClient.fetchProductData(itemCab.ARTICULO);
    // If item exists, there will be only one node, as we have a unique
    // product type
    auto nodes = response.data.products.nodes;
    // If nodes is not empty items does exist
    bool itemExists = !nodes.empty();
    // If item exists, keep its data in itemShopy
    FetchProductDataResponse::Data::Products::ProductNode itemShopy;
    if (itemExists) {
        itemShopy = nodes[0];
    }

    // Create item to upload
    UploadItemsSetVariables uploadProductsVariables{};

    uploadProductsVariables.synchronous =
        true; // We leave this to true for the moment

    uploadProductsVariables.identifier =
        itemExists ? std::optional{ProductSetIdentifier{itemShopy.id}}
                   : std::nullopt;

    auto &inputValues = uploadProductsVariables.input;

    //------------------------REVIEW!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    std::string title =
        std::format("{} {} {} {}", itemCab.TEMA, itemCab.NOMBRE_HORMA,
                    itemCab.MATERIAL, itemCab.COLOR);
    // Trim text
    textUtils::trim(title);
    // Format title to Title Case and move it
    inputValues.title = std::move(textUtils::titleCase(title));

    // U_GSP_REFERENCE
    inputValues.productType = itemCab.ARTICULO;

    // ---------------- AQUI HAY QUE PONER TAGS HTML PARA SEO (strong..)
    inputValues.descriptionHtml = itemCab.DESCRIPCION_LARGA.value_or(
        ""); // If it does not exist, send "" we could use std::optional,
             // but
    //-----------IMPORTANTE: Vendor depende de tienda
    inputValues.vendor = "Mascaro";

    // Handle
    // Convert title to the form hola-adeu-test
    inputValues.handle = textUtils::handleText(inputValues.title);
    inputValues.redirectNewHandle = true; // Forces to redirect all handles

    // Default upload of items set to Active
    using enum UploadItemVariables::ProductStatus;
    inputValues.status = DRAFT;

    // For each talla, create a variant, called productOption on shopify

    // Define Variants Names
    UploadItemVariables::ProductOption tallasOptions;
    // Define Variants Data
    std::vector<UploadItemVariables::Variant> variants;

    // Build map of existing variants and its GIDs to pass them if they
    // exist
    std::unordered_map<std::string, std::string> existingVariantsGidMap;
    if (itemExists) {
        existingVariantsGidMap =
            variantsUtils::buildMapWithVariantGID(itemShopy);
    }

    // Tallas name
    tallasOptions.name = UploadItemVariables::DEFAULT_PRODUCT_OPTIONS;
    // Check if item has tallas coming from eans:
    auto itDet = detMap.find(itemCab.ARTICULO);
    if (itDet == detMap.end()) {
        std::println("[ERROR]: Item  has no tallas", itemCab.ARTICULO);
        throw SkipProductSet{};
    }
    // Loop through each talla coming from itemDet, itemDet come from ean,
    // where empty eans are not returned If new eans are added, we just
    // update them
    for (auto &d : itDet->second) {
        // Fill variant names
        auto normalizedTalla = tallasUtils::normalizeTalla(d.TALLA);
        tallasOptions.values.emplace_back(normalizedTalla);

        // Fill variant data
        std::vector<UploadItemVariables::VariantOptionValue>
            variantOptionValues;
        variantOptionValues.push_back({
            .optionName =
                std::string{UploadItemVariables::DEFAULT_PRODUCT_OPTIONS},
            .name = normalizedTalla,
        });
        auto price = std::format(
            "{}", itemCab.PVP); // Precio Peninsula. Said by Cristian

        // Check if talla already existed on shopify, if itemShopy did not
        // exist, this just gives a nullopt
        std::optional<std::string> id;
        if (itemExists) {
            if (auto itGid = existingVariantsGidMap.find(normalizedTalla);
                itGid != existingVariantsGidMap.end()) {
                id = itGid->second;
            }
        }

        UploadItemVariables::Variant variant{
            .optionValues = variantOptionValues,
            .sku = d.ARTICULO + "_" +
                   d.TALLA, // SKU is built with not normalized talla
            .price = price,
            .compareAtPrice = price, // Mismo precio
            // If Talla exists, assign id, otherwise, nullopt to create it
            .id = id // If variant exists, we have to pass
                     // it the id, otherwise a new variant
                     // that replaces the old one replaces
                     // it deleting its relation with the
                     // stock and old buy orders
        };
        variants.push_back(variant);
    }

    // Place data into the final object
    inputValues.productOptions.push_back(std::move(tallasOptions));
    inputValues.variants = std::move(variants);

    // Media
    // Check if media exists, this returns a std::optional, if no media
    // exists, skip item
    auto cloudImagesData = co_await getCloudImagesData(itemCab.ARTICULO);

    if (!cloudImagesData) {
        std::println("[ERROR]: Item {} could not be pushed because there "
                     "are no images",
                     itemCab.ARTICULO);
        throw SkipProductSet{};
    }

    // Set files vector to fill and send on the shopify request to upload
    // item
    std::vector<UploadItemVariables::File> files;

    // Define Image order
    std::vector<std::string_view> imageOrder;

    // Generos mapping
    const std::string_view BOLSOS = "4";
    const std::string_view ABRIGOS = "3";
    const std::string_view CARTERAS = "8";

    // Bolsos Order
    if (itemCab.GENERO == BOLSOS || itemCab.GENERO == ABRIGOS ||
        itemCab.GENERO == CARTERAS) {
        imageOrder = {"side", "pers", "top", "back", "lateral", "det"};
    } else {
        // Normal Order
        imageOrder = {"pers", "top", "side", "back", "det"};
    }

    // We assume that there are at least 3 images for the selected items,
    // otherwise this will add no images
    for (auto &name : imageOrder) {
        // Loop going through given order, and then add the corresponding
        // image to the final object
        auto it = (*cloudImagesData).find(name);
        // If variant is found (which should always be true) and exists is
        // true, we add it
        if (it != (*cloudImagesData).end() && it->second) {
            addImages(files, itemCab, it->first);
        }
    }

    // Add created files to inputValue
    inputValues.files = std::move(files);

    co_return uploadProductsVariables;
}

} // namespace sapify::productSet
