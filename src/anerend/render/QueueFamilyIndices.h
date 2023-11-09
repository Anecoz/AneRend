#pragma once

#include <optional>

namespace render {

struct QueueFamilyIndices {
  std::optional<std::uint32_t> graphicsFamily;
  std::optional<std::uint32_t> presentFamily;
  std::optional<std::uint32_t> computeFamily;
  std::optional<std::uint32_t> transferFamily;

  bool isComplete() {
    return graphicsFamily.has_value() && presentFamily.has_value() && computeFamily.has_value() && transferFamily.has_value();
  }
};

}