#include "BufferMemoryInterface.h"

namespace render::internal
{

BufferMemoryInterface::BufferMemoryInterface()
  : _size(0)
{}

BufferMemoryInterface::BufferMemoryInterface(std::size_t size)
  : _size(size)
{
  // Create initial free block that spans entire size
  FreeBlock initialFreeBlock{ 0, size };
  _freeBlocks.emplace_back(std::move(initialFreeBlock));
}

BufferMemoryInterface::~BufferMemoryInterface()
{}

BufferMemoryInterface::operator bool() const
{
  return _size != 0;
}

BufferMemoryInterface::Handle BufferMemoryInterface::addData(std::size_t dataSize)
{
  // Check if there is a free block big enough
  for (auto it = _freeBlocks.begin(); it != _freeBlocks.end(); ++it) {
    auto size = it->_size;
    auto offset = it->_offset;

    if (size >= dataSize) {
      Handle handle{ (int64_t)offset, dataSize };

      // Erase free block since it is now used.
      _freeBlocks.erase(it);

      // Do some de-fragmentation efforts. Create a new block with the remaining size.
      if (size > dataSize) {
        FreeBlock splitBlock{ offset + (int64_t)dataSize, size - dataSize };

        // Also check if we can merge this new block with an adjacent block.
        // TODO

        _freeBlocks.emplace_back(std::move(splitBlock));
      }

      printf("Adding data at offset %zu and size %zu\n", handle._offset, handle._size);
      return handle;
    }
  }

  // No free blocks, tough luck! Return invalid handle.
  return Handle();
}

void BufferMemoryInterface::removeData(BufferMemoryInterface::Handle handle)
{
  FreeBlock freeBlock{ handle._offset, handle._size };
  printf("Removing data at offset %zu and size %zu\n", handle._offset, handle._size);
  _freeBlocks.emplace_back(std::move(freeBlock));
}

}