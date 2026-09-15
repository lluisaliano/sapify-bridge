#include "controllers/PushController.hpp"
#include "controllers/types/PushTypes.hpp"
#include <drogon/HttpClient.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/utils/coroutine.h>
#include <stdexcept>

namespace sapify {
    drogon::Task<drogon::HttpResponsePtr> PushController::pushItems(drogon::HttpRequestPtr req) {

        // Receive items to upload from SuperTCModel
       const auto reqBody = req->body();
       const UploadItems items;

       if (glz::read_json(items, reqBody)) {
           throw std::runtime_error("Error while receiving items from superTCModel on push items");
       }

       // Fetch api for items data
       // build shopify query
       // send
       // return response



       auto client = drogon::HttpClient::newHttpClient(m_config->apiSAP);


    }
}
