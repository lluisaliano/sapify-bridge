#include "controllers/PushController.hpp"
#include "controllers/types/PushTypes.hpp"
#include "utils/text.hpp"

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
    const auto baseUrl{"https://" + m_config->apiSAP};
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
        // ---------------- IMPORTANT: We need to clean tallas
        for (auto &d : itemDet) {
            tallas::normalizeTalla(d.TALLA);
        }
        UploadItemVariables::ProductOption

            inputValues.productOptions.push_back(
                {.name = UploadItemVariables::DEFAULT_PRODUCT_OPTIONS})
    }

    // FALTARA SUBIR IDIOMAS
    // TESTING CHANGING PRODUCT NAME ONLY FROM SHOPIFY
    // String! means that it is mandatory
    // ABSTRACT

    // Use redirectNewHandle if handle is changed, handle is based on title on
    // first upload. If title is modified, we have to also pass new handle and
    // use redirectnewhandle
    //
    // SELECT RANDOM ITEM, THIS WILL COME FROM THE LIST, We need to keys as we
    // create an array otherwise We use product_type to search only through
    // product_type
    std::string productType = "53212_001";
    glz::generic variables{
        {"productType", std::format("product_type:'{}'", productType)}};

    // This may get copy ellision with lucky!
    auto [shopyResJson, shopyRes] =
        co_await m_client->graphql(GetProductsByArticleQuery, variables);

    // const auto it =
    // shopyResJson["data"]["products"]["nodes"][0]["id"].get<std::string>();

    auto response = drogon::HttpResponse::newHttpResponse();
    response->setContentTypeCode(drogon::ContentType::CT_APPLICATION_JSON);
    response->setBody(std::move(shopyRes));

    co_return response;

    // build shopify query "bulkOperationRunMutation"
    // "productVariantsBulkCreate" send return response
}
} // namespace sapify
