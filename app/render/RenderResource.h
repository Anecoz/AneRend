#pragma once

#include "AllocatedBuffer.h"
#include "AllocatedImage.h"

#include <cstdint>
#include <variant>

namespace render {

struct IRenderResource
{
  enum class State {
    Valid,
    Invalid,
    Persistent
  } _state = State::Valid;

  enum class Type {
    Generic,
    Buffer,
    Image
  } _type;

  IRenderResource(Type type)
    : _type(type) {}  

  virtual ~IRenderResource() {}
};

// Things like light structs
template <typename T>
struct GenericRenderResource : public IRenderResource
{
  GenericRenderResource() : IRenderResource(Type::Generic) {}

  T _res;
};

struct BufferRenderResource : public IRenderResource
{
  BufferRenderResource() : IRenderResource(Type::Buffer) {}

  AllocatedBuffer _buffer;
};

struct ImageRenderResource : public IRenderResource
{
  ImageRenderResource() : IRenderResource(Type::Image) {}

  AllocatedImage _image;
  VkFormat _format;
};

}