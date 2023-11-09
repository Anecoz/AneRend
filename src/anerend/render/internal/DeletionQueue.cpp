#include "DeletionQueue.h"
#include "../VulkanExtensions.h"

namespace render::internal {

DeletionQueue::DeletionQueue()
  : _maxNumFrames(0)
  , _vmaAllocator(nullptr)
  , _device(nullptr)
{}

DeletionQueue::DeletionQueue(std::size_t maxNumFrames, VmaAllocator vmaAllocator, VkDevice device)
  : _maxNumFrames(maxNumFrames)
  , _vmaAllocator(vmaAllocator)
  , _device(device)
{}

DeletionQueue::~DeletionQueue()
{
  for (auto& item : _items) {
    deleteItem(item.first);
  }
}

// This is expected to be called once per frame
void DeletionQueue::execute()
{
  for (auto it = _items.begin(); it != _items.end();) {
    if (it->second > _maxNumFrames) {
      deleteItem(it->first);
      it = _items.erase(it);
    }
    else {
      ++it;
    }
  }

  for (auto& item : _items) {
    item.second++;
  }
}

void DeletionQueue::add(AllocatedBuffer bufferToDelete)
{
  DeletionItem item{};
  item._bufferToDelete = bufferToDelete;
  _items.emplace_back(std::move(item), 0);
}

void DeletionQueue::add(AccelerationStructure asToDelete)
{
  DeletionItem item{};
  item._asToDelete = asToDelete;
  _items.emplace_back(std::move(item), 0);
}

void DeletionQueue::deleteItem(DeletionItem& item)
{
  if (item._asToDelete) {
    vmaDestroyBuffer(_vmaAllocator, item._asToDelete._buffer._buffer, item._asToDelete._buffer._allocation);
    vmaDestroyBuffer(_vmaAllocator, item._asToDelete._scratchBuffer._buffer, item._asToDelete._scratchBuffer._allocation);
    vkext::vkDestroyAccelerationStructureKHR(_device, item._asToDelete._as, nullptr);
  }
  if (item._bufferToDelete) {
    vmaDestroyBuffer(_vmaAllocator, item._bufferToDelete._buffer, item._bufferToDelete._allocation);
  }
}

}