#pragma once

#include "../AllocatedBuffer.h"

namespace render::internal {

struct StagingBuffer
{
  AllocatedBuffer _buf;

  std::size_t _size = 0;
  std::size_t _currentOffset = 0;

  void advance(std::size_t bytes) {
    _currentOffset += bytes;
  }

  void reset() {
    _currentOffset = 0;
  }

  bool canFit(std::size_t bytes) {
    return _currentOffset + bytes <= _size;
  }
};

}