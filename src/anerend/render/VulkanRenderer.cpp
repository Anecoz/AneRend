#include "VulkanRenderer.h"

#include "QueueFamilyIndices.h"
#include "ImageHelpers.h"
#include "BufferHelpers.h"
#include "GpuBuffers.h"
#include "RenderResource.h"
#include "VulkanExtensions.h"
#include "passes/CullRenderPass.h"
#include "passes/GeometryRenderPass.h"
#include "passes/PresentationRenderPass.h"
#include "passes/UIRenderPass.h"
#include "passes/ShadowRenderPass.h"
#include "passes/DebugViewRenderPass.h"
#include "passes/GrassRenderPass.h"
#include "passes/GrassShadowRenderPass.h"
#include "passes/DeferredLightingRenderPass.h"
#include "passes/SSAORenderPass.h"
#include "passes/SSAOBlurRenderPass.h"
#include "passes/FXAARenderPass.h"
#include "passes/HiZRenderPass.h"
#include "passes/ShadowRayTracingPass.h"
#include "passes/DebugBSRenderPass.h"
#include "passes/IrradianceProbeTranslationPass.h"
#include "passes/IrradianceProbeRayTracingPass.h"
#include "passes/SurfelUpdateRayTracingPass.h"
//#include "passes/SSGlobalIlluminationRayTracingPass.h"
//#include "passes/SSGIBlurRenderPass.h"
//#include "passes/SurfelConvolveRenderPass.h"
#include "passes/IrradianceProbeConvolvePass.h"
#include "passes/SpecularGIRTPass.h"
#include "passes/SpecularGIMipPass.h"
#include "passes/BloomRenderPass.h"
//#include "passes/LightShadowRayTracingPass.h"
//#include "passes/LightShadowSumPass.h"
#include "passes/ParticleUpdatePass.h"
#include "passes/UpdateTLASPass.h"
#include "passes/SurfelSHPass.h"
#include "passes/LuminanceHistogramPass.h"
#include "passes/LuminanceAveragePass.h"
#include "passes/UpdateBlasPass.h"

#include "../../common/util/Utils.h"
#include  "../util/GraphicsUtils.h"
#include "../LodePng/lodepng.h"
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_glfw.h"
#include "../imgui/imgui_impl_vulkan.h"
#include <ImGuizmo.h>

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include <glm/gtc/matrix_access.hpp>

#include <algorithm>
#include <cstdint>
#include <chrono>
#include <cstring>
#include <limits>
#include <iostream>
#include <set>
#include <random>

namespace {

// EXTRA FUNCTION LOADS
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) {
    return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr) {
    func(instance, debugMessenger, pAllocator);
  }
}

VkResult SetDebugUtilsObjectNameEXT(VkInstance instance, VkDevice device, const VkDebugUtilsObjectNameInfoEXT* pNameInfo)
{
  auto func = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT");
  if (func != nullptr) {
    return func(device, pNameInfo);
  }
  else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

VKAPI_ATTR VkBool32 VKAPI_CALL g_DebugCallback(
  VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
  VkDebugUtilsMessageTypeFlagsEXT messageType,
  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
  void* pUserData)
{
  if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    //printf("\nvalidation layer: %s\n", pCallbackData->pMessage);
  }

  return VK_FALSE;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateAccelerationStructureKHR(
  VkDevice                                    device,
  const VkAccelerationStructureCreateInfoKHR* pCreateInfo,
  const VkAllocationCallbacks* pAllocator,
  VkAccelerationStructureKHR* pAccelerationStructure)
{
  auto func = (PFN_vkCreateAccelerationStructureKHR)vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureKHR");
  if (func != nullptr) {
    return func(device, pCreateInfo, pAllocator, pAccelerationStructure);
  }
  else {
    printf("vkCreateAccelerationStructure not available!\n");
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

VKAPI_ATTR void VKAPI_CALL GetAccelerationStructureBuildSizesKHR(
  VkDevice                                    device,
  VkAccelerationStructureBuildTypeKHR         buildType,
  const VkAccelerationStructureBuildGeometryInfoKHR* pBuildInfo,
  const uint32_t* pMaxPrimitiveCounts,
  VkAccelerationStructureBuildSizesInfoKHR* pSizeInfo)
{
  auto func = (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetDeviceProcAddr(device, "vkGetAccelerationStructureBuildSizesKHR");
  if (func != nullptr) {
    return func(device, buildType, pBuildInfo, pMaxPrimitiveCounts, pSizeInfo);
  }
  else {
    printf("vkGetAccelerationStructureBuildSizesKHR not available!\n");
  }
}

VKAPI_ATTR VkDeviceAddress VKAPI_CALL GetAccelerationStructureDeviceAddressKHR(
  VkDevice                                    device,
  const VkAccelerationStructureDeviceAddressInfoKHR* pInfo)
{
  auto func = (PFN_vkGetAccelerationStructureDeviceAddressKHR)vkGetDeviceProcAddr(device, "vkGetAccelerationStructureDeviceAddressKHR");
  if (func != nullptr) {
    return func(device, pInfo);
  }
  else {
    printf("vkGetAccelerationStructureDeviceAddressKHR not available!\n");
  }

  return 0;
}

}

namespace render {

VulkanRenderer::VulkanRenderer(GLFWwindow* window, const Camera& initialCamera)
  : _currentSwapChainIndex(0)
  , _vault(MAX_FRAMES_IN_FLIGHT)
  , _gigaVtxBuffer(1024 * 1024 * GIGA_MESH_BUFFER_SIZE_MB)
  , _gigaIdxBuffer(1024 * 1024 * GIGA_MESH_BUFFER_SIZE_MB)
  , _skeletonMemIf(MAX_NUM_SKINNED_MODELS * MAX_NUM_JOINTS) // worst case, all models with max num of joints. In terms of number of matrices, not bytes.
  , _bindlessTextureMemIf(MAX_BINDLESS_RESOURCES)
  , _latestCamera(initialCamera)
  , _fgb(&_vault)
  , _window(window)
  , _enableValidationLayers(true)
  , _enableRayTracing(true)
  , _delQ()
{
  imageutil::init();
}

VulkanRenderer::~VulkanRenderer()
{
  // Let logical device finish operations first
  vkDeviceWaitIdle(_device);

  vmaDestroyBuffer(_vmaAllocator, _gigaVtxBuffer._buffer._buffer, _gigaVtxBuffer._buffer._allocation);
  vmaDestroyBuffer(_vmaAllocator, _gigaIdxBuffer._buffer._buffer, _gigaIdxBuffer._buffer._allocation);
  
  vmaDestroyBuffer(_vmaAllocator, _worldPosRequest._hostBuffer._buffer, _worldPosRequest._hostBuffer._allocation);

  for (auto& mat : _currentMaterials) {
    if (mat._albedoInfo) {
      vkDestroySampler(_device, mat._albedoInfo._sampler, nullptr);
      vmaDestroyImage(_vmaAllocator, mat._albedoInfo._image._image, mat._albedoInfo._image._allocation);
      vkDestroyImageView(_device, mat._albedoInfo._view, nullptr);
    }
    if (mat._emissiveInfo) {
      vkDestroySampler(_device, mat._emissiveInfo._sampler, nullptr);
      vmaDestroyImage(_vmaAllocator, mat._emissiveInfo._image._image, mat._emissiveInfo._image._allocation);
      vkDestroyImageView(_device, mat._emissiveInfo._view, nullptr);
    }
    if (mat._metRoughInfo) {
      vkDestroySampler(_device, mat._metRoughInfo._sampler, nullptr);
      vmaDestroyImage(_vmaAllocator, mat._metRoughInfo._image._image, mat._metRoughInfo._image._allocation);
      vkDestroyImageView(_device, mat._metRoughInfo._view, nullptr);
    }
    if (mat._normalInfo) {
      vkDestroySampler(_device, mat._normalInfo._sampler, nullptr);
      vmaDestroyImage(_vmaAllocator, mat._normalInfo._image._image, mat._normalInfo._image._allocation);
      vkDestroyImageView(_device, mat._normalInfo._view, nullptr);
    }
  }

  if (_enableRayTracing) {
    for (auto& blas : _blases) {
      vmaDestroyBuffer(_vmaAllocator, blas.second._buffer._buffer, blas.second._buffer._allocation);
      vmaDestroyBuffer(_vmaAllocator, blas.second._scratchBuffer._buffer, blas.second._scratchBuffer._allocation);
      vkext::vkDestroyAccelerationStructureKHR(_device, blas.second._as, nullptr);
    }

    for (auto& dynamicPair : _dynamicBlases) {
      for (auto& blas : dynamicPair.second) {
        vmaDestroyBuffer(_vmaAllocator, blas._buffer._buffer, blas._buffer._allocation);
        vmaDestroyBuffer(_vmaAllocator, blas._scratchBuffer._buffer, blas._scratchBuffer._allocation);
        vkext::vkDestroyAccelerationStructureKHR(_device, blas._as, nullptr);
      }
    }

    vmaDestroyBuffer(_vmaAllocator, _tlas._buffer._buffer, _tlas._buffer._allocation);
    vmaDestroyBuffer(_vmaAllocator, _tlas._scratchBuffer._buffer, _tlas._scratchBuffer._allocation);
    vkext::vkDestroyAccelerationStructureKHR(_device, _tlas._as, nullptr);
  }

  for (auto& light: _lights) {
    for (auto& v: light._shadowImageViews) {
      vkDestroyImageView(_device, v, nullptr);
    }
    vkDestroyImageView(_device, light._cubeShadowView, nullptr);

    vmaDestroyImage(_vmaAllocator, light._shadowImage._image, light._shadowImage._allocation);
    vkDestroySampler(_device, light._sampler, nullptr);
    light._shadowImageViews.clear();
  }

  vkDestroyDescriptorPool(_device, _descriptorPool, nullptr);
  vkDestroyDescriptorPool(_device, _imguiDescriptorPool, nullptr);

  ImGui_ImplVulkan_Shutdown();

  vkDestroyDescriptorSetLayout(_device, _bindlessDescSetLayout, nullptr);
  vkDestroyPipelineLayout(_device, _bindlessPipelineLayout, nullptr);

  _fgb.reset(this);

  for (auto& rp : _renderPasses) {
    rp->cleanup(this, &_vault);
    delete rp;
  }

  _vault.clear(this);

  cleanupSwapChain();

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    vkDestroySemaphore(_device, _imageAvailableSemaphores[i], nullptr);
    vkDestroySemaphore(_device, _renderFinishedSemaphores[i], nullptr);
    vkDestroyFence(_device, _inFlightFences[i], nullptr);
    vmaDestroyBuffer(_vmaAllocator, _gpuRenderableBuffer[i]._buffer, _gpuRenderableBuffer[i]._allocation);
    vmaDestroyBuffer(_vmaAllocator, _gpuMaterialBuffer[i]._buffer, _gpuMaterialBuffer[i]._allocation);
    vmaDestroyBuffer(_vmaAllocator, _gpuRenderableMaterialIndexBuffer[i]._buffer, _gpuRenderableMaterialIndexBuffer[i]._allocation);
    vmaDestroyBuffer(_vmaAllocator, _gpuModelBuffer[i]._buffer, _gpuModelBuffer[i]._allocation);
    vmaDestroyBuffer(_vmaAllocator, _gpuMeshInfoBuffer[i]._buffer, _gpuMeshInfoBuffer[i]._allocation);
    vmaDestroyBuffer(_vmaAllocator, _gpuStagingBuffer[i]._buffer, _gpuStagingBuffer[i]._allocation);
    vmaDestroyBuffer(_vmaAllocator, _gpuSceneDataBuffer[i]._buffer, _gpuSceneDataBuffer[i]._allocation);
    vmaDestroyBuffer(_vmaAllocator, _gpuLightBuffer[i]._buffer, _gpuLightBuffer[i]._allocation);
    vmaDestroyBuffer(_vmaAllocator, _gpuViewClusterBuffer[i]._buffer, _gpuViewClusterBuffer[i]._allocation);
    vmaDestroyBuffer(_vmaAllocator, _gpuSkeletonBuffer[i]._buffer, _gpuSkeletonBuffer[i]._allocation);

    vkDestroyImageView(_device, _gpuWindForceView[i], nullptr);
    vmaDestroyImage(_vmaAllocator, _gpuWindForceImage[i]._image, _gpuWindForceImage[i]._allocation);
    vkDestroySampler(_device, _gpuWindForceSampler[i], nullptr);

    vkDestroyQueryPool(_device, _queryPools[i], nullptr);
  }

  vkDestroyCommandPool(_device, _commandPool, nullptr);

  vmaDestroyAllocator(_vmaAllocator);

  vkDestroyDevice(_device, nullptr);
  if (_enableValidationLayers) {
    DestroyDebugUtilsMessengerEXT(_instance, _debugMessenger, nullptr);
  }

  vkDestroySurfaceKHR(_instance, _surface, nullptr);
  vkDestroyInstance(_instance, nullptr);
}

bool VulkanRenderer::init()
{
  _renderablesChanged.resize(MAX_FRAMES_IN_FLIGHT);
  _modelsChanged.resize(MAX_FRAMES_IN_FLIGHT);
  _materialsChanged.resize(MAX_FRAMES_IN_FLIGHT);

  uint32_t extensionCount = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
  std::vector<VkExtensionProperties> extensions(extensionCount);
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

  printf("Available vulkan extensions:\n");

  for (const auto& extension : extensions) {
    printf("\t%s\n", extension.extensionName);
  }

  printf("Initialising instance...");
  bool res = createInstance();
  if (!res) return false;
  printf("Done!\n");

  printf("Creating debug messenger...");
  res &= setupDebugMessenger();
  if (!res) return false;
  printf("Done!\n");

  printf("Creating surface...");
  res &= createSurface();
  if (!res) return false;
  printf("Done!\n");

  printf("Picking physical device...");
  res &= pickPhysicalDevice();
  if (!res) return false;
  printf("Done!\n");

  printf("Creating logical device...");
  res &= createLogicalDevice();
  if (!res) return false;
  printf("Done!\n");

  printf("Creating VmaAllocator...");
  res &= createVmaAllocator();
  if (!res) return false;
  printf("Done!\n");

  printf("Creating swap chain...");
  res &= createSwapChain();
  if (!res) return false;
  printf("Done!\n");

  printf("Creating command pool...");
  res &= createCommandPool();
  if (!res) return false;
  printf("Done!\n");

  printf("Creating query pool...");
  res &= createQueryPool();
  if (!res) return false;
  printf("Done!\n");

  printf("Creating descriptor pool...");
  res &= createDescriptorPool();
  if (!res) return false;
  printf("Done!\n");

  printf("Creating command buffer...");
  res &= createCommandBuffers();
  if (!res) return false;
  printf("Done!\n");

  printf("Creating sync objects...");
  res &= createSyncObjects();
  if (!res) return false;
  printf("Done!\n");

  printf("Init lights...");
  res &= initLights();
  if (!res) return false;
  printf("Done!\n");

  printf("Init giga mesh buffer...");
  res &= initGigaMeshBuffer();
  if (!res) return false;
  printf("Done!\n");

  printf("Init gpu buffers...");
  res &= initGpuBuffers();
  if (!res) return false;
  printf("Done!\n");

  printf("Init bindless...");
  res &= initBindless();
  if (!res) return false;
  printf("Done!\n");

  printf("Init render passes...");
  res &= initRenderPasses();
  if (!res) return false;
  printf("Done!\n");

  vkext::vulkanExtensionInit(_device);
  _renderOptions.raytracingEnabled = _enableRayTracing;

  //createParticles();

  printf("Init frame graph builder...");
  res &= initFrameGraphBuilder();
  if (!res) return false;
  printf("Done!\n");

  printf("Init imgui...");
  res &= initImgui();
  if (!res) return false;
  printf("Done!\n");  

  for (auto& val : _renderablesChanged) {
    val = false;
  }
  for (auto& val : _modelsChanged) {
    val = false;
  }
  for (auto& val : _materialsChanged) {
    val = false;
  }  

  std::vector<Vertex> vert;
  std::vector<std::uint32_t> inds;
  util::createSphere(1.0f, &vert, &inds, true);

  asset::Model sphereModel{};
  asset::Mesh sphereMesh{};
  _debugSphereMeshId = sphereMesh._id;
  sphereMesh._indices = std::move(inds);
  sphereMesh._vertices = std::move(vert);
  sphereModel._meshes.emplace_back(std::move(sphereMesh));

  AssetUpdate upd{};
  upd._addedModels.emplace_back(std::move(sphereModel));
  assetUpdate(std::move(upd));

  _delQ = internal::DeletionQueue(MAX_FRAMES_IN_FLIGHT, _vmaAllocator, _device);

  // Buffer for world pos requests, where depth value will be copied into
  bufferutil::createBuffer(
    _vmaAllocator,
    4, // 4 bytes for exactly 1 D32 float value
    VK_BUFFER_USAGE_TRANSFER_DST_BIT,
    VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT, // hopefully BAR mem
    _worldPosRequest._hostBuffer);

  return res;
}

// Stolen from here: https://gist.github.com/dgoguerra/7194777
std::string bytesToFormattedString(std::size_t bytes)
{
  char* suffix[] = { "B", "KB", "MB", "GB", "TB" };
  char length = sizeof(suffix) / sizeof(suffix[0]);

  int i = 0;
  double dblBytes = (double)bytes;

  if (bytes > 1024) {
    for (i = 0; (bytes / 1024) > 0 && i < length - 1; i++, bytes /= 1024)
      dblBytes = bytes / 1024.0;
  }

  static char output[200];
  sprintf_s(output, "%.02lf %s", dblBytes, suffix[i]);
  return output;
}

void printAssetUpdateInfo(
  std::size_t modelBytes,
  std::size_t materialBytes,
  std::size_t numModels,
  std::size_t numMeshes,
  std::size_t numSkeles,
  std::size_t numAnimations,
  std::size_t numAnimators,
  std::size_t numRendsAdd,
  std::size_t numRendsUpd, 
  std::size_t numRendsDel)
{
  printf("Asset Update info: \n");
  if (modelBytes > 0) {
    printf("\tModel bytes added: %s\n", bytesToFormattedString(modelBytes).c_str());
  }
  if (materialBytes > 0) {
    printf("\tMaterial bytes added: %s\n", bytesToFormattedString(materialBytes).c_str());
  }
  if (numModels > 0) {
    printf("\tNum models: %zu\n", numModels);
  }
  if (numMeshes > 0) {
    printf("\tNum meshes: %zu\n", numMeshes);
  }
  if (numSkeles > 0) {
    printf("\tNum skeles: %zu\n", numSkeles);
  }
  if (numAnimations > 0) {
    printf("\tNum animations: %zu\n", numAnimations);
  }
  if (numAnimators > 0) {
    printf("\tNum animators: %zu\n", numAnimators);
  }
  if (numRendsAdd > 0) {
    printf("\tNum rends added: %zu\n", numRendsAdd);
  }
  if (numRendsUpd > 0) {
    printf("\tNum rends updated: %zu\n", numRendsUpd);
  }
  if (numRendsDel > 0) {
    printf("\tNum rends deleted: %zu\n", numRendsDel);
  }
}

void VulkanRenderer::assetUpdate(AssetUpdate&& update)
{
  // TODO: Threading? Do this on a dedicated update thread, i.e. just queue this update here?

  std::size_t modelBytes = 0;
  std::size_t materialBytes = 0;
  std::size_t numMeshes = 0;

  bool rendIdMapUpdate = !update._removedRenderables.empty();
  bool modelIdMapUpdate = !update._removedModels.empty();
  bool materialIdMapUpdate = !update._removedMaterials.empty();
  bool forceModelChange = false;

  // Removed models (forces id map to update, could be expensive)
  for (auto& removedModel : update._removedModels) {
    // Find internal model and use memory interface to "free" memory
    auto it = _modelIdMap.find(removedModel);
    if (it == _modelIdMap.end()) {
      printf("Cannot remove model %s, it doesn't exist!\n", removedModel.str().c_str());
      continue;
    }

    auto& internalModel = _currentModels[it->second];
    for (auto& mesh : internalModel._meshes) {
      for (auto meshIt = _currentMeshes.begin(); meshIt != _currentMeshes.end();) {
        if (meshIt->_id == mesh) {
          // Put blas on del q
          if (_enableRayTracing) {
            auto& blas = _blases[mesh];
            _delQ.add(blas);

            _blases.erase(mesh);
          }

          _gigaVtxBuffer._memInterface.removeData(meshIt->_vertexHandle);
          _gigaIdxBuffer._memInterface.removeData(meshIt->_indexHandle);
          meshIt = _currentMeshes.erase(meshIt);
          break;
        }
        else {
          ++meshIt;
        }
      }
    }

    // Do a sanity check that no renderable uses this model
    for (auto it = _currentRenderables.begin(); it != _currentRenderables.end(); ++it) {
      if (it->_renderable._model == removedModel) {
        //_currentRenderables.erase(it);
        update._removedRenderables.emplace_back(it->_renderable._id);
        rendIdMapUpdate = true;
        printf("Warning! Removing renderable that still uses removed model!\n");
      }
    }

    _currentModels.erase(_currentModels.begin() + it->second);
  }

  // Model/mesh id map update
  if (modelIdMapUpdate) {
    _modelIdMap.clear();
    _meshIdMap.clear();

    for (std::size_t i = 0; i < _currentModels.size(); ++i) {
      _modelIdMap[_currentModels[i]._id] = i;
    }
    for (std::size_t i = 0; i < _currentMeshes.size(); ++i) {
      _meshIdMap[_currentMeshes[i]._id] = i;
    }

    modelIdMapUpdate = false;
  }

  // Added models
  for (auto& model : update._addedModels) {
    if (!model._id) {
      printf("Asset update fail: cannot add model with invalid id\n");
      continue;
    }

    if (_currentMeshes.size() == MAX_NUM_MESHES) {
      printf("Cannot add model, max num meshes reached!\n");
      continue;
    }

    // Note: Moving vertex and index data
    internal::InternalModel internalModel{};
    internalModel._id = model._id;

    for (const auto& mesh : model._meshes) {
      if (!mesh._id) {
        printf("Asset update fail: cannot add mesh with invalid id\n");
        continue;
      }

      modelBytes += mesh._vertices.size() * sizeof(Vertex);
      modelBytes += mesh._indices.size() * sizeof(uint32_t);
      numMeshes++;

      internal::InternalMesh internalMesh{};
      internalMesh._id = mesh._id;
      internalMesh._numIndices = static_cast<uint32_t>(mesh._indices.size());
      internalMesh._numVertices = static_cast<uint32_t>(mesh._vertices.size());

      internalModel._meshes.push_back(mesh._id);

      // vertex and index offset will be set once uploaded.

      _currentMeshes.push_back(internalMesh);
      auto internalId = _currentMeshes.size() - 1;
      _meshIdMap[mesh._id] = internalId;
    }

    _currentModels.push_back(std::move(internalModel));
    auto internalId = _currentModels.size() - 1;
    _modelIdMap[model._id] = internalId;

    _modelsToUpload.emplace_back(std::move(model));
  }

  // Removed materials
  for (auto& mat : update._removedMaterials) {
    for (auto it = _currentMaterials.begin(); it != _currentMaterials.end();) {
      if (it->_id == mat) {
        // TODO: Add to deletion q
        if (it->_albedoInfo) {
          _bindlessTextureMemIf.removeData(it->_albedoInfo._bindlessIndexHandle);

          vkDestroySampler(_device, it->_albedoInfo._sampler, nullptr);
          vmaDestroyImage(_vmaAllocator, it->_albedoInfo._image._image, it->_albedoInfo._image._allocation);
          vkDestroyImageView(_device, it->_albedoInfo._view, nullptr);
        }
        if (it->_metRoughInfo) {
          _bindlessTextureMemIf.removeData(it->_metRoughInfo._bindlessIndexHandle);

          vkDestroySampler(_device, it->_metRoughInfo._sampler, nullptr);
          vmaDestroyImage(_vmaAllocator, it->_metRoughInfo._image._image, it->_metRoughInfo._image._allocation);
          vkDestroyImageView(_device, it->_metRoughInfo._view, nullptr);
        }
        if (it->_normalInfo) {
          _bindlessTextureMemIf.removeData(it->_normalInfo._bindlessIndexHandle);

          vkDestroySampler(_device, it->_normalInfo._sampler, nullptr);
          vmaDestroyImage(_vmaAllocator, it->_normalInfo._image._image, it->_normalInfo._image._allocation);
          vkDestroyImageView(_device, it->_normalInfo._view, nullptr);
        }
        if (it->_emissiveInfo) {
          _bindlessTextureMemIf.removeData(it->_emissiveInfo._bindlessIndexHandle);

          vkDestroySampler(_device, it->_emissiveInfo._sampler, nullptr);
          vmaDestroyImage(_vmaAllocator, it->_emissiveInfo._image._image, it->_emissiveInfo._image._allocation);
          vkDestroyImageView(_device, it->_emissiveInfo._view, nullptr);
        }

        it = _currentMaterials.erase(it);
        break;
      } 
      else {
        ++it;
      }
    }
  }

  // Mat id map update
  if (materialIdMapUpdate) {
    _materialIdMap.clear();

    for (std::size_t i = 0; i < _currentMaterials.size(); ++i) {
      _materialIdMap[_currentMaterials[i]._id] = i;
    }

    materialIdMapUpdate = false;
  }

  // Added materials
  for (auto& mat : update._addedMaterials) {
    if (!mat._id) {
      printf("Material has invalid id, not adding!\n");
      continue;
    }

    if (_currentMaterials.size() == MAX_NUM_MATERIALS) {
      printf("Cannot add material, max size reached!\n");
      continue;
    }

    internal::InternalMaterial internalMat{};
    internalMat._emissive = mat._emissive;
    internalMat._baseColFactor = mat._baseColFactor;
    internalMat._id = mat._id;

    if (mat._albedoTex) materialBytes += mat._albedoTex.data.size();
    if (mat._metallicRoughnessTex) materialBytes += mat._metallicRoughnessTex.data.size();
    if (mat._normalTex) materialBytes += mat._normalTex.data.size();
    if (mat._emissiveTex) materialBytes += mat._emissiveTex.data.size();

    materialBytes += 4 * 4 + 3 * 4; // basecol and emissive

    _currentMaterials.push_back(internalMat);
    auto internalId = _currentMaterials.size() - 1;
    _materialIdMap[mat._id] = internalId;

    _materialsToUpload.emplace_back(std::move(mat));
  }

  // Updated materials
  for (auto& mat : update._updatedMaterials){
    if (!mat._id) {
      printf("Asset update fail: cannot update material with invalid id\n");
      continue;
    }

    auto it = _materialIdMap.find(mat._id);

    if (it == _materialIdMap.end()) {
      printf("Could not update material %s, doesn't exist!\n", mat._id.str().c_str());
      break;
    }

    // TODO: Currently only support updating basecolfac and emissive (not the textures)
    _currentMaterials[it->second]._baseColFactor = mat._baseColFactor;
    _currentMaterials[it->second]._emissive = mat._emissive;
  }

  // Removed animations
  for (auto remAnim : update._removedAnimations) {
    // TODO: What if a renderable references this animation?
    _animThread.removeAnimation(remAnim);
  }

  // Added animations
  for (auto& anim : update._addedAnimations) {
    _animThread.addAnimation(std::move(anim));
  }

  // Removed skeletons
  for (auto remSkel : update._removedSkeletons) {
    // TODO: What if a renderable references this skeleton?
    
    // Free from skeleton offset mem if
    bool exist = _skeletonOffsets.count(remSkel) > 0;

    if (!exist) {
      printf("WARNING! Removing skeleton with id %s, but does not exist in mem if!\n", remSkel.str().c_str());
    }
    else {
      _skeletonMemIf.removeData(_skeletonOffsets[remSkel]);
      _skeletonOffsets.erase(remSkel);
    }

    // Remove from animation runtime (actually frees the memory)
    _animThread.removeSkeleton(remSkel);
  }

  // Added skeletons
  for (auto& skel : update._addedSkeletons) {
    // Also update mem if of skeleton offsets
    std::size_t numMatrices = 0;
    if (skel._nonJointRoot) {
      numMatrices = skel._joints.size() - 1;
    }
    else {
      numMatrices = skel._joints.size();
    }

    auto handle = _skeletonMemIf.addData(numMatrices);
    if (!handle) {
      printf("Cannot add skeleton with id %s, can't fit into mem interface!\n", skel._id.str().c_str());
      continue;
    }

    _skeletonOffsets[skel._id] = std::move(handle);

    _animThread.addSkeleton(std::move(skel));
  }

  // Removed animators
  for (auto remAnimator : update._removedAnimators) {
    _animThread.removeAnimator(remAnimator);
  }

  // Updated animators
  for (auto& animator : update._updatedAnimators) {
    _animThread.updateAnimator(std::move(animator));
  }

  // Added animators
  for (auto& animator : update._addedAnimators) {
    _animThread.addAnimator(std::move(animator));
  }   
  
  // Removed renderables. This forces the id map to update aswell (potentially expensive)
  for (auto remId : update._removedRenderables) {
    for (auto it = _currentRenderables.begin(); it != _currentRenderables.end(); ++it) {
      if (it->_renderable._id == remId) {
        if (_enableRayTracing && !it->_dynamicMeshes.empty()) {

          modelIdMapUpdate = true;
          forceModelChange = true;

          if (_dynamicBlases.count(remId) > 0) {
            auto& dynamicBlas = _dynamicBlases[remId];

            for (std::size_t i = 0; i < it->_meshes.size(); ++i) {
              auto& mesh = it->_dynamicMeshes[i];

              for (auto meshIt = _currentMeshes.begin(); meshIt != _currentMeshes.end();) {
                if (meshIt->_id == mesh) {
                  // Put blas on deletion q
                  auto& blas = dynamicBlas[i];
                  _delQ.add(blas); // copy

                  // Remove meshes so that nothing uses the blas anymore
                  _gigaVtxBuffer._memInterface.removeData(meshIt->_vertexHandle);
                  _gigaIdxBuffer._memInterface.removeData(meshIt->_indexHandle);
                  meshIt = _currentMeshes.erase(meshIt);
                  break;
                }
                else {
                  ++meshIt;
                }
              }
            }

            _dynamicBlases.erase(remId);
            printf("erased id %s from dynamic blases\n", remId.str().c_str());
          }
        }
        else if (_enableRayTracing) {
          // If we're still in the process of adding this dynamic rend, remove it from the processing list
          for (auto it = _dynamicModelsToCopy.begin(); it != _dynamicModelsToCopy.end(); ++it) {
            if (it->_renderableId == remId) {

              // Add any potentially added blases to delQ
              for (auto& blas : it->_currentBlases) {
                _delQ.add(blas);
              }

              // Also give back potential meshes that have already been added
              for (std::size_t i = 0; i < it->_currentBlases.size(); ++i) {
                auto& meshId = it->_generatedDynamicIds[i];
                for (auto meshIt = _currentMeshes.begin(); meshIt != _currentMeshes.end();) {
                  if (meshIt->_id == meshId) {

                    // Remove meshes so that nothing uses the blas anymore
                    _gigaVtxBuffer._memInterface.removeData(meshIt->_vertexHandle);
                    _gigaIdxBuffer._memInterface.removeData(meshIt->_indexHandle);
                    meshIt = _currentMeshes.erase(meshIt);
                    break;
                  }
                  else {
                    ++meshIt;
                  }
                }
              }

              _dynamicModelsToCopy.erase(it);
              break;
            }
          }
        }
        _currentRenderables.erase(it);
        break;
      }
    }
  }

  // Rend id map update
  if (rendIdMapUpdate) {
    _renderableIdMap.clear();

    for (std::size_t i = 0; i < _currentRenderables.size(); ++i) {
      _renderableIdMap[_currentRenderables[i]._renderable._id] = i;
    }
  }

  // Added renderables
  for (const auto& rend : update._addedRenderables) {
    if (!rend._id) {
      printf("Asset update fail: cannot add renderable with invalid id\n");
      continue;
    }

    if (!rend._model) {
      printf("Asset update fail: cannot add renderable with invalid model id\n");
      continue;
    }

    if (_currentRenderables.size() == MAX_NUM_RENDERABLES) {
      printf("Cannot add renderable, max size reached!\n");
      continue;
    }

    internal::InternalRenderable internalRend{};
    internalRend._renderable = rend;

    // Fill in meshids
    auto& model = _currentModels[_modelIdMap[rend._model]];
    internalRend._meshes = model._meshes;

    if (rend._skeleton) {
      internalRend._skeletonOffset = (uint32_t)_skeletonOffsets[rend._skeleton]._offset;

      // If ray tracing is enabled, we need to write individual BLAS:es for each renderable that is animated.
      if (_enableRayTracing) {
        if (_currentMeshes.size() == MAX_NUM_MESHES) {
          printf("Cannot add dynamic BLAS for renderable, max num meshes reached!\n");
          continue;
        }

        // TODO: Don't do this if we already have this skeleton + model combination copied
        //       In that case, just point to the dynamic model already present
        DynamicModelCopyInfo copyInfo{};
        copyInfo._renderableId = internalRend._renderable._id;
        _dynamicModelsToCopy.emplace_back(std::move(copyInfo));

        for (std::size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
          _modelsChanged[i] = true;
        }
      }
    }

    _currentRenderables.push_back(internalRend);
    auto internalId = _currentRenderables.size() - 1;
    _renderableIdMap[rend._id] = internalId;
  }

  // Updated renderables
  for (const auto& rend : update._updatedRenderables) {
    if (!rend._id) {
      printf("Asset update fail: cannot update renderable with invalid id\n");
      continue;
    }

    if (!rend._model) {
      printf("Asset update fail: cannot update renderable with invalid model id\n");
      continue;
    }

    auto it = _renderableIdMap.find(rend._id);

    if (it == _renderableIdMap.end()) {
      printf("Could not update renderable %s, doesn't exist!\n", rend._id.str().c_str());
      break;
    }

    _currentRenderables[it->second]._renderable = rend;
  }

  // Mat id map update
  if (materialIdMapUpdate) {
    _materialIdMap.clear();

    for (std::size_t i = 0; i < _currentMaterials.size(); ++i) {
      _materialIdMap[_currentMaterials[i]._id] = i;
    }
  }

  // Model/mesh id map update
  if (modelIdMapUpdate) {
    _modelIdMap.clear();
    _meshIdMap.clear();

    for (std::size_t i = 0; i < _currentModels.size(); ++i) {
      _modelIdMap[_currentModels[i]._id] = i;
    }
    for (std::size_t i = 0; i < _currentMeshes.size(); ++i) {
      _meshIdMap[_currentMeshes[i]._id] = i;
    }
  }

  // Book-keeping
  bool modelChange = forceModelChange || !update._addedModels.empty() || !update._removedModels.empty();
  bool renderableChange = !update._addedRenderables.empty() || !update._removedRenderables.empty() || !update._updatedRenderables.empty();
  bool materialChange = !update._addedMaterials.empty() || !update._removedMaterials.empty() || !update._updatedMaterials.empty();

  if (modelChange) {
    for (std::size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      _modelsChanged[i] = true;
    }
  }

  if (renderableChange) {
    for (std::size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      _renderablesChanged[i] = true;
    }
  }

  if (materialChange) {
    for (std::size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      _materialsChanged[i] = true;
    }
  }

  printAssetUpdateInfo(
    modelBytes,
    materialBytes,
    update._addedModels.size(),
    numMeshes,
    update._addedSkeletons.size(),
    update._addedAnimations.size(),
    update._addedAnimators.size(),
    update._addedRenderables.size(),
    update._updatedRenderables.size(),
    update._removedRenderables.size());
  //printf("Asset update, %zu mesh bytes and %zu material bytes to upload\n", modelBytes, materialBytes);
}

void VulkanRenderer::uploadPendingModels(VkCommandBuffer cmdBuffer)
{
  for (auto& model : _modelsToUpload) {

    for (auto& mesh : model._meshes) {
      std::size_t vertSize = mesh._vertices.size() * sizeof(Vertex);
      std::size_t indSize = mesh._indices.size() * sizeof(std::uint32_t);
      std::size_t stagingBufSize = vertSize + indSize;

      uint8_t* data = nullptr;
      vmaMapMemory(_vmaAllocator, _gpuStagingBuffer[_currentFrame]._allocation, (void**)&data);

      // Offset according to current staging buffer usage
      data = data + _currentStagingOffset;

      internal::BufferMemoryInterface::Handle vertexHandle;
      internal::BufferMemoryInterface::Handle indexHandle;

      // Vertices first
      {
        // Find where to copy data in the fat buffer
        vertexHandle = _gigaVtxBuffer._memInterface.addData(vertSize);

        if (!vertexHandle) {
          printf("Could not add %zu bytes of vertex data! Make buffer bigger! Things won't work now!\n", vertSize);
          continue;
        }

        memcpy(data, mesh._vertices.data(), vertSize);

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = _currentStagingOffset;
        copyRegion.dstOffset = vertexHandle._offset;
        copyRegion.size = vertSize;
        vkCmdCopyBuffer(cmdBuffer, _gpuStagingBuffer[_currentFrame]._buffer, _gigaVtxBuffer._buffer._buffer, 1, &copyRegion);

        VkBufferMemoryBarrier memBarr{};
        memBarr.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        memBarr.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        memBarr.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
        memBarr.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        memBarr.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        memBarr.buffer = _gigaVtxBuffer._buffer._buffer;
        memBarr.offset = vertexHandle._offset;
        memBarr.size = vertSize;

        vkCmdPipelineBarrier(
          cmdBuffer,
          VK_PIPELINE_STAGE_TRANSFER_BIT,
          VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
          0, 0, nullptr,
          1, &memBarr,
          0, nullptr);
      }

      // Now indices
      if (indSize > 0) {
        // Find where to copy data in the fat buffer
        indexHandle = _gigaIdxBuffer._memInterface.addData(indSize);

        if (!indexHandle) {
          printf("Could not add %zu bytes of index data! Make buffer bigger! Things won't work now!\n", indSize);
          continue;
        }

        memcpy(data + vertSize, mesh._indices.data(), indSize);

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = _currentStagingOffset + vertSize;
        copyRegion.dstOffset = indexHandle._offset;
        copyRegion.size = indSize;
        vkCmdCopyBuffer(cmdBuffer, _gpuStagingBuffer[_currentFrame]._buffer, _gigaIdxBuffer._buffer._buffer, 1, &copyRegion);

        VkBufferMemoryBarrier memBarr{};
        memBarr.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        memBarr.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        memBarr.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_INDEX_READ_BIT;
        memBarr.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        memBarr.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        memBarr.buffer = _gigaIdxBuffer._buffer._buffer;
        memBarr.offset = indexHandle._offset;
        memBarr.size = indSize;

        vkCmdPipelineBarrier(
          cmdBuffer,
          VK_PIPELINE_STAGE_TRANSFER_BIT,
          VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
          0, 0, nullptr,
          1, &memBarr,
          0, nullptr);
      }

      // Update internal mesh
      auto idx = _meshIdMap[mesh._id];
      auto& internalMesh = _currentMeshes[idx];
      internalMesh._vertexHandle = vertexHandle;
      internalMesh._indexHandle = indexHandle;
      internalMesh._vertexOffset = static_cast<uint32_t>(vertexHandle._offset / sizeof(Vertex));
      internalMesh._indexOffset = indexHandle._offset == -1 ? -1 : static_cast<int64_t>(indexHandle._offset / sizeof(uint32_t));

      if (_enableRayTracing) {
        auto blas = registerBottomLevelAS(cmdBuffer, mesh._id);
        if (blas) {
          _blases[mesh._id] = std::move(blas);
        }

        // Retrieve device address of BLAS for use in TLAS update pass.
        if (_blases.find(mesh._id) != _blases.end()) {
          auto& blas = _blases[mesh._id];
          VkAccelerationStructureDeviceAddressInfoKHR addressInfo{};
          addressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
          addressInfo.accelerationStructure = blas._as;
          VkDeviceAddress blasAddress = GetAccelerationStructureDeviceAddressKHR(_device, &addressInfo);

          internalMesh._blasRef = blasAddress;
        }
      }

      _currentStagingOffset += stagingBufSize;
      vmaUnmapMemory(_vmaAllocator, _gpuStagingBuffer[_currentFrame]._allocation);
    }
  }

  _modelsToUpload.clear();
}

void VulkanRenderer::uploadPendingMaterials(VkCommandBuffer cmdBuffer)
{
  // Create all necessary textures and add to bindless.
  // This (currently) only needs to happen once, not for each multibuffer index.
  for (auto& mat : _materialsToUpload) {
    auto& internalMat = _currentMaterials[_materialIdMap[mat._id]];

    if (mat._albedoTex && !internalMat._albedoInfo) {      
      createTexture(
        cmdBuffer,
        VK_FORMAT_R8G8B8A8_SRGB,
        mat._albedoTex.width,
        mat._albedoTex.height,
        mat._albedoTex.data,
        internalMat._albedoInfo._sampler,
        internalMat._albedoInfo._image,
        internalMat._albedoInfo._view);

      internalMat._albedoInfo._bindlessIndexHandle = addTextureToBindless(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, internalMat._albedoInfo._view, internalMat._albedoInfo._sampler);
    }
    if (mat._metallicRoughnessTex && !internalMat._metRoughInfo) {
      createTexture(
        cmdBuffer,
        VK_FORMAT_R8G8B8A8_UNORM,
        mat._metallicRoughnessTex.width,
        mat._metallicRoughnessTex.height,
        mat._metallicRoughnessTex.data,
        internalMat._metRoughInfo._sampler,
        internalMat._metRoughInfo._image,
        internalMat._metRoughInfo._view);

      internalMat._metRoughInfo._bindlessIndexHandle = addTextureToBindless(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, internalMat._metRoughInfo._view, internalMat._metRoughInfo._sampler);
    }
    if (mat._normalTex && !internalMat._normalInfo) {
      createTexture(
        cmdBuffer,
        VK_FORMAT_R8G8B8A8_UNORM,
        mat._normalTex.width,
        mat._normalTex.height,
        mat._normalTex.data,
        internalMat._normalInfo._sampler,
        internalMat._normalInfo._image,
        internalMat._normalInfo._view);

      internalMat._normalInfo._bindlessIndexHandle = addTextureToBindless(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, internalMat._normalInfo._view, internalMat._normalInfo._sampler);
    }
    if (mat._emissiveTex && !internalMat._emissiveInfo) {
      createTexture(
        cmdBuffer,
        VK_FORMAT_R8G8B8A8_SRGB,
        mat._emissiveTex.width,
        mat._emissiveTex.height,
        mat._emissiveTex.data,
        internalMat._emissiveInfo._sampler,
        internalMat._emissiveInfo._image,
        internalMat._emissiveInfo._view);

      internalMat._emissiveInfo._bindlessIndexHandle = addTextureToBindless(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, internalMat._emissiveInfo._view, internalMat._emissiveInfo._sampler);
    }
  }

  // Do upload to GPU buffer. Do all current materials.
  std::size_t dataSize = _currentMaterials.size() * sizeof(gpu::GPUMaterialInfo);
  uint8_t* data;
  vmaMapMemory(_vmaAllocator, _gpuStagingBuffer[_currentFrame]._allocation, (void**)&data);

  // Offset according to current staging buffer usage
  data = data + _currentStagingOffset;

  gpu::GPUMaterialInfo* mappedData = reinterpret_cast<gpu::GPUMaterialInfo*>(data);

  for (std::size_t i = 0; i < _currentMaterials.size(); ++i) {
    auto& mat = _currentMaterials[i];

    mappedData[i]._baseColFac = glm::vec4(mat._baseColFactor.x, mat._baseColFactor.y, mat._baseColFactor.z, 0.0f);
    mappedData[i]._emissive = mat._emissive;
    mappedData[i]._bindlessIndices = glm::ivec4(
      mat._metRoughInfo._bindlessIndexHandle._offset, 
      mat._albedoInfo._bindlessIndexHandle._offset,
      mat._normalInfo._bindlessIndexHandle._offset,
      mat._emissiveInfo._bindlessIndexHandle._offset);
  }

  VkBufferCopy copyRegion{};
  copyRegion.dstOffset = 0;
  copyRegion.srcOffset = _currentStagingOffset;
  copyRegion.size = dataSize;
  vkCmdCopyBuffer(cmdBuffer, _gpuStagingBuffer[_currentFrame]._buffer, _gpuMaterialBuffer[_currentFrame]._buffer, 1, &copyRegion);

  VkBufferMemoryBarrier memBarr{};
  memBarr.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  memBarr.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  memBarr.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  memBarr.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  memBarr.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  memBarr.buffer = _gpuMaterialBuffer[_currentFrame]._buffer;
  memBarr.offset = 0;
  memBarr.size = VK_WHOLE_SIZE;

  vkCmdPipelineBarrier(
    cmdBuffer,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
    0, 0, nullptr,
    1, &memBarr,
    0, nullptr);

  vmaUnmapMemory(_vmaAllocator, _gpuStagingBuffer[_currentFrame]._allocation);

  // Update staging offset
  _currentStagingOffset += dataSize;

  _materialsToUpload.clear();
}

void VulkanRenderer::copyDynamicModels(VkCommandBuffer cmdBuffer)
{
  const std::size_t maxBuildsPerFrame = 5;
  std::size_t numBuilds = 0;

  std::vector<VkBufferMemoryBarrier> blasBarrs;
  std::vector<VkBufferMemoryBarrier> vtxBarrs;
  for (auto it = _dynamicModelsToCopy.begin(); it != _dynamicModelsToCopy.end();) {
    auto rend = it->_renderableId;
    auto& internalRend = _currentRenderables[_renderableIdMap[rend]];

    // Copy all meshes and BLASes
    auto& model = _currentModels[_modelIdMap[internalRend._renderable._model]];

    if (model._meshes.size() + _currentMeshes.size() >= MAX_NUM_MESHES) {
      printf("Cannot add dynamic mesh, max size reached!\n");
      continue;
    }

    if (it->_generatedDynamicIds.empty()) {
      it->_generatedDynamicIds.reserve(model._meshes.size());
      for (std::size_t i = 0; i < model._meshes.size(); ++i) {
        it->_generatedDynamicIds.emplace_back(util::Uuid::generate());
      }
    }

    bool abortRend = false;
    for (auto meshIt = model._meshes.begin() + it->_currentMeshIdx; meshIt != model._meshes.end(); ++meshIt) {
      if (numBuilds >= maxBuildsPerFrame) break;

      auto meshId = *meshIt;

      auto& internalMesh = _currentMeshes[_meshIdMap[meshId]];

      // This mesh may have not been uploaded yet
      if (!internalMesh._vertexHandle) {
        abortRend = true;
        continue;
      }

      internal::InternalMesh meshCopy{};
      meshCopy._id = it->_generatedDynamicIds[meshIt - model._meshes.begin()];
      meshCopy._numIndices = internalMesh._numIndices;
      meshCopy._numVertices = internalMesh._numVertices;

      auto vtxHandle = _gigaVtxBuffer._memInterface.addData(internalMesh._vertexHandle._size);
      auto idxHandle = _gigaIdxBuffer._memInterface.addData(internalMesh._indexHandle._size);

      if (!vtxHandle || !idxHandle) {
        printf("Could not copy mesh!\n");
        continue;
      }

      // Vertex
      {
        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = internalMesh._vertexHandle._offset;
        copyRegion.dstOffset = vtxHandle._offset;
        copyRegion.size = internalMesh._vertexHandle._size;
        vkCmdCopyBuffer(cmdBuffer, _gigaVtxBuffer._buffer._buffer, _gigaVtxBuffer._buffer._buffer, 1, &copyRegion);

        VkBufferMemoryBarrier memBarr{};
        memBarr.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        memBarr.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        memBarr.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_INDEX_READ_BIT;
        memBarr.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        memBarr.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        memBarr.buffer = _gigaVtxBuffer._buffer._buffer;
        memBarr.offset = vtxHandle._offset;
        memBarr.size = internalMesh._vertexHandle._size;

        vtxBarrs.emplace_back(std::move(memBarr));
      }

      // Index
      {
        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = internalMesh._indexHandle._offset;
        copyRegion.dstOffset = idxHandle._offset;
        copyRegion.size = internalMesh._indexHandle._size;
        vkCmdCopyBuffer(cmdBuffer, _gigaIdxBuffer._buffer._buffer, _gigaIdxBuffer._buffer._buffer, 1, &copyRegion);

        VkBufferMemoryBarrier memBarr{};
        memBarr.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        memBarr.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        memBarr.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_INDEX_READ_BIT;
        memBarr.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        memBarr.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        memBarr.buffer = _gigaIdxBuffer._buffer._buffer;
        memBarr.offset = idxHandle._offset;
        memBarr.size = internalMesh._indexHandle._size;

        vtxBarrs.emplace_back(std::move(memBarr));
      }

      meshCopy._vertexOffset = static_cast<uint32_t>(vtxHandle._offset / sizeof(Vertex));
      meshCopy._indexOffset = idxHandle._offset == -1 ? -1 : static_cast<int64_t>(idxHandle._offset / sizeof(uint32_t));
      meshCopy._vertexHandle = vtxHandle;
      meshCopy._indexHandle = idxHandle;

      std::size_t idx = _currentMeshes.size();
      auto idCopy = meshCopy._id;
      _meshIdMap[meshCopy._id] = idx;
      _currentMeshes.emplace_back(std::move(meshCopy));

      // Copy BLAS
      auto blas = registerBottomLevelAS(cmdBuffer, idCopy, true, false);

      if (!blas) {
        printf("Something went horribly wrong!\n");
        continue;
      }

      VkAccelerationStructureDeviceAddressInfoKHR addressInfo{};
      addressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
      addressInfo.accelerationStructure = blas._as;
      VkDeviceAddress blasAddress = GetAccelerationStructureDeviceAddressKHR(_device, &addressInfo);
      _currentMeshes.back()._blasRef = blasAddress;

      VkBufferMemoryBarrier postBarrier{};
      postBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
      postBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
      postBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
      postBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      postBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      postBarrier.buffer = blas._buffer._buffer;
      postBarrier.offset = 0;
      postBarrier.size = VK_WHOLE_SIZE;

      blasBarrs.emplace_back(std::move(postBarrier));

      it->_currentBlases.emplace_back(std::move(blas));

      numBuilds++;
      it->_currentMeshIdx++;
    }
    
    if (numBuilds >= maxBuildsPerFrame) break;
    if (abortRend) break;

    internalRend._dynamicMeshes = it->_generatedDynamicIds;
    _dynamicBlases[rend] = std::move(it->_currentBlases);
    printf("added id %s to dynamic blases, first dynamic mesh id is %s\n", rend.str().c_str(), internalRend._dynamicMeshes[0].str().c_str());
    it = _dynamicModelsToCopy.erase(it);
  }

  // Do all barriers at once
  vkCmdPipelineBarrier(cmdBuffer,
    VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
    VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR | VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
    0,
    0, nullptr,
    (uint32_t)blasBarrs.size(), blasBarrs.data(),
    0, nullptr);

  vkCmdPipelineBarrier(
    cmdBuffer,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    0, 0, nullptr,
    (uint32_t)vtxBarrs.size(), vtxBarrs.data(),
    0, nullptr);
}

void VulkanRenderer::update(
  const Camera& camera,
  const Camera& shadowCamera,
  const glm::vec4& lightDir,
  double delta,
  double time,
  bool lockCulling,
  RenderOptions renderOptions,
  RenderDebugOptions debugOptions)
  //logic::WindMap windMap)
{
  // TODO: We can't write to this frames UBO's until the GPU is finished with it.
  // If we run unlimited FPS we are quite likely to end up doing just that.
  // The ubo memory write _will_ be visible by the next queueSubmit, so that isn't the issue.
  // but we might be writing into memory that is currently accessed by the GPU.
  vkWaitForFences(_device, 1, &_inFlightFences[_currentFrame], VK_TRUE, UINT64_MAX);

  // TODO: Fix
  if (_enableRayTracing && !_topLevelBuilt) {
    buildTopLevelAS();
    writeTLASDescriptor();
    _topLevelBuilt = true;
  }

  bool probesDebugChanged = renderOptions.probesDebug != _renderOptions.probesDebug;

  _renderOptions = renderOptions;
  _debugOptions = debugOptions;
  //_currentWindMap = windMap;

  if (!_enableRayTracing) {
    _renderOptions.raytracedShadows = false;
    _renderOptions.raytracingEnabled = false;
    _renderOptions.ddgiEnabled = false;
    _renderOptions.multiBounceDdgiEnabled = false;
    _renderOptions.specularGiEnabled = false;
    _renderOptions.screenspaceProbes = false;
  }

  // Update bindless UBO
  auto shadowProj = shadowCamera.getProjection();
  auto proj = camera.getProjection();

  shadowProj[1][1] *= -1;
  proj[1][1] *= -1;

  auto invView = glm::inverse(camera.getCamMatrix());
  auto invProj = glm::inverse(proj);

  gpu::GPUSceneData ubo{};
  ubo.cameraPos = glm::vec4(camera.getPosition(), 1.0);
  ubo.cameraGridPos = glm::ivec4((int)floor(ubo.cameraPos.x), (int)floor(ubo.cameraPos.y), (int)floor(ubo.cameraPos.z), 1);
  ubo.invView = invView;
  ubo.invViewProj = invView * invProj;
  ubo.invProj = invProj;
  ubo.directionalShadowMatrixProj = shadowProj;
  ubo.directionalShadowMatrixView = shadowCamera.getCamMatrix();
  ubo.lightDir = lightDir;
  ubo.proj = proj;
  ubo.view = camera.getCamMatrix();
  ubo.viewVector = glm::vec4(camera.getForward(), 1.0);
  ubo.time = static_cast<float>(time);
  ubo.delta = static_cast<float>(delta);
  ubo.screenHeight = swapChainExtent().height;
  ubo.screenWidth = swapChainExtent().width;
  ubo.ssaoEnabled = _renderOptions.ssao;
  ubo.fxaaEnabled = _renderOptions.fxaa;
  ubo.directionalShadowsEnabled = _renderOptions.directionalShadows;
  ubo.rtShadowsEnabled = _renderOptions.raytracedShadows;
  ubo.ddgiEnabled = _renderOptions.ddgiEnabled;
  ubo.multiBounceDdgiEnabled = _renderOptions.multiBounceDdgiEnabled;
  ubo.specularGiEnabled = _renderOptions.specularGiEnabled;
  ubo.screenspaceProbesEnabled = _renderOptions.screenspaceProbes;
  ubo.visualizeBoundingSpheresEnabled = _renderOptions.visualizeBoundingSpheres;
  ubo.hack = _renderOptions.hack;
  ubo.sunIntensity = _renderOptions.sunIntensity;
  ubo.skyIntensity = _renderOptions.skyIntensity;
  ubo.exposure = _renderOptions.exposure;
  ubo.rtEnabled = _enableRayTracing;

  for (int i = 0; i < _lights.size(); ++i) {
    _lights[i].debugUpdatePos(delta);
    //ubo.lightColor[i] = glm::vec4(_lights[i]._color, 1.0);
    //ubo.lightPos[i] = glm::vec4(_lights[i]._pos, 0.0);
  }

  void* data;
  vmaMapMemory(_vmaAllocator, _gpuSceneDataBuffer[_currentFrame]._allocation, &data);
  memcpy(data, &ubo, sizeof(gpu::GPUSceneData));
  vmaUnmapMemory(_vmaAllocator, _gpuSceneDataBuffer[_currentFrame]._allocation);

  if (!lockCulling) {
    _latestCamera = camera;
  }
}

AccelerationStructure VulkanRenderer::registerBottomLevelAS(
  VkCommandBuffer cmdBuffer, 
  util::Uuid meshId,
  bool dynamic, 
  bool doBarrier)
{
  if (!dynamic && _blases.count(meshId) > 0) {
    printf("Cannot create BLAS for mesh id %s, it already exists!\n", meshId.str().c_str());
    return AccelerationStructure();
  }

  auto& mesh = _currentMeshes[_meshIdMap[meshId]];

  if (mesh._indexOffset == -1) {
    printf("Skipping blas for non-indexed mesh\n");
    return AccelerationStructure();
  }

  // Retrieve gigabuffer device address

  // According to spec it is ok to get address of offset data using a simple uint64_t offset
  // Also note, the vertexoffset and indexoffset are not _bytes_ but "numbers"
  VkDeviceAddress vertexAddress = _gigaVtxAddr + mesh._vertexOffset * sizeof(Vertex);
  VkDeviceAddress indexAddress = _gigaIdxAddr + (uint32_t)mesh._indexOffset * sizeof(uint32_t);

  // Now we create a structure for the triangle and index data for this mesh
  VkAccelerationStructureGeometryTrianglesDataKHR triangles{};
  triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
  triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
  triangles.vertexData.deviceAddress = vertexAddress;
  triangles.vertexStride = sizeof(Vertex);
  triangles.indexType = VK_INDEX_TYPE_UINT32;
  triangles.indexData.deviceAddress = indexAddress;
  triangles.maxVertex = mesh._numVertices - 1;
  triangles.transformData = { 0 }; // No transform

  // Point to the triangle data in the higher abstracted geometry structure (can contain other things than triangles)
  VkAccelerationStructureGeometryKHR geometry{};
  geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
  geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
  geometry.geometry.triangles = triangles;
  geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;

  // Range information for the build process
  VkAccelerationStructureBuildRangeInfoKHR rangeInfo{};
  rangeInfo.firstVertex = 0;
  rangeInfo.primitiveCount = mesh._numIndices / 3; // Number of triangles
  rangeInfo.primitiveOffset = 0;
  rangeInfo.transformOffset = 0;

  VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
  buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
  buildInfo.flags = dynamic ? VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR : VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
  buildInfo.geometryCount = 1;
  buildInfo.pGeometries = &geometry;
  buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
  buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
  buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;

  // We will set dstAccelerationStructure and scratchData once
  // we have created those objects.
  VkAccelerationStructureBuildSizesInfoKHR sizeInfo{};
  sizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
  GetAccelerationStructureBuildSizesKHR(
    _device,
    VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
    &buildInfo,
    &rangeInfo.primitiveCount,
    &sizeInfo);

  // Output buffer that will hold BLAS
  AccelerationStructure blas{};
  
  bufferutil::createBuffer(
    _vmaAllocator,
    sizeInfo.accelerationStructureSize,
    VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
    0,
    blas._buffer);

  // Create an empty acceleration structure object.

  VkAccelerationStructureCreateInfoKHR createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
  createInfo.type = buildInfo.type;
  createInfo.size = sizeInfo.accelerationStructureSize;
  createInfo.buffer = blas._buffer._buffer;
  createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
  createInfo.offset = 0;
  if (CreateAccelerationStructureKHR(_device, &createInfo, nullptr, &blas._as) != VK_SUCCESS) {
    printf("Could not create BLAS!\n");
    return AccelerationStructure();
  }
  
  buildInfo.dstAccelerationStructure = blas._as;
  blas._accelerationStructureSize = sizeInfo.accelerationStructureSize;

  // Create scratch buffer
  AllocatedBuffer scratchBuffer{};
  bufferutil::createBuffer(
    _vmaAllocator,
    sizeInfo.buildScratchSize,
    VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
    0,
    scratchBuffer);

  VkBufferDeviceAddressInfo scratchAddrInfo{};
  scratchAddrInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
  scratchAddrInfo.buffer = scratchBuffer._buffer;

  VkDeviceAddress scratchAddr = vkGetBufferDeviceAddress(_device, &scratchAddrInfo);
  buildInfo.scratchData.deviceAddress = scratchAddr;

  // Do the build
  VkAccelerationStructureBuildRangeInfoKHR* pRangeInfo = &rangeInfo;

  vkext::vkCmdBuildAccelerationStructuresKHR(
    cmdBuffer,
    1,
    &buildInfo,
    &pRangeInfo);

  blas._scratchBuffer = scratchBuffer;
  blas._scratchAddress = scratchAddr;
  blas._scratchBufferSize = sizeInfo.buildScratchSize;

  // Barrier
  if (doBarrier) {
    VkBufferMemoryBarrier postBarrier{};
    postBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    postBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
    postBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
    postBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    postBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    postBarrier.buffer = blas._buffer._buffer;
    postBarrier.offset = 0;
    postBarrier.size = VK_WHOLE_SIZE;

    vkCmdPipelineBarrier(cmdBuffer,
      VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
      VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR | VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
      0,
      0, nullptr,
      1, &postBarrier,
      0, nullptr);
  }

  return blas;
}

void VulkanRenderer::buildTopLevelAS()
{
  auto cmdBuffer = beginSingleTimeCommands();
  VkAccelerationStructureGeometryInstancesDataKHR instancesVk{};
  instancesVk.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
  instancesVk.arrayOfPointers = VK_FALSE;

  // Like creating the BLAS, point to the geometry (in this case, the
  // instances) in a polymorphic object.
  VkAccelerationStructureGeometryKHR geometry{};
  geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
  geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
  geometry.geometry.instances = instancesVk;

  // Create the build info: in this case, pointing to only one
  // geometry object.
  VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
  buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
  buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
  buildInfo.geometryCount = 1;
  buildInfo.pGeometries = &geometry;
  buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
  buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
  buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;
  
  // Query the worst-case AS size and scratch space size based on
  // the number of instances.
  uint32_t maxPrimitiveCount = MAX_NUM_RENDERABLES;
  VkAccelerationStructureBuildSizesInfoKHR sizeInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
  GetAccelerationStructureBuildSizesKHR(
    _device,
    VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
    &buildInfo,
    &maxPrimitiveCount,
    &sizeInfo);

  // Allocate a buffer for the acceleration structure.
  bufferutil::createBuffer(
    _vmaAllocator,
    sizeInfo.accelerationStructureSize,
    VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
    0,
    _tlas._buffer);

  // Create the acceleration structure object.
  // (Data has not yet been set.)
  VkAccelerationStructureCreateInfoKHR createInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
  createInfo.type = buildInfo.type;
  createInfo.size = sizeInfo.accelerationStructureSize;
  createInfo.buffer = _tlas._buffer._buffer;
  createInfo.offset = 0;

  if (CreateAccelerationStructureKHR(_device, &createInfo, nullptr, &_tlas._as) != VK_SUCCESS) {
    printf("Could not create TLAS!\n");
    return;
  }

  // Allocate the scratch buffer holding temporary build data.
  bufferutil::createBuffer(
    _vmaAllocator,
    sizeInfo.updateScratchSize + sizeInfo.buildScratchSize,
    VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
    0,
    _tlas._scratchBuffer);

  VkBufferDeviceAddressInfo scratchAddrInfo{};
  scratchAddrInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
  scratchAddrInfo.buffer = _tlas._scratchBuffer._buffer;
  _tlas._scratchAddress = vkGetBufferDeviceAddress(_device, &scratchAddrInfo);

  endSingleTimeCommands(cmdBuffer);
}

void VulkanRenderer::writeTLASDescriptor()
{
  // TODO: No checking is done here that the TLAS actually exists

  VkWriteDescriptorSetAccelerationStructureKHR descriptorAccelerationStructureInfo{};
  descriptorAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
  descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
  descriptorAccelerationStructureInfo.pAccelerationStructures = &_tlas._as;

  for (std::size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    VkWriteDescriptorSet accelerationStructureWrite{};
    accelerationStructureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    // The specialized acceleration structure descriptor has to be chained
    accelerationStructureWrite.pNext = &descriptorAccelerationStructureInfo;
    accelerationStructureWrite.dstSet = _bindlessDescriptorSets[i];
    accelerationStructureWrite.dstBinding = _tlasBinding;
    accelerationStructureWrite.descriptorCount = 1;
    accelerationStructureWrite.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

    vkUpdateDescriptorSets(_device, 1, &accelerationStructureWrite, 0, VK_NULL_HANDLE);
  }
}

std::vector<const char*> VulkanRenderer::getRequiredExtensions()
{
  uint32_t glfwExtensionCount = 0;
  const char** glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

  if (_enableValidationLayers) {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  return extensions;
}

bool VulkanRenderer::checkValidationLayerSupport()
{
  std::uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

  for (const char* layerName : _validationLayers) {
    bool layerFound = false;

    for (const auto& layerProperties : availableLayers) {
      if (strcmp(layerName, layerProperties.layerName) == 0) {
        layerFound = true;
        break;
      }
    }

    if (!layerFound) {
      printf("Validation layer %s not found!\n", layerName);
      return false;
    }
  }

  return true;
}

bool VulkanRenderer::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
  uint32_t extensionCount;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

  std::vector<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

  std::set<std::string> requiredExtensions;
  if (_enableRayTracing) {
    requiredExtensions = std::set<std::string>(_deviceExtensionsRT.begin(), _deviceExtensionsRT.end());
  }
  else {
    requiredExtensions = std::set<std::string>(_deviceExtensions.begin(), _deviceExtensions.end());
  }

  for (const auto& extension : availableExtensions) {
    requiredExtensions.erase(extension.extensionName);
  }

  return requiredExtensions.empty();
}

void VulkanRenderer::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
  createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo.pfnUserCallback = g_DebugCallback;
}

bool VulkanRenderer::createInstance()
{
  if (_enableValidationLayers && !checkValidationLayerSupport()) {
    printf("Validation layers requested but not available!\n");
    return false;
  }

  VkApplicationInfo appInfo{};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "Vulkan Renderer";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "anengine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_3;

  VkInstanceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;

  auto extensions = getRequiredExtensions();
  createInfo.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();

  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
  if (_enableValidationLayers) {
    createInfo.enabledLayerCount = static_cast<std::uint32_t>(_validationLayers.size());
    createInfo.ppEnabledLayerNames = _validationLayers.data();

    populateDebugMessengerCreateInfo(debugCreateInfo);
    createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
  }
  else {
    createInfo.enabledLayerCount = 0;

    createInfo.pNext = nullptr;
  }

  if (vkCreateInstance(&createInfo, nullptr, &_instance) != VK_SUCCESS) {
    printf("Renderer could not create instance!");
    return false;
  }

  return true;
}

bool VulkanRenderer::setupDebugMessenger()
{
  if (!_enableValidationLayers) {
    printf("Not setting up debug messenger, validation layers are turned off\n");
    return true;
  }

  VkDebugUtilsMessengerCreateInfoEXT createInfo{};
  populateDebugMessengerCreateInfo(createInfo);

  if (CreateDebugUtilsMessengerEXT(_instance, &createInfo, nullptr, &_debugMessenger) != VK_SUCCESS) {
    printf("Could not create debug messenger!\n");
    return false;
  }

  return true;
}

bool VulkanRenderer::pickPhysicalDevice()
{
  std::uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);
  if (deviceCount == 0) {
    printf("No phyiscal devices found!\n");
    return false;
  }

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.data());

  for (const auto& device : devices) {
    if (isDeviceSuitable(device)) {
      _physicalDevice = device;
      break;
    }
  }

  if (_physicalDevice == VK_NULL_HANDLE) {
    printf("Failed to find a suitable GPU!\n");
    return false;
  }

  return true;
}

bool VulkanRenderer::isDeviceSuitable(VkPhysicalDevice device)
{
  VkPhysicalDeviceProperties deviceProperties;
  vkGetPhysicalDeviceProperties(device, &deviceProperties);

  VkPhysicalDeviceFeatures deviceFeatures;
  vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

  auto queueFamIndices = findQueueFamilies(device);
  bool extensionsSupported = checkDeviceExtensionSupport(device);

  VkPhysicalDeviceDescriptorIndexingFeatures indexingFeatures{};
  indexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;

  VkPhysicalDeviceShaderAtomicFloatFeaturesEXT atomicFloatFeatures{};
  atomicFloatFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_FEATURES_EXT;
  atomicFloatFeatures.pNext = &indexingFeatures;

  VkPhysicalDeviceFeatures2 deviceFeatures2{};
  deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
  deviceFeatures2.pNext = &atomicFloatFeatures;

  VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtFeatures{};
  rtFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
  indexingFeatures.pNext = &rtFeatures;

  vkGetPhysicalDeviceFeatures2(device, &deviceFeatures2);

  bool swapChainAdequate = false;
  if (extensionsSupported) {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, _surface);
    swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
  }

  return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
           indexingFeatures.descriptorBindingPartiallyBound && indexingFeatures.runtimeDescriptorArray &&
           deviceFeatures.geometryShader && rtFeatures.rayTracingPipeline && queueFamIndices.isComplete() && 
           atomicFloatFeatures.shaderBufferFloat32AtomicAdd && atomicFloatFeatures.shaderSharedFloat32AtomicAdd &&
           extensionsSupported && swapChainAdequate && deviceFeatures.samplerAnisotropy;
}

QueueFamilyIndices VulkanRenderer::findQueueFamilies(VkPhysicalDevice device)
{
  QueueFamilyIndices indices;

  std::uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

  // TODO: Find a compute queue and a transfer-only queue.
  int i = 0;
  for (const auto& queueFamily : queueFamilies) {
    if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      indices.graphicsFamily = i;
    }

    // Async compute
    if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT &&
       !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
      indices.computeFamily = i;
    }

    // Transfer only (hopefully DMA)
    if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT &&
      !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
      !(queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)) {
      indices.transferFamily = i;
    }

    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, _surface, &presentSupport);
    if (presentSupport) {
      indices.presentFamily = i;
    }

    if (indices.isComplete()) {
      break;
    }

    i++;
  }

  return indices;
}

VkFormat VulkanRenderer::findDepthFormat()
{
  return findSupportedFormat(
    {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
    VK_IMAGE_TILING_OPTIMAL,
    VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
  );
}

VkFormat VulkanRenderer::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
  for (VkFormat format : candidates) {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(_physicalDevice, format, &props);

    if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
      return format;
    }
    else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
      return format;
    }
  }

  printf("Warning, could not find supported format\n");
  return VK_FORMAT_D32_SFLOAT;
}

bool VulkanRenderer::createLogicalDevice()
{
  auto familyIndices = findQueueFamilies(_physicalDevice);

  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t> uniqueQueueFamilies = {
    familyIndices.graphicsFamily.value(),
    familyIndices.presentFamily.value(),
    familyIndices.computeFamily.value(),
    familyIndices.transferFamily.value()};

  float queuePriority = 1.0f;
  for (uint32_t queueFamily : uniqueQueueFamilies) {
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back(queueCreateInfo);
  }

  VkPhysicalDeviceFeatures deviceFeatures{};
  deviceFeatures.samplerAnisotropy = VK_TRUE;
  deviceFeatures.multiDrawIndirect = VK_TRUE;
  deviceFeatures.fillModeNonSolid = VK_TRUE;
  deviceFeatures.shaderInt64 = VK_TRUE;

  VkDeviceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.pQueueCreateInfos = queueCreateInfos.data();
  createInfo.queueCreateInfoCount = static_cast<std::uint32_t>(queueCreateInfos.size());

  createInfo.pEnabledFeatures = &deviceFeatures;

  if (_enableRayTracing) {
    createInfo.enabledExtensionCount = static_cast<std::uint32_t>(_deviceExtensionsRT.size());
    createInfo.ppEnabledExtensionNames = _deviceExtensionsRT.data();
  }
  else {
    createInfo.enabledExtensionCount = static_cast<std::uint32_t>(_deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = _deviceExtensions.data();
  }

  if (_enableValidationLayers) {
    createInfo.enabledLayerCount = static_cast<uint32_t>(_validationLayers.size());
    createInfo.ppEnabledLayerNames = _validationLayers.data();
  } else {
    createInfo.enabledLayerCount = 0;
  }

  // Dynamic rendering
  VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeature{};
  dynamicRenderingFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
  dynamicRenderingFeature.dynamicRendering = VK_TRUE;

  createInfo.pNext = &dynamicRenderingFeature;

  // Atomic float
  VkPhysicalDeviceShaderAtomicFloatFeaturesEXT atomicFloatFeature{};
  atomicFloatFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_FEATURES_EXT;
  atomicFloatFeature.shaderBufferFloat32AtomicAdd = VK_TRUE;
  atomicFloatFeature.shaderSharedFloat32AtomicAdd = VK_TRUE;

  dynamicRenderingFeature.pNext = &atomicFloatFeature;

  // Bindless
  /*VkPhysicalDeviceDescriptorIndexingFeatures indexingFeatures{};
  indexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
  indexingFeatures.descriptorBindingPartiallyBound = VK_TRUE;
  indexingFeatures.descriptorBindingVariableDescriptorCount = VK_TRUE;
  indexingFeatures.runtimeDescriptorArray = VK_TRUE;
  indexingFeatures.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;

  dynamicRenderingFeature.pNext = &indexingFeatures;*/

  // filter min max and bindless
  VkPhysicalDeviceVulkan12Features vulkan12Features{};
  vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
  vulkan12Features.samplerFilterMinmax = VK_TRUE;
  vulkan12Features.descriptorBindingPartiallyBound = VK_TRUE;
  vulkan12Features.descriptorBindingVariableDescriptorCount = VK_TRUE;
  vulkan12Features.runtimeDescriptorArray = VK_TRUE;
  vulkan12Features.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
  vulkan12Features.descriptorBindingStorageImageUpdateAfterBind = VK_TRUE;
  vulkan12Features.bufferDeviceAddress = VK_TRUE;
  vulkan12Features.hostQueryReset = VK_TRUE;
  vulkan12Features.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;

  atomicFloatFeature.pNext = &vulkan12Features;

  // Ray tracing
  VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtPipeFeatures{};
  VkPhysicalDeviceAccelerationStructureFeaturesKHR asFeatures{};
  if (_enableRayTracing) {
    rtPipeFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
    rtPipeFeatures.rayTracingPipeline = VK_TRUE;
    vulkan12Features.pNext = &rtPipeFeatures;

    asFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    asFeatures.accelerationStructure = VK_TRUE;
    rtPipeFeatures.pNext = &asFeatures;
  }

  if (vkCreateDevice(_physicalDevice, &createInfo, nullptr, &_device) != VK_SUCCESS) {
    printf("Could not create logical device!\n");
    return false;
  }

  // Get RT props
  _rtPipeProps = VkPhysicalDeviceRayTracingPipelinePropertiesKHR{};
  _rtPipeProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
  VkPhysicalDeviceProperties2 deviceProps{};
  deviceProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
  deviceProps.pNext = &_rtPipeProps;
  vkGetPhysicalDeviceProperties2(_physicalDevice, &deviceProps);

  // TS period for interpreting timestamp queries
  _timestampPeriod = deviceProps.properties.limits.timestampPeriod;

  _queueIndices = familyIndices;
  vkGetDeviceQueue(_device, familyIndices.computeFamily.value(), 0, &_computeQ);
  vkGetDeviceQueue(_device, familyIndices.transferFamily.value(), 0, &_transferQ);
  vkGetDeviceQueue(_device, familyIndices.graphicsFamily.value(), 0, &_graphicsQ);
  vkGetDeviceQueue(_device, familyIndices.presentFamily.value(), 0, &_presentQ);

  return true;
}

bool VulkanRenderer::createVmaAllocator()
{
  VmaAllocatorCreateInfo allocatorCreateInfo = {};
  allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_3;
  allocatorCreateInfo.physicalDevice = _physicalDevice;
  allocatorCreateInfo.device = _device;
  allocatorCreateInfo.instance = _instance;
  allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

  vmaCreateAllocator(&allocatorCreateInfo, &_vmaAllocator);

  return true;
}

bool VulkanRenderer::createSurface()
{
  if (glfwCreateWindowSurface(_instance, _window, nullptr, &_surface) != VK_SUCCESS) {
    printf("Could not create window surface!\n");
    return false;
  }

  return true;
}

void VulkanRenderer::cleanupSwapChain()
{
  _swapChain.cleanup(_device);
}

void VulkanRenderer::recreateSwapChain()
{
  vkDeviceWaitIdle(_device);

  cleanupSwapChain();

  createSwapChain();

  _perFrameTimers.clear();

  _fgb.reset(this);

  for (auto& rp : _renderPasses) {
    rp->cleanup(this, &_vault);
  }

  _vault.clear(this);

  // Add the wind force image view to the vault so that the debug view can watch it
  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    auto res = new ImageRenderResource();
    res->_format = VK_FORMAT_R32_SFLOAT;
    res->_image = _gpuWindForceImage[i];
    res->_views.emplace_back(_gpuWindForceView[i]);
    _vault.addResource("WindForceImage", std::unique_ptr<IRenderResource>(res), true, i, true);

    // Add renderable buffer to vault, needed by particle system
    auto bufRes = new BufferRenderResource();
    bufRes->_buffer = _gpuRenderableBuffer[i];
    _vault.addResource("RenderableBuffer", std::unique_ptr<IRenderResource>(bufRes), true, i, true);

    {
      auto bufRes = new BufferRenderResource();
      bufRes->_buffer = _gigaVtxBuffer._buffer;
      _vault.addResource("GigaVtxBuffer", std::unique_ptr<IRenderResource>(bufRes), false, 0, true);
    }
  }

  initFrameGraphBuilder();
}

bool VulkanRenderer::createSwapChain()
{
  int width, height;
  glfwGetFramebufferSize(_window, &width, &height);
  return _swapChain.create(_device, _physicalDevice, _surface, _queueIndices, width, height) &&
         _swapChain.createImageViews(_device);
}

bool VulkanRenderer::createCommandPool()
{
  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex = _queueIndices.graphicsFamily.value();

  if (vkCreateCommandPool(_device, &poolInfo, nullptr, &_commandPool) != VK_SUCCESS) {
    printf("failed to create command pool!\n");
    return false;
  }

  return true;
}

bool VulkanRenderer::createQueryPool()
{
  _queryPools.resize(MAX_FRAMES_IN_FLIGHT);

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    VkQueryPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    createInfo.flags = 0; // Reserved for future use, must be 0!

    createInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
    createInfo.queryCount = (uint32_t)MAX_TIMESTAMP_QUERIES;

    VkResult result = vkCreateQueryPool(_device, &createInfo, nullptr, &_queryPools[i]);
    if (result != VK_SUCCESS) {
      printf("Could not create query pool!\n");
      return false;
    }

    vkResetQueryPool(_device, _queryPools[i], 0, (uint32_t)MAX_TIMESTAMP_QUERIES);
  }

  return true;
}

VkCommandBuffer VulkanRenderer::beginSingleTimeCommands()
{
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = _commandPool;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(_device, &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  return commandBuffer;
}

void VulkanRenderer::endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
  auto res = vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  vkQueueSubmit(_graphicsQ, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(_graphicsQ);

  vkFreeCommandBuffers(_device, _commandPool, 1, &commandBuffer);
}

VkPhysicalDeviceRayTracingPipelinePropertiesKHR VulkanRenderer::getRtPipeProps()
{
  return _rtPipeProps;
}

void VulkanRenderer::checkWorldPosCallback()
{
  if (!_worldPosRequest._callback || !_worldPosRequest._recorded) return;

  // If we are on the same frame, we should be done.
  // This absolutely necessitates that the worldPosCopy is done at the end of frame,
  // and this function call is done at the start of frame.
  if (_currentFrame == _worldPosRequest._frameWhenRecordedCopy) {
    // Map the host buffer and read the depth value back.
    void* data;
    vmaMapMemory(_vmaAllocator, _worldPosRequest._hostBuffer._allocation, &data);

    // Read exactly 4 bytes
    float depth = 0.0f;
    memcpy(&depth, data, 4);

    vmaUnmapMemory(_vmaAllocator, _worldPosRequest._hostBuffer._allocation);

    // Use last known camera to get the 3D position
    glm::vec2 screenSizePixels{ _swapChain._swapChainExtent.width, _swapChain._swapChainExtent.height };
    glm::vec4 clipSpacePos{
      (float)_worldPosRequest._viewportPos.x / screenSizePixels.x * 2.0f - 1.0f,
      (float)_worldPosRequest._viewportPos.y / screenSizePixels.y * 2.0f - 1.0f,
      depth,
      1.0f };

    auto proj = _latestCamera.getProjection();
    proj[1][1] *= -1;
    auto invView = glm::inverse(_latestCamera.getCamMatrix());
    auto invProj = glm::inverse(proj);
    auto invViewProj = invView * invProj;

    glm::vec4 position = invViewProj * clipSpacePos;
    position /= position.w;

    _worldPosRequest._callback(glm::vec3(position.x, position.y, position.z));

    // Reset
    _worldPosRequest._callback = {};
    _worldPosRequest._recorded = false;
  }
}

void VulkanRenderer::worldPosCopy(VkCommandBuffer cmdBuf)
{
  // Check if there is a request
  if (!_worldPosRequest._callback || _worldPosRequest._recorded) return;

  // Get depth buffer. This is a bit yikers since it is created in the frame graph but whatever
  auto depthRes = (ImageRenderResource*)_vault.getResource("GeometryDepthImage");
  if (!depthRes) {
    printf("Could not get depth resource for world pos request!\n");
    return;
  }

  // Depth image has to transition to transfer_src_optimal
  imageutil::transitionImageLayout(
    cmdBuf,
    depthRes->_image._image,
    depthRes->_format,
    VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

  // Record the copy command
  VkBufferImageCopy imCopy{};
  imCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  imCopy.imageSubresource.baseArrayLayer = 0;
  imCopy.imageSubresource.layerCount = 1;
  imCopy.imageSubresource.mipLevel = 0;
  imCopy.imageOffset.x = _worldPosRequest._viewportPos.x;
  imCopy.imageOffset.y = _worldPosRequest._viewportPos.y;
  imCopy.imageOffset.z = 0;
  imCopy.imageExtent.depth = 1;
  imCopy.imageExtent.height = 1;
  imCopy.imageExtent.width = 1;

  vkCmdCopyImageToBuffer(
    cmdBuf,
    depthRes->_image._image,
    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    _worldPosRequest._hostBuffer._buffer,
    1,
    &imCopy);

  // Barrier for the host buffer
  VkBufferMemoryBarrier memBarr{};
  memBarr.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  memBarr.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  memBarr.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
  memBarr.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  memBarr.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  memBarr.buffer = _worldPosRequest._hostBuffer._buffer;
  memBarr.offset = 0;
  memBarr.size = VK_WHOLE_SIZE;

  vkCmdPipelineBarrier(
    cmdBuf,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    VK_PIPELINE_STAGE_HOST_BIT,
    0, 0, nullptr,
    1, &memBarr,
    0, nullptr);

  // Transition depth back as to now disturb the frame graph
  imageutil::transitionImageLayout(
    cmdBuf,
    depthRes->_image._image,
    depthRes->_format,
    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);

  _worldPosRequest._frameWhenRecordedCopy = _currentFrame;
  _worldPosRequest._recorded = true;
}

int VulkanRenderer::findTimerIndex(const std::string& timer)
{
  for (std::size_t i = 0; i < _perFrameTimers.size(); ++i) {
    if (_perFrameTimers[i]._name == timer) {
      return static_cast<int>(i);
    }
  }
  
  return -1;
}

void VulkanRenderer::resetPerFrameQueries(VkCommandBuffer cmdBuffer)
{
  vkCmdResetQueryPool(cmdBuffer, _queryPools[_currentFrame], 0, MAX_TIMESTAMP_QUERIES);
}

void VulkanRenderer::computePerFrameQueries()
{
  // Do it for last frame (so hopefully is ready)
  uint32_t frame = _currentFrame;

  VkQueryResultFlags queryResFlags = VK_QUERY_RESULT_64_BIT;

  for (int idx = 0; idx < _perFrameTimers.size(); ++idx) {
    // The "stop" query index
    uint32_t queryIdx = idx * 2 + 1;

    uint64_t buffer[2];
    auto res = vkGetQueryPoolResults(
      _device,
      _queryPools[frame],
      queryIdx - 1,
      2,
      2 * sizeof(uint64_t),
      buffer,
      sizeof(uint64_t),
      queryResFlags);

    if (res == VK_SUCCESS) {
      PerFrameTimer& pfTimer = _perFrameTimers[idx];
      uint64_t diff = (buffer[1] - buffer[0]);
      double ns = diff * _timestampPeriod;

      pfTimer._durationMs = ns / 1000000.0f;

      pfTimer._cumulative10 += pfTimer._durationMs;
      pfTimer._cumulative100 += pfTimer._durationMs;

      pfTimer._currNum10++;
      pfTimer._currNum100++;

      if (pfTimer._currNum10 == 10) {
        pfTimer._currNum10 = 0;
        pfTimer._avg10 = pfTimer._cumulative10 / 10.0f;
        pfTimer._cumulative10 = 0.0f;
      }

      if (pfTimer._currNum100 == 100) {
        pfTimer._currNum100 = 0;
        pfTimer._avg100 = pfTimer._cumulative100 / 100.0f;
        pfTimer._cumulative100 = 0.0f;
      }

      pfTimer._buf.emplace_back(static_cast<float>(pfTimer._durationMs));
      pfTimer._buf.erase(pfTimer._buf.begin());
    }
  }
}

void VulkanRenderer::registerPerFrameTimer(const std::string& name, const std::string& group)
{
  PerFrameTimer timer{ name, group };
  timer._buf.resize(1000);
  _perFrameTimers.emplace_back(std::move(timer));
}

void VulkanRenderer::startTimer(const std::string& name, VkCommandBuffer cmdBuffer)
{
  int idx = findTimerIndex(name);

  if (idx == -1) {
    printf("Timer %s does not exist!\n", name.c_str());
    return;
  }

  uint32_t queryIdx = idx * 2;

  //printf("Query idx for start is %d, frame is %u, queryPool is %p\n", queryIdx, _currentFrame, _queryPools[_currentFrame]);
  vkCmdWriteTimestamp(cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, _queryPools[_currentFrame], queryIdx);
}

void VulkanRenderer::stopTimer(const std::string& name, VkCommandBuffer cmdBuffer)
{
  int idx = findTimerIndex(name);

  if (idx == -1) {
    printf("Timer %s does not exist!\n", name.c_str());
    return;
  }

  uint32_t queryIdx = idx * 2 + 1;

  //printf("Query idx for stop is %d, frame is %u\n", queryIdx, _currentFrame);
  vkCmdWriteTimestamp(cmdBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, _queryPools[_currentFrame], queryIdx);
}

internal::InternalMesh& VulkanRenderer::getSphereMesh()
{
  auto idx = _meshIdMap[_debugSphereMeshId];
  return _currentMeshes[idx];
}

size_t VulkanRenderer::getNumIrradianceProbesXZ()
{
  return NUM_IRRADIANCE_PROBES_XZ;
}

size_t VulkanRenderer::getNumIrradianceProbesY()
{
  return NUM_IRRADIANCE_PROBES_Y;
}

double VulkanRenderer::getElapsedTime()
{
  return glfwGetTime();
}

std::vector<PerFrameTimer> VulkanRenderer::getPerFrameTimers()
{
  return _perFrameTimers;
}

const Camera& VulkanRenderer::getCamera()
{
  return _latestCamera;
}

AccelerationStructure& VulkanRenderer::getTLAS()
{
  return _tlas;
}

VkFormat VulkanRenderer::getHDRFormat()
{
  return VK_FORMAT_R16G16B16A16_SFLOAT;
}

std::vector<Particle>& VulkanRenderer::getParticles()
{
  return _particles;
}

bool VulkanRenderer::blackboardValueBool(const std::string& key)
{
  auto exist = _blackboard.find(key) != _blackboard.end();
  
  if (exist) {
    return std::any_cast<bool>(_blackboard[key]);
  }

  return exist;
}

int VulkanRenderer::blackboardValueInt(const std::string& key)
{
  auto exist = _blackboard.find(key) != _blackboard.end();

  if (exist) {
    return std::any_cast<int>(_blackboard[key]);
  }

  return -1;
}

void VulkanRenderer::setBlackboardValueBool(const std::string& key, bool val)
{
  _blackboard[key] = val;
}

void VulkanRenderer::setBlackboardValueInt(const std::string& key, int val)
{
  _blackboard[key] = val;
}

bool VulkanRenderer::createDescriptorPool()
{
  VkDescriptorPoolSize poolSizes[] =
  {
    { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, (uint32_t)MAX_BINDLESS_RESOURCES },
    { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
    { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
    { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
    { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
    { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 },
    { VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1000 }
  };

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
  poolInfo.poolSizeCount = static_cast<uint32_t>(std::size(poolSizes));
  poolInfo.pPoolSizes = poolSizes;
  poolInfo.maxSets = 1000;

  if (vkCreateDescriptorPool(_device, &poolInfo, nullptr, &_descriptorPool) != VK_SUCCESS) {
    printf("failed to create descriptor pool!\n");
    return false;
  }

  return true;
}

bool VulkanRenderer::createCommandBuffers()
{
  _commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = _commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = static_cast<std::uint32_t>(_commandBuffers.size());

  if (vkAllocateCommandBuffers(_device, &allocInfo, _commandBuffers.data()) != VK_SUCCESS) {
    printf("failed to allocate command buffers!\n");
    return false;
  }

  return true;
}

void VulkanRenderer::prefillGPURendMatIdxBuffer(VkCommandBuffer& commandBuffer)
{
  // Each renderable references a material ID, one for each mesh its model uses.
  // However, some of the meshes may (and usually do) reference the same ID.
  // This buffer serves as an index buffer, so when rendering the shaders will
  // look at the renderable which will reference a "start point" in this buffer,
  // and then go to this buffer to actually find the material indices.

  if (_currentRenderables.empty()) return;

  uint8_t* data;
  vmaMapMemory(_vmaAllocator, _gpuStagingBuffer[_currentFrame]._allocation, (void**)&data);

  // Offset according to current staging buffer usage
  data = data + _currentStagingOffset;
  uint32_t* mappedData = reinterpret_cast<uint32_t*>(data);

  uint32_t currentIndex = 0;
  for (std::size_t i = 0; i < _currentRenderables.size(); ++i) {
    auto& renderable = _currentRenderables[i];
    renderable._materialIndexBufferIndex = currentIndex;

    for (auto& mat : renderable._renderable._materials) {
      auto idx = _materialIdMap[mat];
      mappedData[currentIndex] = static_cast<uint32_t>(idx);

      currentIndex++;
    }
  }

  std::size_t dataSize = (currentIndex + 1) * sizeof(uint32_t);
  VkBufferCopy copyRegion{};
  copyRegion.dstOffset = 0;
  copyRegion.srcOffset = _currentStagingOffset;
  copyRegion.size = dataSize;
  vkCmdCopyBuffer(commandBuffer, _gpuStagingBuffer[_currentFrame]._buffer, _gpuRenderableMaterialIndexBuffer[_currentFrame]._buffer, 1, &copyRegion);

  VkBufferMemoryBarrier memBarr{};
  memBarr.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  memBarr.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  memBarr.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  memBarr.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  memBarr.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  memBarr.buffer = _gpuRenderableMaterialIndexBuffer[_currentFrame]._buffer;
  memBarr.offset = 0;
  memBarr.size = VK_WHOLE_SIZE;

  vkCmdPipelineBarrier(
    commandBuffer,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    0, 0, nullptr,
    1, &memBarr,
    0, nullptr);

  vmaUnmapMemory(_vmaAllocator, _gpuStagingBuffer[_currentFrame]._allocation);

  // Update staging offset
  _currentStagingOffset += dataSize;
}

void VulkanRenderer::prefillGPUModelBuffer(VkCommandBuffer& commandBuffer)
{
  if (_currentModels.empty()) return;
  if (_currentRenderables.empty()) return;

  uint8_t* data;
  vmaMapMemory(_vmaAllocator, _gpuStagingBuffer[_currentFrame]._allocation, (void**)&data);

  // Offset according to current staging buffer usage
  data = data + _currentStagingOffset;

  std::uint32_t* mappedData = reinterpret_cast<std::uint32_t*>(data);

  uint32_t currIdx = 0;
  for (std::size_t i = 0; i < _currentRenderables.size(); ++i) {
    auto& rend = _currentRenderables[i];
    rend._modelBufferOffset = currIdx;

    for (auto& mesh: rend._meshes) {
      auto idx = _meshIdMap[mesh];
      mappedData[currIdx] = (uint32_t)idx;
      currIdx++;
    }

    if (_enableRayTracing && !rend._dynamicMeshes.empty()) {
      rend._dynamicModelBufferOffset = currIdx;

      for (auto& mesh : rend._dynamicMeshes) {
        auto idx = _meshIdMap[mesh];
        mappedData[currIdx] = (uint32_t)idx;
        currIdx++;
      }
    }
  }

  std::size_t dataSize = (currIdx + 1) * sizeof(uint32_t);
  VkBufferCopy copyRegion{};
  copyRegion.dstOffset = 0;
  copyRegion.srcOffset = _currentStagingOffset;
  copyRegion.size = dataSize;
  vkCmdCopyBuffer(commandBuffer, _gpuStagingBuffer[_currentFrame]._buffer, _gpuModelBuffer[_currentFrame]._buffer, 1, &copyRegion);

  VkBufferMemoryBarrier memBarr{};
  memBarr.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  memBarr.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  memBarr.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
  memBarr.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  memBarr.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  memBarr.buffer = _gpuModelBuffer[_currentFrame]._buffer;
  memBarr.offset = 0;
  memBarr.size = VK_WHOLE_SIZE;

  vkCmdPipelineBarrier(
    commandBuffer,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    0, 0, nullptr,
    1, &memBarr,
    0, nullptr);

  vmaUnmapMemory(_vmaAllocator, _gpuStagingBuffer[_currentFrame]._allocation);

  // Update staging offset
  _currentStagingOffset += dataSize;
}

void VulkanRenderer::prefillGPURenderableBuffer(VkCommandBuffer& commandBuffer)
{
  if (_currentRenderables.empty()) return;

  // Each renderable that we currently have on the CPU needs to be udpated for the GPU buffer.
  std::size_t dataSize = _currentRenderables.size() * sizeof(gpu::GPURenderable);
  uint8_t* data;
  vmaMapMemory(_vmaAllocator, _gpuStagingBuffer[_currentFrame]._allocation, (void**)&data);

  // Offset according to current staging buffer usage
  data = data + _currentStagingOffset;

  gpu::GPURenderable* mappedData = reinterpret_cast<gpu::GPURenderable*>(data);

  // Also fill mesh usage buffer while we're looping through renderables anyway
  _currentMeshUsage.clear();

  for (std::size_t i = 0; i < _currentRenderables.size(); ++i) {
    auto& renderable = _currentRenderables[i]._renderable;
    auto internalModelIdx = _modelIdMap[renderable._model];
    auto& model = _currentModels[internalModelIdx];

    for (uint32_t i = 0; i < model._meshes.size(); ++i) {
      _currentMeshUsage[model._meshes[i]]++;
    }
    
    mappedData[i]._transform = renderable._transform;
    mappedData[i]._tint = glm::vec4(renderable._tint, 1.0f);
    mappedData[i]._modelOffset = _currentRenderables[i]._modelBufferOffset;
    mappedData[i]._numMeshes = (uint32_t)model._meshes.size();
    mappedData[i]._skeletonOffset = _currentRenderables[i]._skeletonOffset;
    mappedData[i]._bounds = renderable._boundingSphere;
    mappedData[i]._visible = renderable._visible ? 1 : 0;
    mappedData[i]._firstMaterialIndex = _currentRenderables[i]._materialIndexBufferIndex;
    mappedData[i]._dynamicModelOffset = _currentRenderables[i]._dynamicModelBufferOffset;
  }

  VkBufferCopy copyRegion{};
  copyRegion.dstOffset = 0;
  copyRegion.srcOffset = _currentStagingOffset;
  copyRegion.size = dataSize;
  vkCmdCopyBuffer(commandBuffer, _gpuStagingBuffer[_currentFrame]._buffer, _gpuRenderableBuffer[_currentFrame]._buffer, 1, &copyRegion);

  VkBufferMemoryBarrier memBarr{};
  memBarr.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  memBarr.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  memBarr.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
  memBarr.srcQueueFamilyIndex = _queueIndices.graphicsFamily.value();
  memBarr.dstQueueFamilyIndex = _queueIndices.graphicsFamily.value();
  memBarr.buffer = _gpuRenderableBuffer[_currentFrame]._buffer;
  memBarr.offset = 0;
  memBarr.size = VK_WHOLE_SIZE;

  vkCmdPipelineBarrier(
    commandBuffer,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    0, 0, nullptr, 
    1, &memBarr,
    0, nullptr);

  vmaUnmapMemory(_vmaAllocator, _gpuStagingBuffer[_currentFrame]._allocation);

  // Update staging offset
  _currentStagingOffset += dataSize;
}

void VulkanRenderer::prefillGPUMeshBuffer(VkCommandBuffer& commandBuffer)
{
  // Go through each mesh of each model and update corresponding spot in the buffer
  std::size_t dataSize = _currentMeshes.size() * sizeof(gpu::GPUMeshInfo);
  uint8_t* data;
  vmaMapMemory(_vmaAllocator, _gpuStagingBuffer[_currentFrame]._allocation, (void**)&data);

  // Offset according to current staging buffer usage
  data = data + _currentStagingOffset;

  gpu::GPUMeshInfo* mappedData = reinterpret_cast<gpu::GPUMeshInfo*>(data);

  for (std::size_t i = 0; i < _currentMeshes.size(); ++i) {
    auto& mesh = _currentMeshes[i];

    mappedData[i]._vertexOffset = mesh._vertexOffset;
    mappedData[i]._indexOffset = (uint32_t)mesh._indexOffset;
    mappedData[i]._blasRef = mesh._blasRef;
  }

  VkBufferCopy copyRegion{};
  copyRegion.dstOffset = 0;
  copyRegion.srcOffset = _currentStagingOffset;
  copyRegion.size = dataSize;
  vkCmdCopyBuffer(commandBuffer, _gpuStagingBuffer[_currentFrame]._buffer, _gpuMeshInfoBuffer[_currentFrame]._buffer, 1, &copyRegion);

  VkBufferMemoryBarrier memBarr{};
  memBarr.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  memBarr.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  memBarr.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  memBarr.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  memBarr.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  memBarr.buffer = _gpuMeshInfoBuffer[_currentFrame]._buffer;
  memBarr.offset = 0;
  memBarr.size = VK_WHOLE_SIZE;

  vkCmdPipelineBarrier(
    commandBuffer,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
    0, 0, nullptr,
    1, &memBarr,
    0, nullptr);

  vmaUnmapMemory(_vmaAllocator, _gpuStagingBuffer[_currentFrame]._allocation);

  // Update staging offset
  _currentStagingOffset += dataSize;
}

void VulkanRenderer::prefillGPUSkeletonBuffer(VkCommandBuffer& commandBuffer, std::vector<anim::Skeleton>& skeletons)
{
  if (skeletons.empty()) return;

  std::size_t dataSize = 0;
  for (auto& skel : skeletons) {
    dataSize += skel._joints.size() * sizeof(float) * 16; // one mat4 per joint
  }

  uint8_t* data;
  vmaMapMemory(_vmaAllocator, _gpuStagingBuffer[_currentFrame]._allocation, (void**)&data);

  // Offset according to current staging buffer usage
  data = data + _currentStagingOffset;

  glm::mat4* mappedData = reinterpret_cast<glm::mat4*>(data);

  for (std::size_t i = 0; i < skeletons.size(); ++i) {
    std::size_t j = 0;

    if (skeletons[i]._nonJointRoot) {
      j = 1;
    }

    std::size_t currentLocalIndex = 0;
    std::size_t indexOffset = _skeletonOffsets[skeletons[i]._id]._offset;
    for (; j < skeletons[i]._joints.size(); ++j) {
      mappedData[indexOffset + currentLocalIndex] = skeletons[i]._joints[j]._globalTransform * skeletons[i]._joints[j]._inverseBindMatrix;
      currentLocalIndex++;
    }
  }

  VkBufferCopy copyRegion{};
  copyRegion.dstOffset = 0;
  copyRegion.srcOffset = _currentStagingOffset;
  copyRegion.size = dataSize;
  vkCmdCopyBuffer(commandBuffer, _gpuStagingBuffer[_currentFrame]._buffer, _gpuSkeletonBuffer[_currentFrame]._buffer, 1, &copyRegion);

  VkBufferMemoryBarrier memBarr{};
  memBarr.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  memBarr.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  memBarr.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  memBarr.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  memBarr.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  memBarr.buffer = _gpuSkeletonBuffer[_currentFrame]._buffer;
  memBarr.offset = 0;
  memBarr.size = VK_WHOLE_SIZE;

  vkCmdPipelineBarrier(
    commandBuffer,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
    0, 0, nullptr,
    1, &memBarr,
    0, nullptr);

  vmaUnmapMemory(_vmaAllocator, _gpuStagingBuffer[_currentFrame]._allocation);

  // Update staging offset
  _currentStagingOffset += dataSize;
}

void VulkanRenderer::prefillGPULightBuffer(VkCommandBuffer& commandBuffer)
{
  std::size_t dataSize = MAX_NUM_LIGHTS * sizeof(gpu::GPULight);
  uint8_t* data;
  vmaMapMemory(_vmaAllocator, _gpuStagingBuffer[_currentFrame]._allocation, (void**)&data);

  // Offset according to current staging buffer usage
  data = data + _currentStagingOffset;

  gpu::GPULight* mappedData = reinterpret_cast<gpu::GPULight*>(data);

  for (std::size_t i = 0; i < MAX_NUM_LIGHTS; ++i) {
    mappedData[i]._color = glm::vec4(0.0);
    if (_lights.size() > i) {
      auto& light = _lights[i];

      mappedData[i]._worldPos = glm::vec4(light._pos, light._range);
      mappedData[i]._color = glm::vec4(light._color, light._enabled ? 1.0 : 0.0);
    }
  }

  VkBufferCopy copyRegion{};
  copyRegion.dstOffset = 0;
  copyRegion.srcOffset = _currentStagingOffset;
  copyRegion.size = dataSize;
  vkCmdCopyBuffer(commandBuffer, _gpuStagingBuffer[_currentFrame]._buffer, _gpuLightBuffer[_currentFrame]._buffer, 1, &copyRegion);

  VkBufferMemoryBarrier memBarr{};
  memBarr.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  memBarr.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  memBarr.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  memBarr.srcQueueFamilyIndex = _queueIndices.graphicsFamily.value();
  memBarr.dstQueueFamilyIndex = _queueIndices.graphicsFamily.value();
  memBarr.buffer = _gpuLightBuffer[_currentFrame]._buffer;
  memBarr.offset = 0;
  memBarr.size = VK_WHOLE_SIZE;

  vkCmdPipelineBarrier(
    commandBuffer,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    0, 0, nullptr,
    1, &memBarr,
    0, nullptr);

  vmaUnmapMemory(_vmaAllocator, _gpuStagingBuffer[_currentFrame]._allocation);

  // Update staging offset
  _currentStagingOffset += dataSize;
}

void VulkanRenderer::updateWindForceImage(VkCommandBuffer& commandBuffer)
{
  /*std::size_t dataSize = _currentWindMap.height * _currentWindMap.width * 4;
  uint8_t* data;
  vmaMapMemory(_vmaAllocator, _gpuStagingBuffer[_currentFrame]._allocation, (void**)&data);

  // Offset according to current staging buffer usage
  data = data + _currentStagingOffset;
  auto mappedData = reinterpret_cast<float*>(data);

  for (int x = 0; x < _currentWindMap.width; x++)
    for (int y = 0; y < _currentWindMap.height; y++) {
      mappedData[x * _currentWindMap.width + y] = (_currentWindMap.data[x * _currentWindMap.width + y] + 1.0f) / 2.0f;
  }

  // Transition image to dst optimal
  imageutil::transitionImageLayout(
    commandBuffer,
    _gpuWindForceImage[_currentFrame]._image,
    VK_FORMAT_R32_SFLOAT,
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  VkExtent3D extent{};
  extent.depth = 1;
  extent.height = _currentWindMap.height;
  extent.width = _currentWindMap.width;

  VkOffset3D offset{};
  offset.x = 0;
  offset.y = 0;
  offset.z = 0;

  VkBufferImageCopy imCopy{};
  imCopy.bufferOffset = _currentStagingOffset;
  imCopy.bufferImageHeight = 0;
  imCopy.bufferRowLength = 0;
  imCopy.imageExtent = extent;
  imCopy.imageOffset = offset;
  imCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imCopy.imageSubresource.mipLevel = 0;
  imCopy.imageSubresource.baseArrayLayer = 0;
  imCopy.imageSubresource.layerCount = 1;

  vkCmdCopyBufferToImage(
    commandBuffer,
    _gpuStagingBuffer[_currentFrame]._buffer,
    _gpuWindForceImage[_currentFrame]._image,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    1,
    &imCopy);

  // Transition to shader read
  imageutil::transitionImageLayout(
    commandBuffer,
    _gpuWindForceImage[_currentFrame]._image,
    VK_FORMAT_R32_SFLOAT,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  vmaUnmapMemory(_vmaAllocator, _gpuStagingBuffer[_currentFrame]._allocation);

  // Update staging offset
  _currentStagingOffset += dataSize;*/
}

bool VulkanRenderer::createSyncObjects()
{
  _imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  _renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  _inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    if (vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &_imageAvailableSemaphores[i]) != VK_SUCCESS ||
      vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &_renderFinishedSemaphores[i]) != VK_SUCCESS ||
      vkCreateFence(_device, &fenceInfo, nullptr, &_inFlightFences[i]) != VK_SUCCESS) {
      printf("failed to create semaphores!\n");
      return false;
    }
  }

  return true;
}

bool VulkanRenderer::initLights()
{
  //_lights.resize(MAX_NUM_LIGHTS);

  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_real_distribution<> col(0.0, 1.0);
  std::uniform_real_distribution<> pos(0.0, 100.0);
  std::uniform_real_distribution<> rad(0.0, 10.0);
  std::uniform_real_distribution<> speed(.1, .2);

  for (std::size_t i = 0; i < _lights.size(); ++i) {
    auto& light = _lights[i];
    float zNear = 0.1f;
    float zFar = 50.0f;

    light._type = Light::Point;

    glm::vec3 randomPos{
      pos(rng),
      2.0f,
      pos(rng)
    };

    light._color = glm::normalize(glm::vec3(col(rng), col(rng), col(rng)));
    light._pos = randomPos;// glm::vec3(1.0f * i, 5.0f, 0.0f);
    light._proj = glm::perspective(glm::radians(90.0f), 1.0f, zNear, zFar);
    light._debugCircleRadius = rad(rng);
    light._debugSpeed = speed(rng);
    light._debugOrigX = light._pos.x;
    light._debugOrigZ = light._pos.z;
    light._range = 5.0;

    // Construct view matrices
    light.updateViewMatrices();
  }

  return true;
}

bool VulkanRenderer::initBindless()
{
  // Create a "use-for-all" pipeline layout, descriptor set layout and descriptor sets.

  /*
  * Bindings:
  * 0: Scene UBO (UBO)
  * 1: Wind force (Sampler)
  * 2: Renderable buffer (SSBO)
  * 3: Light buffer (SSBO)
  * 4: View cluster buffer (SSBO)
  * 5: Mesh material info (SSBO)
  * 6: Giga idx buffer (SSBO)
  * 7: Giga vertex buffer (SSBO)
  * 8: Mesh buffer (SSBO)
  * 9: TLAS for ray tracing (TLAS)
  * 10: Skeleton buffer (SSBO)
  * 11: Id Map buffer (SSBO)
  * 12: Bindless textures (sampler array)
  */

  uint32_t uboBinding = 0;
  uint32_t windForceBinding = 1;
  uint32_t renderableBinding = 2;
  uint32_t lightBinding = 3;
  uint32_t clusterBinding = 4;
  uint32_t materialBinding = 5;

  // Descriptor set layout
  {
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = uboBinding;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR;
    bindings.emplace_back(std::move(uboLayoutBinding));

    VkDescriptorSetLayoutBinding renderableLayoutBinding{};
    renderableLayoutBinding.binding = renderableBinding;
    renderableLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    renderableLayoutBinding.descriptorCount = 1;
    renderableLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
    bindings.emplace_back(std::move(renderableLayoutBinding));

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = windForceBinding;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
    bindings.emplace_back(std::move(samplerLayoutBinding));

    VkDescriptorSetLayoutBinding lightLayoutBinding{};
    lightLayoutBinding.binding = lightBinding;
    lightLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    lightLayoutBinding.descriptorCount = 1;
    lightLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    bindings.emplace_back(std::move(lightLayoutBinding));

    VkDescriptorSetLayoutBinding clusterLayoutBinding{};
    clusterLayoutBinding.binding = clusterBinding;
    clusterLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    clusterLayoutBinding.descriptorCount = 1;
    clusterLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    bindings.emplace_back(std::move(clusterLayoutBinding));

    VkDescriptorSetLayoutBinding materialLayoutBinding{};
    materialLayoutBinding.binding = materialBinding;
    materialLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    materialLayoutBinding.descriptorCount = 1;
    materialLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    bindings.emplace_back(std::move(materialLayoutBinding));

    VkDescriptorSetLayoutBinding rendMatIdxBinding{};
    rendMatIdxBinding.binding = _renderableMatIndexBinding;
    rendMatIdxBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    rendMatIdxBinding.descriptorCount = 1;
    rendMatIdxBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    bindings.emplace_back(std::move(rendMatIdxBinding));

    VkDescriptorSetLayoutBinding modelBinding{};
    modelBinding.binding = _modelBinding;
    modelBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    modelBinding.descriptorCount = 1;
    modelBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    bindings.emplace_back(std::move(modelBinding));

    VkDescriptorSetLayoutBinding idxLayoutBinding{};
    idxLayoutBinding.binding = _gigaIdxBinding;
    idxLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    idxLayoutBinding.descriptorCount = 1;
    idxLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    bindings.emplace_back(std::move(idxLayoutBinding));

    VkDescriptorSetLayoutBinding vtxLayoutBinding{};
    vtxLayoutBinding.binding = _gigaVtxBinding;
    vtxLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    vtxLayoutBinding.descriptorCount = 1;
    vtxLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    bindings.emplace_back(std::move(vtxLayoutBinding));

    VkDescriptorSetLayoutBinding meshLayoutBinding{};
    meshLayoutBinding.binding = _meshBinding;
    meshLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    meshLayoutBinding.descriptorCount = 1;
    meshLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_COMPUTE_BIT;
    bindings.emplace_back(std::move(meshLayoutBinding));

    VkDescriptorSetLayoutBinding tlasLayoutBinding{};
    tlasLayoutBinding.binding = _tlasBinding;
    tlasLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    tlasLayoutBinding.descriptorCount = 1;
    tlasLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    bindings.emplace_back(std::move(tlasLayoutBinding));

    VkDescriptorSetLayoutBinding skeletonLayoutBinding{};
    skeletonLayoutBinding.binding = _skeletonBinding;
    skeletonLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    skeletonLayoutBinding.descriptorCount = 1;
    skeletonLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    bindings.emplace_back(std::move(skeletonLayoutBinding));

    VkDescriptorSetLayoutBinding texLayoutBinding{};
    texLayoutBinding.binding = _bindlessTextureBinding;
    texLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    texLayoutBinding.descriptorCount = MAX_BINDLESS_RESOURCES;
    texLayoutBinding.stageFlags = VK_SHADER_STAGE_ALL;
    bindings.emplace_back(std::move(texLayoutBinding));

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = (uint32_t)bindings.size();
    layoutInfo.pBindings = bindings.data();

    VkDescriptorSetLayoutBindingFlagsCreateInfo extendedInfo{};
    std::vector<VkDescriptorBindingFlags> flags(bindings.size() - 1, 0);
    VkDescriptorBindingFlags bindlessFlags =
      VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT |
      VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT |
      VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT;

    flags.emplace_back(bindlessFlags);

    layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;

    extendedInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    extendedInfo.bindingCount = (uint32_t)bindings.size();
    extendedInfo.pBindingFlags = flags.data();

    layoutInfo.pNext = &extendedInfo;

    if (vkCreateDescriptorSetLayout(_device, &layoutInfo, nullptr, &_bindlessDescSetLayout) != VK_SUCCESS) {
      printf("failed to create descriptor set layout!\n");
      return false;
    }
  }

  // Pipeline layout
  {
    uint32_t pushConstantsSize = 4 * 4 * 4;

    // Push constants
    VkPushConstantRange pushConstant;
    pushConstant.offset = 0;
    pushConstant.size = 256;// (uint32_t)pushConstantsSize;
    pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &_bindlessDescSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstant;

    if (vkCreatePipelineLayout(_device, &pipelineLayoutInfo, nullptr, &_bindlessPipelineLayout) != VK_SUCCESS) {
      printf("failed to create pipeline layout!\n");
      return false;
    }
  }

  // Descriptor sets (for each frame)
  {
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, _bindlessDescSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = _descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    VkDescriptorSetVariableDescriptorCountAllocateInfo countInfo{};
    uint32_t maxBinding = MAX_BINDLESS_RESOURCES - 1;
    std::vector<uint32_t> counts(MAX_FRAMES_IN_FLIGHT, maxBinding);

    countInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
    countInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    countInfo.pDescriptorCounts = counts.data();

    allocInfo.pNext = &countInfo;

    _bindlessDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(_device, &allocInfo, _bindlessDescriptorSets.data()) != VK_SUCCESS) {
      printf("failed to allocate descriptor sets!\n");
      return false;
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      std::vector<VkWriteDescriptorSet> descriptorWrites;

      // Scene data UBO
      VkDescriptorBufferInfo bufferInfo{};
      VkWriteDescriptorSet sceneBufWrite{};
      bufferInfo.buffer = _gpuSceneDataBuffer[i]._buffer;
      bufferInfo.offset = 0;
      bufferInfo.range = sizeof(gpu::GPUSceneData);

      sceneBufWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      sceneBufWrite.dstSet = _bindlessDescriptorSets[i];
      sceneBufWrite.dstBinding = uboBinding;
      sceneBufWrite.dstArrayElement = 0;
      sceneBufWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      sceneBufWrite.descriptorCount = 1;
      sceneBufWrite.pBufferInfo = &bufferInfo;

      descriptorWrites.emplace_back(std::move(sceneBufWrite));

      // Renderable SSBO
      VkDescriptorBufferInfo rendBufferInfo{};
      VkWriteDescriptorSet rendBufWrite{};

      rendBufferInfo.buffer = _gpuRenderableBuffer[i]._buffer;
      rendBufferInfo.offset = 0;
      rendBufferInfo.range = VK_WHOLE_SIZE;

      rendBufWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      rendBufWrite.dstSet = _bindlessDescriptorSets[i];
      rendBufWrite.dstBinding = renderableBinding;
      rendBufWrite.dstArrayElement = 0;
      rendBufWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
      rendBufWrite.descriptorCount = 1;
      rendBufWrite.pBufferInfo = &rendBufferInfo;

      descriptorWrites.emplace_back(std::move(rendBufWrite));

      // Light SSBO
      VkDescriptorBufferInfo lightBufferInfo{};
      VkWriteDescriptorSet lightBufWrite{};

      lightBufferInfo.buffer = _gpuLightBuffer[i]._buffer;
      lightBufferInfo.offset = 0;
      lightBufferInfo.range = VK_WHOLE_SIZE;

      lightBufWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      lightBufWrite.dstSet = _bindlessDescriptorSets[i];
      lightBufWrite.dstBinding = lightBinding;
      lightBufWrite.dstArrayElement = 0;
      lightBufWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
      lightBufWrite.descriptorCount = 1;
      lightBufWrite.pBufferInfo = &lightBufferInfo;

      descriptorWrites.emplace_back(std::move(lightBufWrite));

      // Cluster SSBO
      VkDescriptorBufferInfo clusterBufferInfo{};
      VkWriteDescriptorSet clusterBufWrite{};

      clusterBufferInfo.buffer = _gpuViewClusterBuffer[i]._buffer;
      clusterBufferInfo.offset = 0;
      clusterBufferInfo.range = VK_WHOLE_SIZE;

      clusterBufWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      clusterBufWrite.dstSet = _bindlessDescriptorSets[i];
      clusterBufWrite.dstBinding = clusterBinding;
      clusterBufWrite.dstArrayElement = 0;
      clusterBufWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
      clusterBufWrite.descriptorCount = 1;
      clusterBufWrite.pBufferInfo = &clusterBufferInfo;

      descriptorWrites.emplace_back(std::move(clusterBufWrite));

      // Material info SSBO
      VkDescriptorBufferInfo matBufferInfo{};
      VkWriteDescriptorSet matBufWrite{};

      matBufferInfo.buffer = _gpuMaterialBuffer[i]._buffer;
      matBufferInfo.offset = 0;
      matBufferInfo.range = VK_WHOLE_SIZE;

      matBufWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      matBufWrite.dstSet = _bindlessDescriptorSets[i];
      matBufWrite.dstBinding = materialBinding;
      matBufWrite.dstArrayElement = 0;
      matBufWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
      matBufWrite.descriptorCount = 1;
      matBufWrite.pBufferInfo = &matBufferInfo;

      descriptorWrites.emplace_back(std::move(matBufWrite));

      // Renderable material index buffer
      VkDescriptorBufferInfo rendMatIdxBufInfo{};
      VkWriteDescriptorSet rendMatIdxBufWrite{};

      rendMatIdxBufInfo.buffer = _gpuRenderableMaterialIndexBuffer[i]._buffer;
      rendMatIdxBufInfo.offset = 0;
      rendMatIdxBufInfo.range = VK_WHOLE_SIZE;

      rendMatIdxBufWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      rendMatIdxBufWrite.dstSet = _bindlessDescriptorSets[i];
      rendMatIdxBufWrite.dstBinding = _renderableMatIndexBinding;
      rendMatIdxBufWrite.dstArrayElement = 0;
      rendMatIdxBufWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
      rendMatIdxBufWrite.descriptorCount = 1;
      rendMatIdxBufWrite.pBufferInfo = &rendMatIdxBufInfo;

      descriptorWrites.emplace_back(std::move(rendMatIdxBufWrite));

      // Model mesh index buffer
      VkDescriptorBufferInfo modelBufInfo{};
      VkWriteDescriptorSet modelBufWrite{};

      modelBufInfo.buffer = _gpuModelBuffer[i]._buffer;
      modelBufInfo.offset = 0;
      modelBufInfo.range = VK_WHOLE_SIZE;

      modelBufWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      modelBufWrite.dstSet = _bindlessDescriptorSets[i];
      modelBufWrite.dstBinding = _modelBinding;
      modelBufWrite.dstArrayElement = 0;
      modelBufWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
      modelBufWrite.descriptorCount = 1;
      modelBufWrite.pBufferInfo = &modelBufInfo;

      descriptorWrites.emplace_back(std::move(modelBufWrite));

      // Idx buffer
      VkDescriptorBufferInfo idxBufferInfo{};
      VkWriteDescriptorSet idxBufWrite{};

      idxBufferInfo.buffer = _gigaIdxBuffer._buffer._buffer;
      idxBufferInfo.offset = 0;
      idxBufferInfo.range = VK_WHOLE_SIZE;

      idxBufWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      idxBufWrite.dstSet = _bindlessDescriptorSets[i];
      idxBufWrite.dstBinding = _gigaIdxBinding;
      idxBufWrite.dstArrayElement = 0;
      idxBufWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
      idxBufWrite.descriptorCount = 1;
      idxBufWrite.pBufferInfo = &idxBufferInfo;

      descriptorWrites.emplace_back(std::move(idxBufWrite));

      // Vtx buffer
      VkDescriptorBufferInfo vtxBufferInfo{};
      VkWriteDescriptorSet vtxBufWrite{};

      vtxBufferInfo.buffer = _gigaVtxBuffer._buffer._buffer;
      vtxBufferInfo.offset = 0;
      vtxBufferInfo.range = VK_WHOLE_SIZE;

      vtxBufWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      vtxBufWrite.dstSet = _bindlessDescriptorSets[i];
      vtxBufWrite.dstBinding = _gigaVtxBinding;
      vtxBufWrite.dstArrayElement = 0;
      vtxBufWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
      vtxBufWrite.descriptorCount = 1;
      vtxBufWrite.pBufferInfo = &vtxBufferInfo;

      descriptorWrites.emplace_back(std::move(vtxBufWrite));

      // Mesh buffer
      VkDescriptorBufferInfo meshBufferInfo{};
      VkWriteDescriptorSet meshBufWrite{};

      meshBufferInfo.buffer = _gpuMeshInfoBuffer[i]._buffer;
      meshBufferInfo.offset = 0;
      meshBufferInfo.range = VK_WHOLE_SIZE;

      meshBufWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      meshBufWrite.dstSet = _bindlessDescriptorSets[i];
      meshBufWrite.dstBinding = _meshBinding;
      meshBufWrite.dstArrayElement = 0;
      meshBufWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
      meshBufWrite.descriptorCount = 1;
      meshBufWrite.pBufferInfo = &meshBufferInfo;

      descriptorWrites.emplace_back(std::move(meshBufWrite));

      // Skeleton buffer
      VkDescriptorBufferInfo skeletonBufferInfo{};
      VkWriteDescriptorSet skeletonBufWrite{};

      skeletonBufferInfo.buffer = _gpuSkeletonBuffer[i]._buffer;
      skeletonBufferInfo.offset = 0;
      skeletonBufferInfo.range = VK_WHOLE_SIZE;

      skeletonBufWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      skeletonBufWrite.dstSet = _bindlessDescriptorSets[i];
      skeletonBufWrite.dstBinding = _skeletonBinding;
      skeletonBufWrite.dstArrayElement = 0;
      skeletonBufWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
      skeletonBufWrite.descriptorCount = 1;
      skeletonBufWrite.pBufferInfo = &skeletonBufferInfo;

      descriptorWrites.emplace_back(std::move(skeletonBufWrite));

      // Samplers
      std::vector<VkDescriptorImageInfo> imageInfos;
      VkWriteDescriptorSet imWrite{};
      VkDescriptorImageInfo imageInfo{};
      imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      imageInfo.imageView = _gpuWindForceView[i];
      imageInfo.sampler = _gpuWindForceSampler[i];
      imageInfos.emplace_back(std::move(imageInfo));

      imWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      imWrite.dstSet = _bindlessDescriptorSets[i];
      imWrite.dstBinding = windForceBinding;
      imWrite.dstArrayElement = 0;
      imWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      imWrite.descriptorCount = 1;
      imWrite.pImageInfo = imageInfos.data();

      descriptorWrites.emplace_back(std::move(imWrite));

      vkUpdateDescriptorSets(_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
  }

  return true;
}

bool VulkanRenderer::initFrameGraphBuilder()
{
  for (auto* rp : _renderPasses) {
    rp->init(this, &_vault);
    rp->registerToGraph(_fgb, this);
  }

  auto res = _fgb.build(this, &_vault);
  _fgb.printBuiltGraphDebug();

  return res;
}

bool VulkanRenderer::initRenderPasses()
{
  _renderPasses.emplace_back(new HiZRenderPass());
  _renderPasses.emplace_back(new ParticleUpdatePass());
  _renderPasses.emplace_back(new CullRenderPass());
  _renderPasses.emplace_back(new ShadowRenderPass());
  _renderPasses.emplace_back(new GrassShadowRenderPass());
  _renderPasses.emplace_back(new GeometryRenderPass());
  _renderPasses.emplace_back(new GrassRenderPass());
  _renderPasses.emplace_back(new UpdateBlasPass());
  _renderPasses.emplace_back(new UpdateTLASPass());
  _renderPasses.emplace_back(new IrradianceProbeTranslationPass());
  _renderPasses.emplace_back(new IrradianceProbeRayTracingPass()); // RT
  _renderPasses.emplace_back(new IrradianceProbeConvolvePass());
  //_renderPasses.emplace_back(new LightShadowRayTracingPass());
  //_renderPasses.emplace_back(new LightShadowSumPass());
  _renderPasses.emplace_back(new ShadowRayTracingPass()); // RT
  _renderPasses.emplace_back(new SpecularGIRTPass()); //RT
  _renderPasses.emplace_back(new SpecularGIMipPass());
  //_renderPasses.emplace_back(new SSGlobalIlluminationRayTracingPass());
  //_renderPasses.emplace_back(new SSGIBlurRenderPass());
  //_renderPasses.emplace_back(new SurfelUpdateRayTracingPass());
  //_renderPasses.emplace_back(new SurfelSHPass());
  //_renderPasses.emplace_back(new SurfelConvolveRenderPass());
  _renderPasses.emplace_back(new SSAORenderPass());
  _renderPasses.emplace_back(new SSAOBlurRenderPass());
  _renderPasses.emplace_back(new DeferredLightingRenderPass());
  _renderPasses.emplace_back(new LuminanceHistogramPass());
  _renderPasses.emplace_back(new LuminanceAveragePass());
  _renderPasses.emplace_back(new BloomRenderPass());
  _renderPasses.emplace_back(new FXAARenderPass());
  _renderPasses.emplace_back(new DebugBSRenderPass());
  _renderPasses.emplace_back(new DebugViewRenderPass());
  _renderPasses.emplace_back(new UIRenderPass());
  _renderPasses.emplace_back(new PresentationRenderPass());

  return true;
}

bool VulkanRenderer::initGigaMeshBuffer()
{
  // Allocate "big enough" size... 
  bufferutil::createBuffer(_vmaAllocator, _gigaVtxBuffer._memInterface.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 0, _gigaVtxBuffer._buffer);
  bufferutil::createBuffer(_vmaAllocator, _gigaIdxBuffer._memInterface.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 0, _gigaIdxBuffer._buffer);

  // Get addresses
  VkBufferDeviceAddressInfo vertAddrInfo{};
  vertAddrInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
  vertAddrInfo.buffer = _gigaVtxBuffer._buffer._buffer;

  VkBufferDeviceAddressInfo indAddrInfo{};
  indAddrInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
  indAddrInfo.buffer = _gigaIdxBuffer._buffer._buffer;

  _gigaVtxAddr = vkGetBufferDeviceAddress(_device, &vertAddrInfo);
  _gigaIdxAddr = vkGetBufferDeviceAddress(_device, &indAddrInfo);

  return true;
}

bool VulkanRenderer::initGpuBuffers()
{
  _gpuStagingBuffer.resize(MAX_FRAMES_IN_FLIGHT);
  _gpuRenderableBuffer.resize(MAX_FRAMES_IN_FLIGHT);
  _gpuMaterialBuffer.resize(MAX_FRAMES_IN_FLIGHT);
  _gpuRenderableMaterialIndexBuffer.resize(MAX_FRAMES_IN_FLIGHT);
  _gpuModelBuffer.resize(MAX_FRAMES_IN_FLIGHT);
  _gpuMeshInfoBuffer.resize(MAX_FRAMES_IN_FLIGHT);
  _gpuSceneDataBuffer.resize(MAX_FRAMES_IN_FLIGHT);
  _gpuWindForceSampler.resize(MAX_FRAMES_IN_FLIGHT);
  _gpuWindForceImage.resize(MAX_FRAMES_IN_FLIGHT);
  _gpuWindForceView.resize(MAX_FRAMES_IN_FLIGHT);
  _gpuLightBuffer.resize(MAX_FRAMES_IN_FLIGHT);
  _gpuViewClusterBuffer.resize(MAX_FRAMES_IN_FLIGHT);
  _gpuSkeletonBuffer.resize(MAX_FRAMES_IN_FLIGHT);

  for (std::size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    // Create a staging buffer that will be used to temporarily hold data that is to be copied to the gpu buffers.
    bufferutil::createBuffer(
      _vmaAllocator,
      STAGING_BUFFER_SIZE_MB * 1024 * 1024,
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
      _gpuStagingBuffer[i]);
    std::string name = "gpuStagingBuffer_" + std::to_string(i);
    setDebugName(VK_OBJECT_TYPE_BUFFER, (uint64_t)_gpuStagingBuffer[i]._buffer, name.c_str());

    // Create a buffer that will contain renderable information for use by the frustum culling compute shader.
    bufferutil::createBuffer(
      _vmaAllocator,
      MAX_NUM_RENDERABLES * sizeof(gpu::GPURenderable),
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      0,
      _gpuRenderableBuffer[i]);
    name = "gpuRenderableBuffer" + std::to_string(i);
    setDebugName(VK_OBJECT_TYPE_BUFFER, (uint64_t)_gpuRenderableBuffer[i]._buffer, name.c_str());

    // Create a buffer that will contain renderable information for use by the frustum culling compute shader.
    bufferutil::createBuffer(
      _vmaAllocator,
      MAX_NUM_MATERIALS * sizeof(gpu::GPUMaterialInfo),
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      0,
      _gpuMaterialBuffer[i]);
    name = "_gpuMaterialBuffer" + std::to_string(i);
    setDebugName(VK_OBJECT_TYPE_BUFFER, (uint64_t)_gpuMaterialBuffer[i]._buffer, name.c_str());

    bufferutil::createBuffer(
      _vmaAllocator,
      MAX_NUM_RENDERABLES * sizeof(uint32_t) * 5, // assume max of all renderables and avg 5 meshes per renderable... ish 200kB
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      0,
      _gpuRenderableMaterialIndexBuffer[i]);
    name = "_gpuRenderableMaterialIndexBuffer" + std::to_string(i);
    setDebugName(VK_OBJECT_TYPE_BUFFER, (uint64_t)_gpuRenderableMaterialIndexBuffer[i]._buffer, name.c_str());

    bufferutil::createBuffer(
      _vmaAllocator,
      sizeof(uint32_t) * MAX_NUM_MESHES,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      0,
      _gpuModelBuffer[i]);
    name = "_gpuModelBuffer" + std::to_string(i);
    setDebugName(VK_OBJECT_TYPE_BUFFER, (uint64_t)_gpuModelBuffer[i]._buffer, name.c_str());

    // Create a buffer that will contain renderable information for use by the frustum culling compute shader.
    bufferutil::createBuffer(
      _vmaAllocator,
      MAX_NUM_MESHES * sizeof(gpu::GPUMeshInfo),
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      0,
      _gpuMeshInfoBuffer[i]);
    name = "_gpuMeshInfoBuffer" + std::to_string(i);
    setDebugName(VK_OBJECT_TYPE_BUFFER, (uint64_t)_gpuMeshInfoBuffer[i]._buffer, name.c_str());

    // Used as a UBO in most shaders for accessing scene data.
    bufferutil::createBuffer(
      _vmaAllocator,
      sizeof(gpu::GPUSceneData),
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT, // If this turns out to be a bottle neck we have to switch to a staging buffer
      _gpuSceneDataBuffer[i]);
    name = "_gpuSceneDataBuffer" + std::to_string(i);
    setDebugName(VK_OBJECT_TYPE_BUFFER, (uint64_t)_gpuSceneDataBuffer[i]._buffer, name.c_str());

    bufferutil::createBuffer(
      _vmaAllocator,
      MAX_NUM_LIGHTS * sizeof(gpu::GPULight),
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      0,
      _gpuLightBuffer[i]);
    name = "_gpuLightBuffer" + std::to_string(i);
    setDebugName(VK_OBJECT_TYPE_BUFFER, (uint64_t)_gpuLightBuffer[i]._buffer, name.c_str());

    bufferutil::createBuffer(
      _vmaAllocator,
      120 * 120 * 10 * sizeof(gpu::GPUViewCluster),
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      0,
      _gpuViewClusterBuffer[i]);
    name = "_gpuViewClusterBuffer" + std::to_string(i);
    setDebugName(VK_OBJECT_TYPE_BUFFER, (uint64_t)_gpuViewClusterBuffer[i]._buffer, name.c_str());

    bufferutil::createBuffer(
      _vmaAllocator,
      sizeof(glm::mat4) * MAX_NUM_JOINTS * MAX_NUM_SKINNED_MODELS,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      0,
      _gpuSkeletonBuffer[i]);
    name = "_gpuSkeletonBuffer" + std::to_string(i);
    setDebugName(VK_OBJECT_TYPE_BUFFER, (uint64_t)_gpuSkeletonBuffer[i]._buffer, name.c_str());

    // Wind force sampler
    VkSamplerCreateInfo samplerCreate{};
    samplerCreate.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCreate.magFilter = VK_FILTER_NEAREST;
    samplerCreate.minFilter = VK_FILTER_NEAREST;
    samplerCreate.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerCreate.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCreate.addressModeV = samplerCreate.addressModeU;
    samplerCreate.addressModeW = samplerCreate.addressModeU;
    samplerCreate.mipLodBias = 0.0f;
    samplerCreate.maxAnisotropy = 1.0f;
    samplerCreate.minLod = 0.0f;
    samplerCreate.maxLod = 1.0f;
    samplerCreate.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

    if (vkCreateSampler(_device, &samplerCreate, nullptr, &_gpuWindForceSampler[i]) != VK_SUCCESS) {
      printf("Could not create bindless wind sampler!\n");
    }
    name = "_gpuWindForceSampler" + std::to_string(i);
    setDebugName(VK_OBJECT_TYPE_SAMPLER, (uint64_t)_gpuWindForceSampler[i], name.c_str());

    // Wind force image
    imageutil::createImage(
      256, 256,
      VK_FORMAT_R32_SFLOAT,
      VK_IMAGE_TILING_OPTIMAL,
      _vmaAllocator,
      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
      _gpuWindForceImage[i]);

    // Wind force image view
    _gpuWindForceView[i] = imageutil::createImageView(
      _device,
      _gpuWindForceImage[i]._image,
      VK_FORMAT_R32_SFLOAT,
      VK_IMAGE_ASPECT_COLOR_BIT);

    // Add the wind force image view to the vault so that the debug view can watch it
    auto res = new ImageRenderResource();
    res->_format = VK_FORMAT_R32_SFLOAT;
    res->_image = _gpuWindForceImage[i];
    res->_views.emplace_back(_gpuWindForceView[i]);
    _vault.addResource("WindForceImage", std::unique_ptr<IRenderResource>(res), true, static_cast<int>(i), true);

    // Add renderable buffer to vault, needed by particle system
    auto bufRes = new BufferRenderResource();
    bufRes->_buffer = _gpuRenderableBuffer[i];
    _vault.addResource("RenderableBuffer", std::unique_ptr<IRenderResource>(bufRes), true, static_cast<int>(i), true);

    // Add Vertex buffer to vault, needed to be able to write animated vertices
    {
      auto bufRes = new BufferRenderResource();
      bufRes->_buffer = _gigaVtxBuffer._buffer;
      _vault.addResource("GigaVtxBuffer", std::unique_ptr<IRenderResource>(bufRes), false, 0, true);
    }
  }

  return true;
}

bool VulkanRenderer::initImgui()
{
  //1: create descriptor pool for IMGUI
  // the size of the pool is very oversize, but it's copied from imgui demo itself.
  VkDescriptorPoolSize pool_sizes[] =
  {
    { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
    { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
    { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
    { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
    { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
    { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
  };

  VkDescriptorPoolCreateInfo pool_info = {};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  pool_info.maxSets = 1000;
  pool_info.poolSizeCount = (uint32_t)std::size(pool_sizes);
  pool_info.pPoolSizes = pool_sizes;

  if (vkCreateDescriptorPool(_device, &pool_info, nullptr, &_imguiDescriptorPool) != VK_SUCCESS) {
    printf("Could not create imgui descriptor pools!\n");
    return false;
  }

  // 2: initialize imgui library

  //this initializes the core structures of imgui
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  // Set font
  std::string fontPath = std::string(ASSET_PATH) + "/fonts/Roboto/Roboto-Regular.ttf";
  auto font = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 16.0f);
  assert(font != NULL && "Font not loaded!");
  io.Fonts->Build();

  ImGui::StyleColorsDark();

  if (!ImGui_ImplGlfw_InitForVulkan(_window, true)) {
    printf("Could not init glfw imgui\n");
    return false;
  }

  //this initializes imgui for Vulkan
  ImGui_ImplVulkan_InitInfo init_info = {};
  init_info.Instance = _instance;
  init_info.PhysicalDevice = _physicalDevice;
  init_info.Device = _device;
  init_info.Queue = _graphicsQ;
  init_info.DescriptorPool = _imguiDescriptorPool;
  init_info.MinImageCount = 3;
  init_info.ImageCount = 3;
  init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  init_info.UseDynamicRendering = true;
  init_info.ColorAttachmentFormat = getHDRFormat();

  if (!ImGui_ImplVulkan_Init(&init_info, nullptr)) {
    printf("Could not init impl vulkan for imgui!\n");
    return false;
  }

  auto cmdBuffer = beginSingleTimeCommands();
  ImGui_ImplVulkan_CreateFontsTexture(cmdBuffer);
  endSingleTimeCommands(cmdBuffer);

  // Destroy staging buffers (imgui internal)
  ImGui_ImplVulkan_DestroyFontUploadObjects();

  return true;
}

void VulkanRenderer::prepare()
{
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();

  ImGui::NewFrame();
  ImGuizmo::BeginFrame();
}

void VulkanRenderer::drawFrame(bool applyPostProcessing, bool debug)
{
  // At the start of the frame, we want to wait until the previous frame has finished, so that the command buffer and semaphores are available to use.
  vkWaitForFences(_device, 1, &_inFlightFences[_currentFrame], VK_TRUE, UINT64_MAX);

  // Check world pos requests
  checkWorldPosCallback();

  ImGui::Render();

  // Acquire an image from the swap chain
  uint32_t imageIndex;
  VkResult result = vkAcquireNextImageKHR(_device, _swapChain._swapChain, UINT64_MAX, _imageAvailableSemaphores[_currentFrame], VK_NULL_HANDLE, &imageIndex);

  _currentSwapChainIndex = imageIndex;

  // Reset staging buffer usage
  _currentStagingOffset = 0;

  // Window has resized for instance
  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapChain();
    return;
  }
  else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    printf("Failed to require swap chain image!\n");
    return;
  }

  // After waiting, we need to manually reset the fence to the unsignaled state with the vkResetFences call:
  vkResetFences(_device, 1, &_inFlightFences[_currentFrame]);

  // Recording command buffer
  vkResetCommandBuffer(_commandBuffers[_currentFrame], 0);

  // Execute deletion queue
  _delQ.execute();

  executeFrameGraph(_commandBuffers[_currentFrame], imageIndex);

  // Submit the command buffer
  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  // This semaphore is what we're waiting for TO BE SIGNALED before executing the command buffer
  VkSemaphore waitSemaphores[] = {_imageAvailableSemaphores[_currentFrame]};
  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &_commandBuffers[_currentFrame];

  // The signalSemaphoreCount and pSignalSemaphores parameters specify which semaphores to signal once the command buffer(s) have finished execution. In our case we're using the renderFinishedSemaphore for that purpose.
  VkSemaphore signalSemaphores[] = {_renderFinishedSemaphores[_currentFrame]};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  auto ret = vkQueueSubmit(_graphicsQ, 1, &submitInfo, _inFlightFences[_currentFrame]);
  if (ret != VK_SUCCESS) {
    printf("failed to submit draw command buffer (%d)!\n", ret);
    return;
  }

  // Presentation
  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  // The first two parameters specify which semaphores to wait on before presentation can happen, just like VkSubmitInfo.
  // Since we want to wait on the command buffer to finish execution, thus our triangle being drawn,
  // we take the semaphores which will be signalled and wait on them, thus we use signalSemaphores.
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;

  VkSwapchainKHR swapChains[] = {_swapChain._swapChain};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;
  presentInfo.pImageIndices = &imageIndex;

  result = vkQueuePresentKHR(_presentQ, &presentInfo);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _framebufferResized) {
    _framebufferResized = false;
    recreateSwapChain();
  }
  else if (result != VK_SUCCESS) {
    printf("failed to present swap chain image!\n");
  }
  
  computePerFrameQueries();

  // Advance frame
  _currentFrame = (_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanRenderer::createTexture(
  VkCommandBuffer cmdBuffer,
  VkFormat format,
  int width,
  int height,
  const std::vector<uint8_t>& data,
  VkSampler& samplerOut,
  AllocatedImage& imageOut,
  VkImageView& viewOut)
{
  uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

  AllocatedImage image;

  imageutil::createImage(
    width, height,
    format,
    VK_IMAGE_TILING_OPTIMAL,
    _vmaAllocator,
    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    image,
    mipLevels);

  auto view = imageutil::createImageView(
    _device,
    image._image,
    format,
    VK_IMAGE_ASPECT_COLOR_BIT,
    0,
    mipLevels);

  std::size_t dataSize = data.size();

  glm::uint8_t* mappedData;
  vmaMapMemory(_vmaAllocator, _gpuStagingBuffer[_currentFrame]._allocation, &(void*)mappedData);
  // Offset according to current staging buffer usage
  mappedData = mappedData + _currentStagingOffset;
  std::memcpy(mappedData, data.data(), data.size());
  vmaUnmapMemory(_vmaAllocator, _gpuStagingBuffer[_currentFrame]._allocation);

  // Transition image to dst optimal
  imageutil::transitionImageLayout(
    cmdBuffer,
    image._image,
    format,
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    0,
    mipLevels);

  VkExtent3D extent{};
  extent.depth = 1;
  extent.height = height;
  extent.width = width;

  VkOffset3D offset{};

  VkBufferImageCopy imCopy{};
  imCopy.bufferOffset = _currentStagingOffset;
  imCopy.imageExtent = extent;
  imCopy.imageOffset = offset;
  imCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imCopy.imageSubresource.layerCount = 1;

  vkCmdCopyBufferToImage(
    cmdBuffer,
    _gpuStagingBuffer[_currentFrame]._buffer,
    image._image,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    1,
    &imCopy);

  _currentStagingOffset += data.size();

  generateMipmaps(cmdBuffer, image._image, format, width, height, mipLevels);

  SamplerCreateParams params{};
  params.renderContext = this;

  imageOut = image;
  viewOut = view;
  samplerOut = createSampler(params);
}

void VulkanRenderer::generateMipmaps(VkCommandBuffer cmdBuffer, VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels)
{
  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.image = image;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.subresourceRange.levelCount = 1;

  int32_t mipWidth = texWidth;
  int32_t mipHeight = texHeight;

  for (uint32_t i = 1; i < mipLevels; i++) {
    barrier.subresourceRange.baseMipLevel = i - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    vkCmdPipelineBarrier(cmdBuffer,
      VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
      0, nullptr,
      0, nullptr,
      1, &barrier);

    VkImageBlit blit{};
    blit.srcOffsets[0] = { 0, 0, 0 };
    blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
    blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.srcSubresource.mipLevel = i - 1;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount = 1;
    blit.dstOffsets[0] = { 0, 0, 0 };
    blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
    blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.dstSubresource.mipLevel = i;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount = 1;

    vkCmdBlitImage(cmdBuffer,
      image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      1, &blit,
      VK_FILTER_LINEAR);

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmdBuffer,
      VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
      0, nullptr,
      0, nullptr,
      1, &barrier);

    if (mipWidth > 1) mipWidth /= 2;
    if (mipHeight > 1) mipHeight /= 2;
  }

  barrier.subresourceRange.baseMipLevel = mipLevels - 1;
  barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(cmdBuffer,
    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
    0, nullptr,
    0, nullptr,
    1, &barrier);
}

internal::BufferMemoryInterface::Handle VulkanRenderer::addTextureToBindless(VkImageLayout layout, VkImageView view, VkSampler sampler)
{
  VkWriteDescriptorSet descriptorWrite{};

  VkDescriptorImageInfo imageInfo{};
  imageInfo.imageLayout = layout;
  imageInfo.imageView = view;
  imageInfo.sampler = sampler;

  // Find free index using mem interface
  auto handle = _bindlessTextureMemIf.addData(1);

  if (!handle) {
    printf("Could not add texture to bindless, no more handles left!\n");
    return handle;
  }

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = _bindlessDescriptorSets[i];
    descriptorWrite.dstBinding = _bindlessTextureBinding;
    descriptorWrite.dstArrayElement = (uint32_t)handle._offset;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(_device, 1, &descriptorWrite, 0, nullptr);
  }

  return handle;
}

void VulkanRenderer::executeFrameGraph(VkCommandBuffer commandBuffer, int imageIndex)
{
  // Start cmd buffer
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    printf("failed to begin recording command buffer!\n");
    return;
  }

  resetPerFrameQueries(_commandBuffers[_currentFrame]);

  // The swapchain image has to go to transfer DST, which is the last thing the frame graph does.
  imageutil::transitionImageLayout(commandBuffer, _swapChain._swapChainImages[imageIndex], _swapChain._swapChainImageFormat,
    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  bool forceRenderableUpdate = false;

  if (_enableRayTracing && !_dynamicModelsToCopy.empty()) {
    copyDynamicModels(commandBuffer);
    for (std::size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      // Models changed because new meshes have been added
      _modelsChanged[i] = true;
      // renderables changed because there might be a new dynamicMeshId in the renderable
      forceRenderableUpdate = true;
    }
  }

  // Skeletons, get copies from animation thread. No-op if the list is empty
  prefillGPUSkeletonBuffer(commandBuffer, _animThread.getCurrentSkeletons());

  // Uploads
  {
    // Models, i.e. meshes that need to be uploaded to giga buffers
    if (_modelsChanged[_currentFrame]) {
      // Upload to the giga vertex and index buffers. This will be a no-op if there is nothing to upload.
      uploadPendingModels(commandBuffer);

      // Fill the mesh info buffers used on the GPU in the ray tracing pipelines
      prefillGPUMeshBuffer(commandBuffer);

      // force rend update because they need a model buffer update
      forceRenderableUpdate = true;

      _modelsChanged[_currentFrame] = false;
    }

    // Materials
    if (_materialsChanged[_currentFrame]) {
      uploadPendingMaterials(commandBuffer);

      // force rend update because the material indices might now be out of date
      forceRenderableUpdate = true;

      _materialsChanged[_currentFrame] = false;
    }

    // TODO: Textures
  }

  if (forceRenderableUpdate) {
    for (std::size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      // renderables changed because there might be a new dynamicMeshId in the renderable
      _renderablesChanged[i] = true;
    }
  }

  // Prefill renderable buffer
  if (_renderablesChanged[_currentFrame]) {
    prefillGPUModelBuffer(commandBuffer);
    prefillGPURendMatIdxBuffer(commandBuffer);
    prefillGPURenderableBuffer(commandBuffer);
    _renderablesChanged[_currentFrame] = false;
  }

  // Only do if lights have updated
  prefillGPULightBuffer(commandBuffer);

  // Update windforce texture
  updateWindForceImage(commandBuffer);

  // Bind giga buffers and bindless descriptors
  VkDeviceSize offsets[] = { 0 };
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, &_gigaVtxBuffer._buffer._buffer, offsets);
  vkCmdBindIndexBuffer(commandBuffer, _gigaIdxBuffer._buffer._buffer, 0, VK_INDEX_TYPE_UINT32);

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _bindlessPipelineLayout, 0, 1, &_bindlessDescriptorSets[_currentFrame], 0, nullptr);
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, _bindlessPipelineLayout, 0, 1, &_bindlessDescriptorSets[_currentFrame], 0, nullptr);
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, _bindlessPipelineLayout, 0, 1, &_bindlessDescriptorSets[_currentFrame], 0, nullptr);

  _fgb.executeGraph(commandBuffer, this);

  // Potentially add world pos requests
  worldPosCopy(_commandBuffers[_currentFrame]);

  // The swapchain image has to go to present, which is the last thing the frame graph does.
  imageutil::transitionImageLayout(commandBuffer, _swapChain._swapChainImages[imageIndex], _swapChain._swapChainImageFormat,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    printf("failed to record command buffer!\n");
  }
}

void VulkanRenderer::createParticles()
{
  std::vector<Vertex> vert;
  std::vector<std::uint32_t> inds;
  util::createUnitCube(&vert, &inds, true);

  asset::Mesh cubeMesh{};
  asset::Model cubeModel{};
  cubeMesh._vertices = std::move(vert);
  cubeMesh._indices = std::move(inds);

  cubeModel._meshes.emplace_back(std::move(cubeMesh));

  //auto modelId = registerModel(std::move(cubeModel), true);

  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_real_distribution<float> vel(-8.0f, 8.0f);
  std::uniform_real_distribution<float> velY(2.0f, 12.0f);
  std::uniform_real_distribution<float> scale(0.05f, .5f);
  std::uniform_real_distribution<float> delay(0.0f, 10.0f);

  for (int i = 0; i < 1000; ++i) {
    Particle particle{};
    particle._initialPosition = glm::vec3(-7.0f, 7.0f, -12.0f);
    particle._initialVelocity = glm::vec3(vel(rng), velY(rng), vel(rng));
    particle._lifetime = 5.0f;
    particle._scale = scale(rng);
    particle._spawnDelay = delay(rng);

    /*particle._renderableId = registerRenderable(
      modelId,
      glm::translate(glm::mat4(1.0f), particle._initialPosition) * glm::scale(glm::mat4(1.0f), glm::vec3(particle._scale)),
      glm::vec3(0.0f),
      1.5f);

    setRenderableVisible(particle._renderableId, false);*/

    _particles.emplace_back(std::move(particle));
  }
}

void VulkanRenderer::notifyFramebufferResized()
{
  _framebufferResized = true;
}

void VulkanRenderer::requestWorldPosition(glm::ivec2 viewportPos, WorldPosCallback callback)
{
  // If we already have one queued, don't queue another one...?
  if (_worldPosRequest._callback) return;

  // If the request is unreasonable, don't do it
  if (viewportPos.x < 0 || viewportPos.x >= _swapChain._swapChainExtent.width ||
    viewportPos.y < 0 || viewportPos.y >= _swapChain._swapChainExtent.height) {
    return;
  }

  _worldPosRequest._callback = callback;
  _worldPosRequest._viewportPos = viewportPos;
}

VkDevice& VulkanRenderer::device()
{
  return _device;
}

VkDescriptorPool& VulkanRenderer::descriptorPool()
{
  return _descriptorPool;
}

VmaAllocator VulkanRenderer::vmaAllocator()
{
  return _vmaAllocator;
}

VkPipelineLayout& VulkanRenderer::bindlessPipelineLayout()
{
  return _bindlessPipelineLayout;
}

VkDescriptorSetLayout& VulkanRenderer::bindlessDescriptorSetLayout()
{
  return _bindlessDescSetLayout;
}

VkExtent2D VulkanRenderer::swapChainExtent()
{
  return _swapChain._swapChainExtent;
}

std::size_t VulkanRenderer::getGigaBufferSizeMB()
{
  return GIGA_MESH_BUFFER_SIZE_MB;
}

VkDeviceAddress VulkanRenderer::getGigaVtxBufferAddr()
{
  return _gigaVtxAddr;
}

VkDeviceAddress VulkanRenderer::getGigaIdxBufferAddr()
{
  return _gigaIdxAddr;
}

void VulkanRenderer::drawGigaBufferIndirect(VkCommandBuffer* commandBuffer, VkBuffer drawCalls, uint32_t drawCount)
{
  vkCmdDrawIndexedIndirect(*commandBuffer, drawCalls, 0, drawCount, sizeof(gpu::GPUDrawCallCmd));
}

void VulkanRenderer::drawNonIndexIndirect(VkCommandBuffer* cmdBuffer, VkBuffer drawCalls, uint32_t drawCount, uint32_t stride)
{
  vkCmdDrawIndirect(*cmdBuffer, drawCalls, 0, drawCount, stride);
}

void VulkanRenderer::drawMeshId(VkCommandBuffer* cmdBuffer, util::Uuid meshId, uint32_t vertCount, uint32_t instanceCount)
{
  auto idx = _meshIdMap[meshId];
  auto& mesh = _currentMeshes[idx];
  vkCmdDraw(*cmdBuffer, vertCount, instanceCount, (uint32_t)mesh._vertexOffset, 0);
}

VkImage& VulkanRenderer::getCurrentSwapImage()
{
  return _swapChain._swapChainImages[_currentSwapChainIndex];
}

int VulkanRenderer::getCurrentMultiBufferIdx()
{
  return _currentFrame;
}

int VulkanRenderer::getMultiBufferSize()
{
  return MAX_FRAMES_IN_FLIGHT;
}

size_t VulkanRenderer::getMaxNumMeshes()
{
  return MAX_NUM_MESHES;
}

size_t VulkanRenderer::getMaxNumRenderables()
{
  return MAX_NUM_RENDERABLES;
}

size_t VulkanRenderer::getMaxBindlessResources()
{
  return MAX_BINDLESS_RESOURCES;
}

std::vector<internal::InternalMesh>& VulkanRenderer::getCurrentMeshes()
{
  return _currentMeshes;
}

std::vector<internal::InternalRenderable>& VulkanRenderer::getCurrentRenderables()
{
  return _currentRenderables;
}

bool VulkanRenderer::getRenderableById(util::Uuid id, internal::InternalRenderable** out)
{
  if (_renderableIdMap.find(id) == _renderableIdMap.end()) {
    printf("Warning! Cannot get renderable with id %s, doesn't exist!\n", id.str().c_str());
    return false;
  }

  *out = &_currentRenderables[_renderableIdMap[id]];

  return true;
}

bool VulkanRenderer::getMeshById(util::Uuid id, internal::InternalMesh** out)
{
  if (_meshIdMap.find(id) == _meshIdMap.end()) {
    printf("Warning! Cannot get mesh with id %s, doesn't exist!\n", id.str().c_str());
    return false;
  }

  *out = &_currentMeshes[_meshIdMap[id]];

  return true;
}

std::unordered_map<util::Uuid, std::size_t>& VulkanRenderer::getCurrentMeshUsage()
{
  return _currentMeshUsage;
}

size_t VulkanRenderer::getCurrentNumRenderables()
{
  return _currentRenderables.size();
}

std::unordered_map<util::Uuid, std::vector<AccelerationStructure>>& VulkanRenderer::getDynamicBlases()
{
  return _dynamicBlases;
}

gpu::GPUCullPushConstants VulkanRenderer::getCullParams()
{
  gpu::GPUCullPushConstants cullPushConstants{};
  cullPushConstants._drawCount = (uint32_t)_currentRenderables.size();

  cullPushConstants._frustumPlanes[0] = _latestCamera.getFrustum().getPlane(Frustum::Left);
  cullPushConstants._frustumPlanes[1] = _latestCamera.getFrustum().getPlane(Frustum::Right);
  cullPushConstants._frustumPlanes[2] = _latestCamera.getFrustum().getPlane(Frustum::Top);
  cullPushConstants._frustumPlanes[3] = _latestCamera.getFrustum().getPlane(Frustum::Bottom);
  cullPushConstants._view = _latestCamera.getCamMatrix();
  cullPushConstants._farDist = _latestCamera.getFar();
  cullPushConstants._nearDist = _latestCamera.getNear();
  //auto wind = glm::normalize(_currentWindMap.windDir);
  cullPushConstants._windDirX = 0.0f;// wind.x;
  cullPushConstants._windDirY = 0.0f;// wind.y;

  return cullPushConstants;
}

RenderOptions& VulkanRenderer::getRenderOptions()
{
  return _renderOptions;
}

RenderDebugOptions& VulkanRenderer::getDebugOptions()
{
  return _debugOptions;
}

void VulkanRenderer::setDebugName(VkObjectType objectType, uint64_t objectHandle, const char* name)
{
  VkDebugUtilsObjectNameInfoEXT nameInfo{};
  nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
  nameInfo.objectType = objectType;
  nameInfo.objectHandle = objectHandle;
  nameInfo.pObjectName = name;

  SetDebugUtilsObjectNameEXT(_instance, _device, &nameInfo);
}

glm::vec2 VulkanRenderer::getWindDir()
{
  //return _currentWindMap.windDir;
  return {0.0f, 0.0f};
}

}