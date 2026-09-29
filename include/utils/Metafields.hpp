#pragma once

#include "controllers/types/PushTypes.hpp"
#include "utils/Text.hpp"

#include <format>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace sapify {
namespace metafieldsUtils {

using shoeColorRangeObjects = std::unordered_map<std::string, std::string>;

// Define types to work with metafields
struct MetafieldInfo {
    std::string key;
    std::string type;
    std::function<std::optional<std::string>(const ItemCab &,
                                             const shoeColorRangeObjects &)>
        value;
};

// Define metafields and how they are extracted
// ------------ FALTARAN BOLSOS, PARTE METALICA, PUNTA...
inline const std::vector<MetafieldInfo> metafields = {
    {
        "shoe_shape_name", // Nombre de la horma
        "single_line_text_field",
        [](const ItemCab &itemCab, const shoeColorRangeObjects &)
            -> std::optional<std::string> { return itemCab.NOMBRE_HORMA; },
    },
    {
        "shoe_material", // Material, This one is a list
        "list.single_line_text_field",
        [](const ItemCab &itemCab,
           const shoeColorRangeObjects &) -> std::optional<std::string> {
            return std::format("[\"{}\"]", itemCab.MATERIAL);
        },
    },
    {
        "shoe_season", // Temporada
        "single_line_text_field",
        [](const ItemCab &itemCab, const shoeColorRangeObjects &)
            -> std::optional<std::string> { return itemCab.TEMPORADA; },
    },
    {
        "shoe_instep_composition", // Composición del empoeine
        "single_line_text_field",
        [](const ItemCab &itemCab, const shoeColorRangeObjects &)
            -> std::optional<std::string> { return itemCab.EMPEINE; },
    },
    {
        "shoe_lining_composition", // Composición del forro
        "single_line_text_field",
        [](const ItemCab &itemCab, const shoeColorRangeObjects &)
            -> std::optional<std::string> { return itemCab.FORRO; },
    },
    {
        "shoe_insole_composition", // Composición de la plantilla
        "single_line_text_field",
        [](const ItemCab &itemCab, const shoeColorRangeObjects &)
            -> std::optional<std::string> { return itemCab.PLANTILLA; },
    },
    {
        "shoe_sole", // Suela
        "single_line_text_field",
        [](const ItemCab &itemCab, const shoeColorRangeObjects &)
            -> std::optional<std::string> { return itemCab.SUELA; },
    },
    {
        "shoe_theme", // Tema , This one is a list
        "list.single_line_text_field",
        [](const ItemCab &itemCab,
           const shoeColorRangeObjects &) -> std::optional<std::string> {
            // Esto tiene que ser un array de strings
            return std::format("[\"{}\"]", itemCab.TEMA);
        },
    },
    {
        "shoe_heel_height", // Altura Tacón
        "number_integer",
        [](const ItemCab &itemCab, const shoeColorRangeObjects &)
            -> std::optional<std::string> { return itemCab.TACON; },
    },
    {
        "shoe_tip_shape", // Forma de la punta del zapato
        "single_line_text_field",
        [](const ItemCab &itemCab, const shoeColorRangeObjects &)
            -> std::optional<std::string> { return itemCab.PUNTA; },
    },
    {
        "shoe_short_description", // Descripción corta
        "single_line_text_field",
        [](const ItemCab &itemCab, const shoeColorRangeObjects &)
            -> std::optional<std::string> { return itemCab.DESCRIPCION_CORTA; },
    },
    {
        "shoe_family", // Familia
        "single_line_text_field",
        [](const ItemCab &itemCab, const shoeColorRangeObjects &)
            -> std::optional<std::string> { return itemCab.FAMILIA; },
    },
    {
        "shoe_color", // Color
        "single_line_text_field",
        [](const ItemCab &itemCab, const shoeColorRangeObjects &)
            -> std::optional<std::string> { return itemCab.COLOR; },
    },
    {
        "shoe_color_range", // Gama de Color
        "single_line_text_field",
        [](const ItemCab &itemCab, const shoeColorRangeObjects &)
            -> std::optional<std::string> { return itemCab.COLORES; },
    },
    {
        // Gama de Colores obj
        "shoe_color_range_object", // This is a metaobject definition. We have
                                   // to get all the values from the metaobject
                                   // definition gid, and then match it with the
                                   // corresponding one
        "metaobject_reference",
        [](const ItemCab &itemCab,
           const shoeColorRangeObjects &objects) -> std::optional<std::string> {
            if (auto it = objects.find(textUtils::toLower(itemCab.COLOR));
                it != objects.end()) {
                // This does not set up any new MetaObject, they have to be
                // already created on shopify
                return it
                    ->second; // If key exists, return its id, otherwise nullopt
            }
            return std::nullopt;
        },
    },
    {
        "bag_outer_bag", // Bolso exterior
        "single_line_text_field",
        [](const ItemCab &itemCab, const shoeColorRangeObjects &)
            -> std::optional<std::string> { return itemCab.BOLSOS_EXTERIOR; },
    },
    {
        "bag_inner_bag", // Bolso interior
        "single_line_text_field",
        [](const ItemCab &itemCab, const shoeColorRangeObjects &)
            -> std::optional<std::string> { return itemCab.BOLSOS_EXTERIOR; },
    },
    {
        "bag_metal_part", // Bolso Parte Metalica
        "single_line_text_field",
        [](const ItemCab &itemCab, const shoeColorRangeObjects &)
            -> std::optional<std::string> { return itemCab.BOLSOS_EXTERIOR; },
    },

};

/* Create Metafields from sap data
 * @param ns namespace for metafield, mascaro or prettyballerinas
 * @param itemcab
 * @param shopifyProductId
 * @param Shoe_color_range_Objects
 */
inline std::vector<Metafield> createMetafieldsFromSAP(
    std::string_view ns, const ItemCab &itemCab,
    const std::string &shopifyProductId,
    const shoeColorRangeObjects &colorRangeObjects) {

    std::vector<Metafield> metafieldsData;

    for (const auto &field : metafields) {
        // Define extra parameter for this function metaobject function
        std::optional<std::string> value;
        value = field.value(itemCab, colorRangeObjects);

        if (!value || value->empty()) {
            continue;
        }

        metafieldsData.push_back({
            .ownerId = shopifyProductId,
            .ns = std::string{ns},
            .key = field.key,
            .type = field.type,
            .value = *value,
        });
    }

    return metafieldsData;
}

} // namespace metafieldsUtils
} // namespace sapify
