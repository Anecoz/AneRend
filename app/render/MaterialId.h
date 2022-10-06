#pragma once

#include <cstdint>

namespace render {

typedef std::uint64_t MaterialID;

constexpr MaterialID STANDARD_MATERIAL_ID = 0;
constexpr MaterialID STANDARD_INSTANCED_MATERIAL_ID = 1;

}