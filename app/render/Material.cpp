#include "Material.h"

namespace render 
{

void Material::updateUbos(std::uint32_t index, VmaAllocator& vmaAllocator, const StandardUBO& standardUbo, const ShadowUBO& shadowUbo)
{
  if (!_ubos[STANDARD_INDEX].empty()) {
    void* data;
    vmaMapMemory(vmaAllocator, _ubos[STANDARD_INDEX][index]._allocation, &data);
    memcpy(data, &standardUbo, sizeof(StandardUBO));
    vmaUnmapMemory(vmaAllocator, _ubos[STANDARD_INDEX][index]._allocation);
  }

  if (!_ubos[SHADOW_INDEX].empty()) {
    void* data;
    vmaMapMemory(vmaAllocator, _ubos[SHADOW_INDEX][index]._allocation, &data);
    memcpy(data, &shadowUbo, sizeof(ShadowUBO));
    vmaUnmapMemory(vmaAllocator, _ubos[SHADOW_INDEX][index]._allocation);
  }
}

}