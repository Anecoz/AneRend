#pragma once

#include "RenderResource.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace render {

class RenderResourceVault
{
public:
  RenderResourceVault() = default;
  ~RenderResourceVault() = default;

  RenderResourceVault(const RenderResourceVault&) = delete;
  RenderResourceVault(RenderResourceVault&&) = delete;
  RenderResourceVault& operator=(const RenderResourceVault&) = delete;
  RenderResourceVault& operator=(RenderResourceVault&&) = delete;

  void addResource(const std::string& name, std::unique_ptr<IRenderResource> resource);
  void deleteResource(const std::string& name);
  
  IRenderResource* getResource(const std::string& name);

  bool isPersistentResource(const std::string& name);

  template <typename T>
  GenericRenderResource<T>* getGenericResource(const std::string& name)
  {
    return dynamic_cast<GenericRenderResource<T>*>(getResource(name));
  }

private:
  struct InternalResource
  {
    std::string _name;
    std::unique_ptr<IRenderResource> _resource;
  };

  std::vector<InternalResource> _resources;
};

}