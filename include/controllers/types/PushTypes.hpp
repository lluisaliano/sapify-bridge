#pragma once

#include <cstdint>
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
    std::uint16_t PVP; // PRECIO PVP, Takes price from precio_peninsula from
                       // supertcmodel view
    std::uint16_t PRECIO_UK;
    std::uint16_t PRECIO_USA;
};

using ItemsCab = std::vector<ItemCab>;

// Items Det
struct ItemDet {
    std::string EAN;
    std::string ARTICULO;
    std::string TALLA;
    std::uint32_t STOCK; // 32 bits is more than enough to represent max stock
};

using ItemsDet = std::vector<ItemDet>;
} // namespace sapify
