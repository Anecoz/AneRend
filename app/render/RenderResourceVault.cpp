#include "RenderResourceVault.h"

#include "RenderContext.h"

namespace render {

RenderResourceVault::RenderResourceVault(int multiBufferSize)
  : _multiBufferSize(multiBufferSize)
{
}

void RenderResourceVault::addResource(const std::string& name, std::unique_ptr<IRenderResource> resource, bool multiBuffered, int multiBufferIdx, bool noDelete)
{
  bool found = false;
  for (auto& internal: _resources) {
    if (internal._name == name) {
      if (internal._multiBuffered) {
        if (internal._resource.size() - 1 < multiBufferIdx) {
          internal._resource.resize(multiBufferIdx + 1);
        }

        internal._resource[multiBufferIdx] = std::move(resource);
        found = true;
      }
    }
  }

  if (!found) {
    InternalResource internal;
    internal._resource.resize(_multiBufferSize);
    internal._name = name;
    internal._noDelete = noDelete;
    internal._multiBuffered = multiBuffered;
    internal._resource[multiBufferIdx] = std::move(resource);

    _resources.emplace_back(std::move(internal));
  }
}

IRenderResource* RenderResourceVault::getResource(const std::string& name, int multiBufferIdx)
{
  for (auto& internal: _resources) {
    if (internal._name == name) {
      if (internal._multiBuffered) {
        return internal._resource[multiBufferIdx].get();
      }
      else {
        return internal._resource[0].get();
      }
    }
  }
  return nullptr;
}

bool RenderResourceVault::isPersistentResource(const std::string& name)
{
  for (auto& res : _resources) {
    if (res._name == name && res._resource[0]->_state == IRenderResource::State::Persistent) {
      return true;
    }
  }

  return false;
}

void RenderResourceVault::deleteResource(const std::string& name)
{
  for (auto it = _resources.begin(); it != _resources.end(); ++it) {
    if (it->_name == name) {
      _resources.erase(it);
      return;
    }
  }
}

void RenderResourceVault::clear(RenderContext* rc)
{
  for (auto& res : _resources) {
    if (res._noDelete) continue;

    for (auto& ptr : res._resource) {
      if (auto buf = dynamic_cast<BufferRenderResource*>(ptr.get())) {
        vmaDestroyBuffer(rc->vmaAllocator(), buf->_buffer._buffer, buf->_buffer._allocation);
      }
      else if (auto im = dynamic_cast<ImageRenderResource*>(ptr.get())) {
        vmaDestroyImage(rc->vmaAllocator(), im->_image._image, im->_image._allocation);
        vkDestroyImageView(rc->device(), im->_view, nullptr);
      }
    }
  }

  _resources.clear();
}

}