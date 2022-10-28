#pragma once

#include "Material.h"

#include <vulkan/vulkan.h>

#include <string>

namespace render {
namespace compute {

Material createComputeMaterial(VkDevice& device, const std::string& shader);

}}