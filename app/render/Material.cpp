#include "Material.h"

namespace render 
{

void Material::updateUbos(std::uint32_t index, VmaAllocator& vmaAllocator, const StandardUBO& standardUbo, const ShadowUBO& shadowUbo)
{
  if (!_ubos.empty()) {
    void* data;
    vmaMapMemory(vmaAllocator, _ubos[index]._allocation, &data);
    memcpy(data, &standardUbo, sizeof(StandardUBO));
    vmaUnmapMemory(vmaAllocator, _ubos[index]._allocation);
  }

  if (!_ubosShadow.empty()) {
    void* data;
    vmaMapMemory(vmaAllocator, _ubosShadow[index]._allocation, &data);
    memcpy(data, &shadowUbo, sizeof(ShadowUBO));
    vmaUnmapMemory(vmaAllocator, _ubosShadow[index]._allocation);
  }
}

}