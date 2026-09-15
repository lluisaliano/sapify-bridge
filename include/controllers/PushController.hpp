#pragma once

#include "config/Config.hpp"
#include "services/ShopifyClient.hpp"
#include <drogon/HttpController.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>
#include <memory>

namespace sapify {
// Controller to push items, prices, images, descriptions to sap, it will handle
// all endpoints to push things Its route handlers will use coroutines, so we
// pass Request by value and return Response by value
// We disable autocreation by passing false. We do this in order to pass the
// shopifyclient and the config values struct
// We use move to avoid one extra copy
class PushController : public drogon::HttpController<PushController, false> {
public:
  PushController(std::shared_ptr<const Config> config, std::shared_ptr<ShopifyClient> client)
      : m_config{std::move(config)}, m_client{std::move(client)} {};

  METHOD_LIST_BEGIN

  // Push items
  ADD_METHOD_TO(PushController::pushItems, "/upload/items", drogon::Post);

  // Push prices
  ADD_METHOD_TO(PushController::pushPrices, "/upload/prices", drogon::Post);

  // Push stock
  ADD_METHOD_TO(PushController::pushStock, "/uploaad/stocks", drogon::Post);

  METHOD_LIST_END

  drogon::Task<drogon::HttpResponsePtr> pushItems(drogon::HttpRequestPtr);

  drogon::Task<drogon::HttpResponsePtr> pushPrices(drogon::HttpRequestPtr);

  drogon::Task<drogon::HttpResponsePtr> pushStock(drogon::HttpRequestPtr);

private:
  std::shared_ptr<const Config> m_config;
  std::shared_ptr<ShopifyClient> m_client;
};
} // namespace sapify
