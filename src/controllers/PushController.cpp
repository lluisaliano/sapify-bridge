#include "controllers/PushController.hpp"
#include "controllers/types/PushTypes.hpp"

#include <drogon/HttpClient.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>
#include <drogon/utils/coroutine.h>
#include <stdexcept>
#include <string>

namespace sapify {
drogon::Task<drogon::HttpResponsePtr>
PushController::pushItems(drogon::HttpRequestPtr req) {

    // Receive items to upload from SuperTCModel
    const auto reqBody = std::string{req->body()};

    // Fetch API for shopify cab and det data
    const auto baseUrl{"https://" + m_config->apiSAP};
    auto client = drogon::HttpClient::newHttpClient(baseUrl);

    // Get Cab Data
    auto requestCab = drogon::HttpRequest::newHttpRequest();
    requestCab->setMethod(drogon::Post);
    requestCab->setPath("/shopify/cab"); // API endpoint to get cab items
    requestCab->setBody(
        reqBody); // Send body asking only for corresponding items

    // Get Det Data
    auto requestDet = drogon::HttpRequest::newHttpRequest();
    requestDet->setMethod(drogon::Post);
    requestDet->setPath("/shopify/det"); // API endpoint to get cab items
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
    if (glz::write_json(cab, responseCab->getBody()) ||
        glz::write_json(det, responseDet->getBody())) {
        throw std::runtime_error{
            "Error while parsing APISAP Cab and Det Jsons"};
    }

    // TESTING CHANGING PRODUCT NAME
    // String! means that it is mandatory
    std::string_view GetProductsByArticleQuery =
        R"(query GetProductsByArticle($productType: String!) {
        products(first: 10, query: $productType) {
          nodes {
            id
            title
            productType
          }
        }
      })";
    // SELECT RANDOM ITEM, THIS WILL COME FROM THE LIST
    glz::generic variables{"productType", "53212_001"};

    auto [shopyRes, shopiResJson] =
        co_await m_client->graphql(GetProductsByArticleQuery, variables);

    auto response = drogon::HttpResponse::newHttpResponse();
    response->setContentTypeCode(drogon::ContentType::CT_APPLICATION_JSON);

    response->setBody(std::move(shopiResJson));

    co_return responseBody;

    // build shopify query "bulkOperationRunMutation"
    // "productVariantsBulkCreate" send return response
}
} // namespace sapify
