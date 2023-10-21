#pragma once

#include "../AllocatedBuffer.h"
#include "BufferMemoryInterface.h"

namespace render::internal
{

struct GigaBuffer 
{
  GigaBuffer()
    : _memInterface(0)
  {}

  GigaBuffer(std::size_t size) 
    : _memInterface(size)
  {}

  GigaBuffer(const GigaBuffer&) = delete;
  GigaBuffer(GigaBuffer&&) = delete;
  GigaBuffer& operator=(const GigaBuffer&) = delete;
  GigaBuffer& operator=(GigaBuffer&&) = delete;

  render::AllocatedBuffer _buffer;
  BufferMemoryInterface _memInterface;
};

}