#pragma once

#include "config/Config.hpp"
#include <cstdint>
#include <glaze/glaze.hpp>
#include <optional>
#include <string>
#include <vector>

namespace sapify {
// Items to upload received from SuperTCModel
struct UploadItems {
    std::vector<std::string> articulos;
};

// Items Cab
struct ItemCab {
    std::string ARTICULO;
    std::string DESCRIPCION_CORTA;
    std::string DESCRIPCION_LARGA;
    std::string NOMBRE_HORMA;
    std::string COLOR;
    std::string FAMILIA;
    std::string TEMPORADA;
    std::string TEMA;
    std::string MATERIAL;
    // This following guys can be null, think the why by thinking in a jacket!
    std::optional<std::string> EMPEINE;
    std::optional<std::string> FORRO;
    std::optional<std::string> PLANTILLA;
    std::optional<std::string> SUELA;
    std::optional<std::string> TACON;
    std::optional<std::string> PUNTA;

    std::optional<std::string> COLORES; // Gama Color
    std::optional<std::string> BOLSOS_EXTERIOR;
    std::optional<std::string> BOLSOS_INTERIOR;
    std::optional<std::string> BOLSOS_P_METALICA;

    std::uint16_t PVP; // PRECIO PVP, Takes price from precio_peninsula from
                       // supertcmodel view
    std::uint16_t PRECIO_UK;
    std::uint16_t PRECIO_USA;
};

using ItemsCab = std::vector<ItemCab>;

// Items Det
struct ItemDet {
    long double STOCK; // This has to come as double or the json, conversion may fail, place it first as it is the bigger member
    std::string EAN;
    std::string ARTICULO;
    std::string TALLA;
};

using ItemsDet = std::vector<ItemDet>;

// ------------- Upload Items Variables
struct UploadItemVariables {

    // --- Media (imagen/video/etc.) ---
    // URL Base of Picutres
    static inline std::string PICTURES_BASE_URL =
        "https://" + config.picturesBaseURL;
    struct File {
        std::string filename;
        std::string originalSource;
        std::string alt;
        std::string contentType; // "IMAGE" | "VIDEO" | "EXTERNAL_VIDEO" |
                                 // "MODEL_3D" | "FILE"
        std::string duplicateResolutionMode; // "REPLACE" | "APPEND_UUID" |
                                             // "RAISE_ERROR"
    };

    // --- Variantes (Solo hay un tipo: Talla del Calzado) ---
    // --- Variantes con su nombre y sus valores

    // Variantes de Talla del Calzado. Aqui van las tallas
    struct OptionValue {
        std::string name; // 36, 37...
    };

    // Aqui se define el tipo de la variante. Solo hay uno, el por defecto
    // El nombre sale en la web encima de las tallas
    static constexpr std::string_view DEFAULT_PRODUCT_OPTIONS =
        "Talla del Calzado";
    struct ProductOption {
        std::string name; // Talla del Calzado
        std::vector<OptionValue> values;
    };

    // --- Aqui definimos para que variante son los valores ---
    struct VariantOptionValue {
        std::string optionName; // Talla del Calzado
        std::string name;       // 37
    };

    // --- Definimos valores de variante
    struct Variant {
        std::vector<VariantOptionValue> optionValues;
        std::string sku;
        std::string price;
        std::optional<std::string> compareAtPrice;
        std::optional<std::string> id;
    };

    // --- SEO
    struct Seo {
        std::string title;
        std::string description;
    };

    // Product status
    enum class ProductStatus { ACTIVE, DRAFT, ARCHIVED };

    // --- Valores del producto ---
    std::string title;       // Titulo
    std::string productType; // Codigo ITEM
    std::string vendor;      // Mascaro y Pretty
    std::string
        descriptionHtml;  // Descripcion con etiquetas html, strong, em...
    ProductStatus status; // ACTIVE, DRAFT, UNLISTED
    std::optional<std::string>
        handle; // URL Handle, si no se pasa se usa el titulo en minusculas
    std::optional<bool> redirectNewHandle; // true = redirige automáticamente el
                                           // handle viejo al nuevo
    Seo seo;
    std::vector<File> files;
    std::vector<ProductOption> productOptions;
    std::vector<Variant> variants;
};

// --- Product Shopify Identifier ---
struct ProductSetIdentifier {
    std::string id;
};

// --- USE THIS
// --- Struct exterior: lo que realmente se serializa como "variables" ---
struct UploadItemsSetVariables {
    bool synchronous; // Hacer que shopify espere a subir el item para responder
    std::optional<ProductSetIdentifier>
        identifier; // Si el item no existe, se pasara con std::nullopt
    UploadItemVariables input; // Item
};

// -- Metafields Upload type for variables
struct Metafield {
    std::string ownerId; // Product ID o shopify
    std::string ns;      // mapeado a "namespace"
    std::string key;     // Key
    std::string type;    // Type, single_line_text_field for example
    std::string value;   // value
};

// This is the outter struct for metafields variables
struct Metafields {
    std::vector<Metafield> metafields;
};

// ProductSet Mutation Query Response
struct ProductSetResponse {
      struct Data {
          struct ProductSet {
              struct Product {
                  std::string id{};
              };

              struct UserError {
                  std::string message{};
                  std::string code{};
                  std::vector<std::string> field{};
              };

              Product product{};
              std::vector<UserError> userErrors{};
          };

          ProductSet productSet{};
      };

      Data data{};
  };

} // namespace sapify

// --- Tell Glaze how to deserialize things
// Deserialize Metafield ns to namespace, as namespace is a reserved keyword
template <> struct glz::meta<sapify::Metafield> {
    using T = sapify::Metafield;
    static constexpr auto value =
        glz::object("ownerId", &T::ownerId, "namespace", &T::ns, "key", &T::key, "value", &T::value,
                    "type", &T::type);
};

// Deserialize enum to string
template <> struct glz::meta<sapify::UploadItemVariables::ProductStatus> {
    using enum sapify::UploadItemVariables::ProductStatus;
    static constexpr auto value =
        glz::enumerate("ACTIVE", ACTIVE, "DRAFT", DRAFT, "ARCHIVED", ARCHIVED);
};
