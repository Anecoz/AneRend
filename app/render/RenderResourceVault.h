#pragma once

#include "RenderResource.h"

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace render {

class RenderResourceVault
{
public:
  RenderResourceVault() = delete;
  RenderResourceVault(int multiBufferSize);
  ~RenderResourceVault() = default;

  RenderResourceVault(const RenderResourceVault&) = delete;
  RenderResourceVault(RenderResourceVault&&) = delete;
  RenderResourceVault& operator=(const RenderResourceVault&) = delete;
  RenderResourceVault& operator=(RenderResourceVault&&) = delete;

  void addResource(const std::string& name, std::unique_ptr<IRenderResource> resource, bool multiBuffered = false, int multiBufferIdx = 0);
  void deleteResource(const std::string& name);

  void clear();
  
  IRenderResource* getResource(const std::string& name, int multiBufferIdx = 0);

  bool isPersistentResource(const std::string& name);

  template <typename T>
  GenericRenderResource<T>* getGenericResource(const std::string& name)
  {
    return dynamic_cast<GenericRenderResource<T>*>(getResource(name));
  }

private:
  const std::size_t _multiBufferSize;

  struct InternalResource
  {
    std::string _name;
    bool _multiBuffered = false;
    std::vector<std::unique_ptr<IRenderResource>> _resource;
  };

  std::vector<InternalResource> _resources;
};

}