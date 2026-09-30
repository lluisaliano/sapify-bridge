#pragma once

#include "utils/UnorderedMapTransparency.hpp"
#include <cassert>
#include <chrono>
#include <drogon/drogon.h>
#include <drogon/orm/Exception.h>
#include <iostream>
#include <optional>
#include <print>
#include <string_view>
#include <unordered_map>

namespace sapify {
// SuperCloudImages Database, get if images exists
// In sabates order is: Pers Top Side Back Det
// In bolsos order is: Side Pers Top Back Det Lat

// Using heterogeneous lookup, std::equal_to already has is_transparent
using ImageVariantMap = std::unordered_map<std::string, bool, StringHash, std::equal_to<>>;

inline drogon::Task<std::optional<ImageVariantMap>>
getCloudImagesData(std::string_view item) {
    // We use thread_local because we use is_fast client, so each thread must
    // have its own client With is_fast = false, a thread pool is used to run
    // queries, which are queued by the same client, (getDbClient) which handles
    // enqueue for us Avoid 0,01ms by using thread_local as clients are created
    // on start up and getFastDbClient just does a lookup of the client in a map
    thread_local auto client = drogon::app().getFastDbClient("cloudimages");
    assert(client &&
           "getFastDbClient must be called inside dragon's event loop");

    try {
        // Get if variant image exists
        auto result = co_await client->execSqlCoro(R"(
    select
        cast(max(case when image_name like '%' || $1 || '_pers%' then cast(uploaded as int) else 0 end) as bool) as pers,
        cast(max(case when image_name like '%' || $1 || '_top%' then cast(uploaded as int) else 0 end) as bool) as top,
        cast(max(case when image_name like '%' || $1 || '_side%' then cast(uploaded as int) else 0 end) as bool) as side,
        cast(max(case when image_name like '%' || $1 || '_back%' then cast(uploaded as int) else 0 end) as bool) as back,
        cast(max(case when image_name like '%' || $1 || '_det%' then cast(uploaded as int) else 0 end) as bool) as det,
        cast(max(case when image_name like '%' || $1 || '_lateral%' then cast(uploaded as int) else 0 end) as bool) as lateral
    from cloudflare
    where image_name like '%' || $1 || '%')",
                                                  item);

        ImageVariantMap imageVariants;

        // This should never happen
        if (result.empty()) {
            std::println(
                std::cerr, "{}: [SUPERERROR] This item has no images",
                std::chrono::zoned_time{"Europe/Madrid",
                                        std::chrono::system_clock::now()});
            co_return std::nullopt;
        }

        auto row = result[0];

        imageVariants["pers"] = row["pers"].as<bool>();
        imageVariants["top"] = row["top"].as<bool>();
        imageVariants["side"] = row["side"].as<bool>();
        imageVariants["back"] = row["back"].as<bool>();
        imageVariants["det"] = row["det"].as<bool>();
        imageVariants["lateral"] = row["lateral"].as<bool>();

        co_return imageVariants;

    } catch (const drogon::orm::DrogonDbException &e) {
        std::println("{}: [ERROR] Error on query to the db for item: {}",
                     std::chrono::zoned_time{"Europe/Madrid",
                                             std::chrono::system_clock::now()},
                     item);
        co_return std::nullopt;
    }
}
} // namespace sapify
