#include "controllers/PushController.hpp"
#include "config/Config.hpp"
#include "controllers/types/PushTypes.hpp"
#include "mapping/ProductSet.hpp"
#include "services/types/Queryies.hpp"
#include "utils/Awaiter.hpp"
#include "utils/Metafields.hpp"
#include "utils/Stocks.hpp"

#include <drogon/HttpClient.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>
#include <drogon/utils/coroutine.h>
#include <format>
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

    // SendRequests, usign wrapper function fetch to be able to work with
    // when_all, as it only supports drogon::Task types, but idk why, not
    // drogon::HttpRespAwaiter
    auto [responseCab, responseDet] = co_await drogon::when_all(
        fetch(client, requestCab), fetch(client, requestDet));

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

    // Create map to split have each Item as ID and ItemsDet as vector
    std::unordered_map<std::string, ItemsDet> detMap;
    detMap.reserve(cab.size()); // Reserve enough space for all cab items
    for (auto &itemDet : det) {
        auto [it, inserted] = detMap.try_emplace(itemDet.ARTICULO); // Insert key to map and value initialize ItemsDet
        // Then, move new element
        it->second.push_back(std::move(itemDet));
    }

    // Fetch Shopify Data needed for all items

    // Metaobjects from definitions
    // shoe_color_ranges_object This is stored in a map with lowerCase
    // letters and values are ids we put to create metafields
    auto shoeColorRangesMetaObjects =
        co_await m_client->getColorRangeObjects(); // Keys are lowercased,
                                                   // rojos, naranjas...

    // IMPORTANT!!!!!!!!! REMOVEE THISSSSSSS TODO ---------- MOVE THIS SOMEWHERE
    // ELSE, THIS IS CURRENTLY USED TO SET A RESPONSE FOR THE HTTP POST OF THE
    // USER, if updating multiple articles, it should return an array with
    // status and errors for each product The initialization of the httpresponse
    // can be left here but should be updated with each product and send after
    // the for loop
    auto responseShopy = drogon::HttpResponse::newHttpResponse();

    // For each item
    for (auto &itemCab : cab) {
        // Get Product mapping, pass shopifyClient as referece
        auto productSet = co_await productSet::getProductSet(
            *m_client, itemCab, shoeColorRangesMetaObjects, detMap);
        if (!productSet) {
            continue;
        }
        auto uploadProductsVariables = std::move(productSet.value());

        // UploadItem -> This return gets copy ellision
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

        // Initialize metafields
        Metafields metafields{metafieldsUtils::createMetafieldsFromSAP(
            ns, itemCab, id, shoeColorRangesMetaObjects)};
        // Do the update
        auto [shopyResMetafields, shopyResMetafieldsString] =
            co_await m_client->graphql(articleMetafieldsQuery, metafields);
        // ---

        // ---------- TODO REMOVE THISSSSSSSSSS!!!!!!!!!!!!!!
        responseShopy->setContentTypeCode(
            drogon::ContentType::CT_APPLICATION_JSON);
        responseShopy->setBody(std::move(shopyResString));
        // REMOVE BREAK
        break;
    }
    // FALTARA SUBIR IDIOMAS
    // TESTING CHANGING PRODUCT NAME ONLY FROM SHOPIFY

    co_return responseShopy;
    // --------- IMPORTANT build shopify query "bulkOperationRunMutation" to
    // avoid rate LImits "productVariantsBulkCreate" send return response
}

drogon::Task<drogon::HttpResponsePtr>
PushController::pushStock(drogon::HttpRequestPtr req) {
    // Receive items to update stock from SuperTCModel. We will have to update
    // this regularly, so we may need to track uploaded items
    /*
     * {
     * articulos: ["1231_001",...]
     * }
     */

    // Get articles from body
    const auto reqBody = std::string{req->body()};

    // Parse items to upload items
    UploadItems uploadItems;
    if (glz::read_json(uploadItems, reqBody)) {
        throw std::runtime_error{"Error while parsing recieved articles Json"};
    }

    // Set base URL and http client to get stock query
    const auto baseUrl{"https://" + config.apiSAP};
    auto client = drogon::HttpClient::newHttpClient(baseUrl);

    // Set up request and send request
    auto stockReq = drogon::HttpRequest::newHttpRequest();
    stockReq->setMethod(drogon::Post);
    stockReq->setContentTypeCode(drogon::ContentType::CT_APPLICATION_JSON);
    stockReq->setPath("/shopify/stock"); // Stock Path on API
    stockReq->setBody(
        reqBody); // We can do a move here as we will not use this anymore

    auto stockResponse = co_await client->sendRequestCoro(stockReq);

    if (stockResponse->getStatusCode() < drogon::HttpStatusCode::k200OK ||
        stockResponse->getStatusCode() >=
            drogon::HttpStatusCode::k300MultipleChoices) {
        throw std::runtime_error(std::format(
            "APISAP returned non-2xx code when fetching stock data"));
    }

    // Parse stock data from APISAP
    ItemsStock stock;
    if (glz::read_json(stock, stockResponse->getBody())) {
        throw std::runtime_error{"Error while parsing APISAP Stock Json"};
    }

    // Get Map
    auto& locationMap = stockUtils::getLocationMap();



    co_return drogon::HttpResponse::newHttpResponse();
}
} // namespace sapify
