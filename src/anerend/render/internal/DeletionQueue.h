#pragma once

#include "../AccelerationStructure.h"

#include <utility>
#include <vector>

namespace render::internal {

// Not thread-safe!
class DeletionQueue
{
public:
  DeletionQueue();
  DeletionQueue(std::size_t maxNumFrames, VmaAllocator vmaAllocator, VkDevice device);
  ~DeletionQueue();

  // Don't care about any max frames
  void flush();

  // This is expected to be called once per frame
  void execute();

  void add(AllocatedBuffer bufferToDelete);
  void add(AccelerationStructure asToDelete);

private:
  struct DeletionItem
  {
    AllocatedBuffer _bufferToDelete;
    AccelerationStructure _asToDelete;
  };

  void deleteItem(DeletionItem& item);

  // pair is <item, numFramesInQ>
  std::vector<std::pair<DeletionItem, uint32_t>> _items;

  std::size_t _maxNumFrames = 0;
  VmaAllocator _vmaAllocator = nullptr;
  VkDevice _device = nullptr;
};

}