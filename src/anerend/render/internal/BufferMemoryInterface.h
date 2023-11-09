#pragma once

#include <cstdint>
#include <vector>

namespace render::internal
{

/*
  This class is used as a memory interface to AllocatedBuffer. Typical use-case is 
  a big mesh buffer where we need to keep track of where there is space to insert
  a new mesh data. When mesh data is removed, a free list is kept updated.
*/

class BufferMemoryInterface
{
public:
  struct Handle
  {
    std::int64_t _offset = -1;
    std::size_t _size = 0;

    explicit operator bool() const {
      return _size > 0;
    }
  };

  BufferMemoryInterface();
  BufferMemoryInterface(std::size_t size);
  ~BufferMemoryInterface();

  explicit operator bool() const;

  Handle addData(std::size_t dataSize);
  void removeData(Handle handle);

  std::size_t size() const { return _size; }

private:
  struct FreeBlock
  {
    std::int64_t _offset;
    std::size_t _size;
  };

  std::vector<FreeBlock> _freeBlocks;
  std::size_t _size;
};

}