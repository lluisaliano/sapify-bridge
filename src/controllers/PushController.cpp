#include "controllers/PushController.hpp"
#include "config/Config.hpp"
#include "controllers/types/PushTypes.hpp"
#include "db/CloudImages.hpp"
#include "services/types/Queryies.hpp"
#include "utils/Media.hpp"
#include "utils/Text.hpp"

#include <drogon/HttpClient.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>
#include <drogon/utils/coroutine.h>
#include <glaze/json/lazy.hpp>
#include <optional>
#include <stdexcept>
#include <string>

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

    // MOVE THIS SOMEWHERE ELSE
    auto responseShopy = drogon::HttpResponse::newHttpResponse();

    for (auto &itemCab : cab) {
        auto response = co_await m_client->fetchProductData(itemCab.ARTICULO);
        // If item exists, there will be only one node, as we have a unique
        // product type
        auto nodes = response.data.products.nodes;
        auto itemShopy = nodes[0];
        // If nodes is not empty items does exist
        bool itemExists = !nodes.empty();

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
        text::trim(title);
        // Format title to Title Case and move it
        inputValues.title = std::move(text::titleCase(title));

        // U_GSP_REFERENCE
        inputValues.productType = itemCab.ARTICULO;

        // ---------------- AQUI HAY QUE PONER TAGS HTML PARA SEO
        inputValues.descriptionHtml = itemCab.DESCRIPCION_LARGA;
        inputValues.vendor = "Mascaro";

        // Handle
        // Convert title to the form hola-adeu-test
        inputValues.handle = text::handleText(title);
        inputValues.redirectNewHandle = true; // Forces to redirect all handles

        // Default upload of items set to Active
        using enum UploadItemVariables::ProductStatus;
        inputValues.status = ACTIVE;

        // For each talla, create a variant, called productOption on shopify
        // Define Variants Names
        UploadItemVariables::ProductOption tallas;
        // Define Variants Data
        std::vector<UploadItemVariables::Variant> variants;

        // Tallas name
        tallas.name = UploadItemVariables::DEFAULT_PRODUCT_OPTIONS;
        for (auto &d : itemDet) {
            // Fill variant names
            auto normaliedTalla = tallas::normalizeTalla(d.TALLA);
            tallas.values.emplace_back(normaliedTalla);

            // Fill variant data
            std::vector<UploadItemVariables::VariantOptionValue>
                variantOptionValues;
            variantOptionValues.push_back({
                .optionName =
                    std::string{UploadItemVariables::DEFAULT_PRODUCT_OPTIONS},
                .name = normaliedTalla,
            });
            auto price = std::format(
                "{}", itemCab.PVP); // Precio Peninsula. Dicho por Cristian
            UploadItemVariables::Variant variant{
                .optionValues = variantOptionValues,
                .sku = d.ARTICULO + "_" + d.TALLA,
                .price = price,
                .compareAtPrice = price, // Mismo precio
                .id = std::nullopt // No hace falta, porque el matching es con
                                   // los optionValues
            };
            variants.push_back(variant);
        }

        // Place data into the final object
        inputValues.productOptions.push_back(std::move(tallas));
        inputValues.variants = std::move(variants);

        // Media
        // Check if media exists
        auto result = co_await getCloudImagesData(itemCab.ARTICULO);

        if (!result) {
            std::println("[ERROR]: Item {} could not be pushed",
                         itemCab.ARTICULO);
            continue;
        }

        // Get media from cloudImages DB
        auto &mediaMap = result.value();
        std::vector<UploadItemVariables::File> files;

        // Check if shopify already has images
        // -------------- REVIEW THAT THIS MAP IS FILLED CORRECTLY
        auto shopyImagesMap = getShopyIdFromMedia(itemShopy);
        // We assume that there are at least 3 images
        for (auto &[variant, exists] : mediaMap) {
            if (!exists)
                continue;
            addImages(files, shopyImagesMap, itemCab, variant);
        }

        // ADD METAFIELDS!!!

        // This may get copy ellision with lucky!
        auto [shopyResJson, shopyRes] = co_await m_client->graphql(
            articleUploadQuery, uploadProductsVariables);

        responseShopy->setContentTypeCode(
            drogon::ContentType::CT_APPLICATION_JSON);
        responseShopy->setBody(std::move(shopyRes));
        // REMOVE BREAK
        break;
    }
    // FALTARA SUBIR IDIOMAS
    // TESTING CHANGING PRODUCT NAME ONLY FROM SHOPIFY
    // String! means that it is mandatory
    // ABSTRACT

    // const auto it =
    // shopyResJson["data"]["products"]["nodes"][0]["id"].get<std::string>();

    co_return responseShopy;
    // build shopify query "bulkOperationRunMutation"
    // "productVariantsBulkCreate" send return response
}
} // namespace sapify
