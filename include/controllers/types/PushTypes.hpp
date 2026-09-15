#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace sapify {
// Items to upload
struct Item {
    std::string ARTICULO;
};
using UploadItems = std::vector<Item>;

// Items data
struct ItemCab {
  std::string ARTICULO;
  std::string DESCRIPCION_CORTA;
  std::string DESCRIPCION_LARGA;
  std::uint16_t PRECIO_EUROPA; // PRECIO PVP?
  std::uint16_t PRECIO_UK;
  std::uint16_t PRECIO_USA;
};
using PushItemsBody = std::vector<ItemCab>;
} // namespace sapify
