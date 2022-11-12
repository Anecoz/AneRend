#include "RenderResourceVault.h"

namespace render {

void RenderResourceVault::addResource(const std::string& name, std::unique_ptr<IRenderResource> resource)
{
  InternalResource internal;
  internal._name = name;
  internal._resource = std::move(resource);
}

IRenderResource* RenderResourceVault::getResource(const std::string& name)
{
  for (auto& internal: _resources) {
    if (internal._name == name) {
      return internal._resource.get();
    }
  }
  return nullptr;
}

bool RenderResourceVault::isPersistentResource(const std::string& name)
{
  for (auto& res : _resources) {
    if (res._name == name && res._resource->_state == IRenderResource::State::Persistent) {
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

}