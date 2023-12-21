#pragma once

#include "../AllocatedBuffer.h"

namespace render::internal {

struct StagingBuffer
{
  AllocatedBuffer _buf;

  std::size_t _size = 0;
  std::size_t _currentOffset = 0;
  std::size_t _emergencyReserve = 0;

  void setEmergencyReserve(std::size_t bytes) {
    _emergencyReserve = bytes;
  }

  void advance(std::size_t bytes) {
    _currentOffset += bytes;
  }

  void reset() {
    _currentOffset = 0;
  }

  bool canFit(std::size_t bytes, bool useReserve = false) {
    if (useReserve) {
      return _currentOffset + bytes <= _size;
    }

    return _currentOffset + bytes <= _size - _emergencyReserve;
  }
};

}