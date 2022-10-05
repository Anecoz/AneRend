#pragma once

#include <optional>

namespace render {

struct QueueFamilyIndices {
  std::optional<std::uint32_t> graphicsFamily;
  std::optional<std::uint32_t> presentFamily;

  bool isComplete() {
    return graphicsFamily.has_value() && presentFamily.has_value();
  }
};

}