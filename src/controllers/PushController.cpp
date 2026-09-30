#include "controllers/PushController.hpp"
#include "config/Config.hpp"
#include "controllers/types/PushTypes.hpp"
#include "db/CloudImages.hpp"
#include "services/types/Queryies.hpp"
#include "utils/Media.hpp"
#include "utils/Metafields.hpp"
#include "utils/Text.hpp"
#include "utils/Variants.hpp"

#include <drogon/HttpClient.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>
#include <drogon/utils/coroutine.h>
#include <glaze/json/lazy.hpp>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace sapify {
drogon::Task<drogon::HttpResponsePtr>
PushController::pushItems(drogon::HttpRequestPtr req) {

    // Receive items to upload from SuperTCModel. We need to create a string as
    // we only get a string_view from req-body() This recieves a JSON with
    // property articulos wich has an array of strings of articulos
    /*
     * {
     * articulos: ["1231_001",...]
     * }
     */
    const auto reqBody = std::string{req->body()};

    // Fetch API for shopify cab and det data
    const auto baseUrl{"https://" + config.apiSAP};
    auto client = drogon::HttpClient::newHttpClient(baseUrl);

    // Get Cab Data
    auto requestCab = drogon::HttpRequest::newHttpRequest();
    requestCab->setMethod(drogon::Post);
    requestCab->setPath("/shopify/cab"); // API endpoint to get cab items
    requestCab->setContentTypeCode(drogon::ContentType::CT_APPLICATION_JSON);
    requestCab->setBody(
        reqBody); // Send body asking only for corresponding items

    // Get Det Data
    auto requestDet = drogon::HttpRequest::newHttpRequest();
    requestDet->setMethod(drogon::Post);
    requestDet->setPath("/shopify/det"); // API endpoint to get cab items
    requestDet->setContentTypeCode(drogon::ContentType::CT_APPLICATION_JSON);
    requestDet->setBody(
        reqBody); // Send body asking only for corresponding items

    // Send requests
    auto responseCab = co_await client->sendRequestCoro(requestCab);
    auto responseDet = co_await client->sendRequestCoro(requestDet);

    // Check if request are ok
    if (responseCab->getStatusCode() < drogon::k200OK ||
        responseCab->getStatusCode() >= drogon::k300MultipleChoices ||
        responseDet->getStatusCode() < drogon::k200OK ||
        responseDet->getStatusCode() >= drogon::k300MultipleChoices) {
        throw std::runtime_error(std::format(
            "APISAP returned non-2xx code when fetching cab or det data"));
    }

    // Parse data into vectors
    ItemsCab cab;
    ItemsDet det;
    if (glz::read_json(cab, responseCab->getBody()) ||
        glz::read_json(det, responseDet->getBody())) {
        throw std::runtime_error{
            "Error while parsing APISAP Cab and Det Jsons"};
    }

    // MOVE THIS SOMEWHERE ELSE, THIS IS CURRENTLY USED TO SET A RESPONSE FOR
    // THE HTTP POST OF THE USER, if updating multiple articles, it should
    // return an array with status and errors for each product The
    // initialization of the httpresponse can be left here but should be updated
    // with each product and send after the for loop
    auto responseShopy = drogon::HttpResponse::newHttpResponse();

    for (auto &itemCab : cab) {
        auto response = co_await m_client->fetchProductData(itemCab.ARTICULO);
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
        // Using C++ views to get detItems
        auto itemDet = det | std::views::filter([&](ItemDet &elem) {
                           return elem.ARTICULO == itemCab.ARTICULO;
                       });

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
        // Loop through each talla coming from itemDet, itemDet come from ean,
        // where empty eans are not returned If new eans are added, we just
        // update them
        for (auto &d : itemDet) {
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
                if (auto it = existingVariantsGidMap.find(normalizedTalla);
                    it != existingVariantsGidMap.end()) {
                    id = it->second;
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
        // Check if media exists, this returns a std::optional
        auto cloudImagesData = co_await getCloudImagesData(itemCab.ARTICULO);

        if (!cloudImagesData) {
            std::println("[ERROR]: Item {} could not be pushed because there "
                         "are no images",
                         itemCab.ARTICULO);
            continue;
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

        // We assume that there are at least 3 images for the selected items, otherwise this will add no images
        for (auto& name : imageOrder) {
           auto it = (*cloudImagesData).find(name);
           // If variant is found (which should always be true) and exists is true, we add it
           if (it != (*cloudImagesData).end() && it->second) {
               addImages(files, itemCab, it->first);
           }
        }

        // Add created files to inputValue
        inputValues.files = std::move(files);

        // This may get copy ellision with lucky!
        // UploadItem
        auto [shopyRes, shopyResString] =
            co_await m_client->graphql<ProductSetResponse>(
                articleUploadQuery,
                uploadProductsVariables); // This does deduction of course

        // --- Upload metafields
        // We need to do so with the just created or modified product id
        // Metafields, we will use a different mutation to load metafields, so
        // existing ones are not overwritten
        std::string ns =
            m_client->getNameSpace(); // Get namepsace, this function creates a
                                      // copy of the string
        std::string id = shopyRes.data.productSet.product
                             .id; // Get Product Id, no matter if it is the old
                                  // one or a new one

        // For the moment, we only have a metafield object for
        // shoe_color_ranges_object This is stored in a map with lowerCase
        // letters and values are ids we put to create metafields
        auto shoeColorRangesMetaObjects =
            co_await m_client->getColorRangeObjects(); // Keys are lowercased,
                                                       // rojos, naranjas...

        // Initialize metafields
        Metafields metafields{metafieldsUtils::createMetafieldsFromSAP(
            ns, itemCab, id, shoeColorRangesMetaObjects)};
        // Do the update
        auto [shopyResMetafields, shopyResMetafieldsString] =
            co_await m_client->graphql(articleMetafieldsQuery, metafields);
        // ---

        responseShopy->setContentTypeCode(
            drogon::ContentType::CT_APPLICATION_JSON);
        responseShopy->setBody(std::move(shopyResString));
        // REMOVE BREAK
        break;
    }
    // FALTARA SUBIR IDIOMAS
    // TESTING CHANGING PRODUCT NAME ONLY FROM SHOPIFY
    // String! means that it is mandatory

    co_return responseShopy;
    // --------- IMPORTANT build shopify query "bulkOperationRunMutation" to
    // avoid rate LImits "productVariantsBulkCreate" send return response
}
} // namespace sapify
