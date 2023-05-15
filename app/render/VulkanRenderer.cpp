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

#include "../util/Utils.h"
#include "../util/GraphicsUtils.h"
#include "../LodePng/lodepng.h"
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_glfw.h"
#include "../imgui/imgui_impl_vulkan.h"

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
    printf("\nvalidation layer: %s\n", pCallbackData->pMessage);
  }

  return VK_FALSE;
}

VKAPI_ATTR void VKAPI_CALL CmdTraceRaysKHR(
  VkDevice device,
  VkCommandBuffer commandBuffer,
  const VkStridedDeviceAddressRegionKHR * pRaygenShaderBindingTable,
  const VkStridedDeviceAddressRegionKHR * pMissShaderBindingTable,
  const VkStridedDeviceAddressRegionKHR * pHitShaderBindingTable,
   const VkStridedDeviceAddressRegionKHR * pCallableShaderBindingTable,
   uint32_t width,
   uint32_t height,
   uint32_t depth)
{
  auto func = (PFN_vkCmdTraceRaysKHR)vkGetDeviceProcAddr(device, "vkCmdTraceRaysKHR");
  if (func != nullptr) {
    return func(commandBuffer, pRaygenShaderBindingTable, pMissShaderBindingTable, pHitShaderBindingTable, pCallableShaderBindingTable, width, height, depth);
  }
  else {
    printf("vkCmdTraceRaysKHR not available!\n");
  }
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

VKAPI_ATTR void VKAPI_CALL DestroyAccelerationStructureKHR(
  VkDevice                                    device,
  VkAccelerationStructureKHR                  accelerationStructure,
  const VkAllocationCallbacks* pAllocator)
{
  auto func = (PFN_vkDestroyAccelerationStructureKHR)vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureKHR");
  if (func != nullptr) {
    return func(device, accelerationStructure, pAllocator);
  }
  else {
    printf("vkDestroyAccelerationStructureKHR not available!\n");
  }
}

VKAPI_ATTR void VKAPI_CALL CmdBuildAccelerationStructuresKHR(
  VkDevice                                    device,
  VkCommandBuffer                             commandBuffer,
  uint32_t                                    infoCount,
  const VkAccelerationStructureBuildGeometryInfoKHR* pInfos,
  const VkAccelerationStructureBuildRangeInfoKHR* const* ppBuildRangeInfos)
{
  auto func = (PFN_vkCmdBuildAccelerationStructuresKHR)vkGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructuresKHR");
  if (func != nullptr) {
    return func(commandBuffer, infoCount, pInfos, ppBuildRangeInfos);
  }
  else {
    printf("vkCmdBuildAccelerationStructuresKHR not available!\n");
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
  , _latestCamera(initialCamera)
  , _fgb(&_vault)
  , _window(window)
  , _enableValidationLayers(true)
{
  imageutil::init();
}

VulkanRenderer::~VulkanRenderer()
{
  // Let logical device finish operations first
  vkDeviceWaitIdle(_device);

  vmaDestroyBuffer(_vmaAllocator, _gigaMeshBuffer._buffer._buffer, _gigaMeshBuffer._buffer._allocation);

  for (auto& mesh : _currentMeshes) {
    if (mesh._metallic._bindlessIndex != -1) vkDestroySampler(_device, mesh._metallic._sampler, nullptr);
    if (mesh._roughness._bindlessIndex != -1) vkDestroySampler(_device, mesh._roughness._sampler, nullptr);
    if (mesh._normal._bindlessIndex != -1) vkDestroySampler(_device, mesh._normal._sampler, nullptr);
    if (mesh._albedo._bindlessIndex != -1) vkDestroySampler(_device, mesh._albedo._sampler, nullptr);
    if (mesh._metallicRoughness._bindlessIndex != -1) vkDestroySampler(_device, mesh._metallicRoughness._sampler, nullptr);

    if (mesh._metallic._bindlessIndex != -1) vmaDestroyImage(_vmaAllocator, mesh._metallic._image._image, mesh._metallic._image._allocation);
    if (mesh._roughness._bindlessIndex != -1) vmaDestroyImage(_vmaAllocator, mesh._roughness._image._image, mesh._roughness._image._allocation);
    if (mesh._normal._bindlessIndex != -1) vmaDestroyImage(_vmaAllocator, mesh._normal._image._image, mesh._normal._image._allocation);
    if (mesh._albedo._bindlessIndex != -1) vmaDestroyImage(_vmaAllocator, mesh._albedo._image._image, mesh._albedo._image._allocation);
    if (mesh._metallicRoughness._bindlessIndex != -1) vmaDestroyImage(_vmaAllocator, mesh._metallicRoughness._image._image, mesh._metallicRoughness._image._allocation);

    if (mesh._metallic._bindlessIndex != -1) vkDestroyImageView(_device, mesh._metallic._view, nullptr);
    if (mesh._roughness._bindlessIndex != -1) vkDestroyImageView(_device, mesh._roughness._view, nullptr);
    if (mesh._normal._bindlessIndex != -1) vkDestroyImageView(_device, mesh._normal._view, nullptr);
    if (mesh._albedo._bindlessIndex != -1) vkDestroyImageView(_device, mesh._albedo._view, nullptr);
    if (mesh._metallicRoughness._bindlessIndex != -1) vkDestroyImageView(_device, mesh._metallicRoughness._view, nullptr);
  }

  for (auto& blas : _blases) {
    vmaDestroyBuffer(_vmaAllocator, blas.second._buffer._buffer, blas.second._buffer._allocation);
    DestroyAccelerationStructureKHR(_device, blas.second._as, nullptr);
  }

  vmaDestroyBuffer(_vmaAllocator, _tlas._buffer._buffer, _tlas._buffer._allocation);
  DestroyAccelerationStructureKHR(_device, _tlas._as, nullptr);

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

  for (const auto& renderable: _currentRenderables) {
    cleanupRenderable(renderable);
  }

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    vkDestroySemaphore(_device, _imageAvailableSemaphores[i], nullptr);
    vkDestroySemaphore(_device, _renderFinishedSemaphores[i], nullptr);
    vkDestroyFence(_device, _inFlightFences[i], nullptr);
    vmaDestroyBuffer(_vmaAllocator, _gpuRenderableBuffer[i]._buffer, _gpuRenderableBuffer[i]._allocation);
    vmaDestroyBuffer(_vmaAllocator, _gpuMeshMaterialInfoBuffer[i]._buffer, _gpuMeshMaterialInfoBuffer[i]._allocation);
    vmaDestroyBuffer(_vmaAllocator, _gpuStagingBuffer[i]._buffer, _gpuStagingBuffer[i]._allocation);
    vmaDestroyBuffer(_vmaAllocator, _gpuSceneDataBuffer[i]._buffer, _gpuSceneDataBuffer[i]._allocation);
    vmaDestroyBuffer(_vmaAllocator, _gpuLightBuffer[i]._buffer, _gpuLightBuffer[i]._allocation);
    vmaDestroyBuffer(_vmaAllocator, _gpuViewClusterBuffer[i]._buffer, _gpuViewClusterBuffer[i]._allocation);

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

  printf("Init view clusters...");
  res &= initViewClusters();
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

  printf("Init frame graph builder...");
  res &= initFrameGraphBuilder();
  if (!res) return false;
  printf("Done!\n");

  printf("Init imgui...");
  res &= initImgui();
  if (!res) return false;
  printf("Done!\n");  

  _renderablesChanged.resize(MAX_FRAMES_IN_FLIGHT);
  _meshesChanged.resize(MAX_FRAMES_IN_FLIGHT);

  for (auto& val : _renderablesChanged) {
    val = true;
  }
  for (auto& val : _meshesChanged) {
    val = true;
  }

  /*std::vector<Vertex> vert;
  std::vector<std::uint32_t> inds;
  graphicsutil::createUnitCube(&vert, &inds, false);

  _debugCubeMesh = registerMesh(vert, inds);*/

  // TESTING
  std::vector<Vertex> vert;
  std::vector<std::uint32_t> inds;
  graphicsutil::createSphere(100.0f, &vert, &inds, true);

  Model sphereModel{};
  Mesh sphereMesh{};
  sphereMesh._indices = std::move(inds);
  sphereMesh._vertices = std::move(vert);
  sphereModel._meshes.emplace_back(std::move(sphereMesh));

  _debugSphereModelId = registerModel(std::move(sphereModel));
  _debugSphereMeshId = getMeshIds(_debugSphereModelId)[0];

  /*_debugSphereRenderable = registerRenderable(
    _debugSphereModelId,
    glm::mat4(1.0f),
    glm::vec3(0.0f),
    5000.0f);*/

  return res;
}

MeshId VulkanRenderer::registerMesh(Mesh& mesh, bool buildBlas)
{
  // Check if we need to pad with 0's before the vertices
  std::vector<std::uint8_t> verticesCopy;
  verticesCopy.resize(mesh._vertices.size() * sizeof(Vertex));
  memcpy(verticesCopy.data(), mesh._vertices.data(), verticesCopy.size());

  std::size_t numZeroes = sizeof(Vertex) - (_gigaMeshBuffer._freeSpacePointer % sizeof(Vertex));

  if (numZeroes != 0) {
    std::vector<std::uint8_t> zeroVec(numZeroes, 0);
    verticesCopy.insert(verticesCopy.begin(), zeroVec.begin(), zeroVec.end());
  }

  // Create a staging buffer on CPU side first
  AllocatedBuffer stagingBuffer;
  std::size_t vertSize = verticesCopy.size();
  std::size_t indSize = mesh._indices.size() * sizeof(std::uint32_t);
  std::size_t dataSize = vertSize + indSize;

  bufferutil::createBuffer(_vmaAllocator, dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, stagingBuffer);

  void* mappedData;
  vmaMapMemory(_vmaAllocator, stagingBuffer._allocation, &mappedData);
  memcpy(mappedData, verticesCopy.data(), vertSize);
  if (indSize > 0) {
    memcpy((uint8_t*)mappedData + vertSize, mesh._indices.data(), indSize);
  }
  vmaUnmapMemory(_vmaAllocator, stagingBuffer._allocation);

  // Find where to copy data in the fat buffer
  auto cmdBuffer = beginSingleTimeCommands();

  VkBufferCopy copyRegion{};
  copyRegion.dstOffset = _gigaMeshBuffer._freeSpacePointer;
  copyRegion.size = dataSize;
  vkCmdCopyBuffer(cmdBuffer, stagingBuffer._buffer, _gigaMeshBuffer._buffer._buffer, 1, &copyRegion);

  endSingleTimeCommands(cmdBuffer);

  mesh._metallic._bindlessIndex = -1;
  mesh._roughness._bindlessIndex = -1;
  mesh._normal._bindlessIndex = -1;
  mesh._albedo._bindlessIndex = -1;
  mesh._metallicRoughness._bindlessIndex = -1;

  auto loadTexturesFunc = [&](PbrMaterialData& pbrData) {
    std::vector<uint8_t>& data = pbrData._data;
    bool isColor = true;
    int width = pbrData._width;
    int height = pbrData._height;

    if (data.empty()) {
      auto loaded = imageutil::loadTex(pbrData._texPath);
      data = std::move(loaded.data);
      width = loaded.width;
      height = loaded.height;
      isColor = loaded.isColor;
    }
    
    VkFormat format = isColor ? VK_FORMAT_R8G8B8A8_UNORM : VK_FORMAT_R8_UNORM;

    createTexture(format, width, height, data, pbrData._sampler, pbrData._image, pbrData._view);

    pbrData._bindlessIndex = addTextureToBindless(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, pbrData._view, pbrData._sampler);

    // No need to keep on CPU
    data = std::vector<uint8_t>();
  };

  // Do textures
  if (!mesh._metallic._texPath.empty()) {
    loadTexturesFunc(mesh._metallic);
  }
  if (!mesh._roughness._texPath.empty()) {
    loadTexturesFunc(mesh._roughness);
  }
  if (!mesh._normal._texPath.empty()) {
    loadTexturesFunc(mesh._normal);
  }
  if (!mesh._albedo._texPath.empty()) {
    loadTexturesFunc(mesh._albedo);
  }
  if (!mesh._metallicRoughness._texPath.empty()) {
    loadTexturesFunc(mesh._metallicRoughness);
  }

  mesh._id = _nextMeshId++;
  mesh._numVertices = verticesCopy.size();
  mesh._numIndices = mesh._indices.size();
  mesh._vertexOffset = (_gigaMeshBuffer._freeSpacePointer + numZeroes) / sizeof(Vertex);
  mesh._indexOffset = (_gigaMeshBuffer._freeSpacePointer + vertSize) / sizeof(uint32_t);

  auto idCopy = mesh._id;

  _currentMeshes.emplace_back(std::move(mesh));

  // Advance free space pointer
  _gigaMeshBuffer._freeSpacePointer += dataSize;

  vmaDestroyBuffer(_vmaAllocator, stagingBuffer._buffer, stagingBuffer._allocation);

  // Build BLAS
  if (buildBlas) {
    registerBottomLevelAS(idCopy);
  }

  return _currentMeshes.size() - 1;
}

ModelId VulkanRenderer::registerModel(Model&& model)
{
  // Go through each mesh of the model and add it to our meshes

  for (auto& mesh : model._meshes) {
    registerMesh(mesh);
  }

  _models.emplace_back(std::move(model));

  //printf("Mesh registered, id: %u\n", idOut);
  return _models.size() - 1;
}

bool VulkanRenderer::modelIdExists(ModelId id)
{
  return id < (int64_t)_models.size();
}

RenderableId VulkanRenderer::registerRenderable(
  ModelId modelId,
  const glm::mat4& transform,
  const glm::vec3& sphereBoundCenter,
  float sphereBoundRadius,
  bool debugDraw)
{
  if (_currentRenderables.size() == MAX_NUM_RENDERABLES) {
    printf("Too many renderables!\n");
    return 0;
  }

  if (!modelIdExists(modelId)) {
    printf("Model id %u does not exist!\n", modelId);
    return 0;
  }

  Model& model = _models[modelId];

  if (model._meshes.empty()) {
    printf("Model Id %u contains no meshes!\n", modelId);
    return 0;
  }

  Renderable renderable{};

  auto id = _nextRenderableId++;

  renderable._id = id;
  renderable._firstMeshId = model._meshes[0]._id;
  renderable._numMeshes = model._meshes.size();
  renderable._tint = glm::vec3(1.0f);
  renderable._transform = transform;
  renderable._boundingSphereCenter = sphereBoundCenter;
  renderable._boundingSphereRadius = sphereBoundRadius;

  if (debugDraw) {
    registerDebugRenderable(transform, sphereBoundCenter, sphereBoundRadius);
  }

  _currentRenderables.emplace_back(std::move(renderable));

  for (auto& val : _renderablesChanged) {
    val = true;
  }

  return id;
}

void VulkanRenderer::registerDebugRenderable(const glm::mat4& transform, const glm::vec3& center, float radius)
{
  if (_currentRenderables.size() == MAX_NUM_RENDERABLES) {
    printf("Too many renderables!\n");
    return;
  }

  Renderable renderable{};

  auto id = _nextRenderableId++;

  renderable._id = id;
  renderable._firstMeshId = _debugSphereMeshId;
  renderable._numMeshes = 1;
  renderable._tint = graphicsutil::randomColor();
  renderable._transform = transform;
  renderable._boundingSphereCenter = center;
  renderable._boundingSphereRadius = 50000.0f;

  _currentRenderables.emplace_back(std::move(renderable));

  for (auto& val : _renderablesChanged) {
    val = true;
  }
}

void VulkanRenderer::unregisterRenderable(RenderableId id)
{
  for (auto it = _currentRenderables.begin(); it != _currentRenderables.end(); ++it) {
    if (it->_id == id) {
      cleanupRenderable(*it);
      _currentRenderables.erase(it);
      break;
    }
  }
}

void VulkanRenderer::setRenderableVisible(RenderableId id, bool visible)
{
  for (auto& rend : _currentRenderables) {
    if (rend._id == id) {
      rend._visible = visible;
      return;
    }
  }
}

void VulkanRenderer::cleanupRenderable(const Renderable& renderable)
{
  /*vmaDestroyBuffer(_vmaAllocator, renderable._vertexBuffer._buffer, renderable._vertexBuffer._allocation);
  vmaDestroyBuffer(_vmaAllocator, renderable._indexBuffer._buffer, renderable._indexBuffer._allocation);
  if (renderable._numInstances > 0) {
    vmaDestroyBuffer(_vmaAllocator, renderable._instanceData._buffer, renderable._instanceData._allocation);
  }*/
}

void VulkanRenderer::setRenderableTint(RenderableId id, const glm::vec3& tint)
{
  for (auto& rend : _currentRenderables) {
    if (rend._id == id) {
      rend._tint = tint;
      return;
    }
  }
}

void VulkanRenderer::update(
  const Camera& camera,
  const Camera& shadowCamera,
  const glm::vec4& lightDir,
  double delta,
  double time,
  bool lockCulling,
  RenderOptions renderOptions,
  RenderDebugOptions debugOptions,
  logic::WindMap windMap)
{
  // TODO: We can't write to this frames UBO's until the GPU is finished with it.
  // If we run unlimited FPS we are quite likely to end up doing just that.
  // The ubo memory write _will_ be visible by the next queueSubmit, so that isn't the issue.
  // but we might be writing into memory that is currently accessed by the GPU.
  vkWaitForFences(_device, 1, &_inFlightFences[_currentFrame], VK_TRUE, UINT64_MAX);

  // TODO: Fix
  if (!_topLevelBuilt) {
    buildTopLevelAS();
    writeTLASDescriptor();
    _topLevelBuilt = true;
  }

  _renderOptions = renderOptions;
  _debugOptions = debugOptions;
  _currentWindMap = windMap;

  // Update bindless UBO
  auto shadowProj = shadowCamera.getProjection();
  auto proj = camera.getProjection();

  shadowProj[1][1] *= -1;
  proj[1][1] *= -1;

  auto invView = glm::inverse(camera.getCamMatrix());
  auto invProj = glm::inverse(proj);

  gpu::GPUSceneData ubo{};
  ubo.cameraPos = glm::vec4(camera.getPosition(), 1.0);
  ubo.invView = invView;
  ubo.invViewProj = invView * invProj;
  ubo.invProj = invProj;
  ubo.directionalShadowMatrixProj = shadowProj;
  ubo.directionalShadowMatrixView = shadowCamera.getCamMatrix();
  ubo.lightDir = lightDir;
  ubo.proj = proj;
  ubo.view = camera.getCamMatrix();
  ubo.viewVector = glm::vec4(camera.getForward(), 1.0);
  ubo.time = time;
  ubo.screenHeight = swapChainExtent().height;
  ubo.screenWidth = swapChainExtent().width;
  ubo.ssaoEnabled = _renderOptions.ssao;
  ubo.fxaaEnabled = _renderOptions.fxaa;
  ubo.directionalShadowsEnabled = _renderOptions.directionalShadows;
  ubo.rtShadowsEnabled = _renderOptions.raytracedShadows;

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

bool VulkanRenderer::meshIdExists(MeshId meshId) const
{
  return meshId < (int64_t)_currentMeshes.size();
}

void VulkanRenderer::registerBottomLevelAS(MeshId meshId)
{
  if (!meshIdExists(meshId)) {
    printf("Can't create BLAS for mesh id %u, it does not exist!\n", meshId);
    return;
  }

  if (_blases.count(meshId) > 0) {
    printf("Cannot create BLAS for mesh id %u, it already exists!\n", meshId);
    return;
  }

  //printf("Building BLAS for mesh id %u\n", meshId);

  Mesh& mesh = _currentMeshes[meshId];

  // Retrieve gigabuffer device address
  VkBufferDeviceAddressInfo addrInfo{};
  addrInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
  addrInfo.buffer = _gigaMeshBuffer._buffer._buffer;

  VkDeviceAddress gigaAddress = vkGetBufferDeviceAddress(_device, &addrInfo);

  // According to spec it is ok to get address of offset data using a simple uint64_t offset
  // Also note, the vertexoffset and indexoffset are not _bytes_ but "numbers"
  VkDeviceAddress vertexAddress = gigaAddress + mesh._vertexOffset * sizeof(Vertex);
  VkDeviceAddress indexAddress = gigaAddress + mesh._indexOffset * sizeof(uint32_t);

  // Now we create a structure for the triangle and index data for this mesh
  VkAccelerationStructureGeometryTrianglesDataKHR triangles{};
  triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
  triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
  triangles.vertexData.deviceAddress = vertexAddress;
  triangles.vertexStride = sizeof(Vertex);
  triangles.indexType = VK_INDEX_TYPE_UINT32;
  triangles.indexData.deviceAddress = indexAddress;
  triangles.maxVertex = mesh._numVertices / sizeof(Vertex);
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
  buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
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
    // The device
    _device,
    // Build on device instead of host.
    VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
    // Pointer to build info
    &buildInfo,
    // Array of number of primitives per geometry
    &rangeInfo.primitiveCount,
    // Output pointer to store sizes
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
    return;
  }
  
  buildInfo.dstAccelerationStructure = blas._as;

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
  auto cmdBuffer = beginSingleTimeCommands();

  VkAccelerationStructureBuildRangeInfoKHR* pRangeInfo = &rangeInfo;
  CmdBuildAccelerationStructuresKHR(
    _device,
    cmdBuffer, // The command buffer to record the command
    1, // Number of acceleration structures to build
    &buildInfo, // Array of ...BuildGeometryInfoKHR objects
    &pRangeInfo); // Array of ...RangeInfoKHR objects

  endSingleTimeCommands(cmdBuffer);

  vmaDestroyBuffer(_vmaAllocator, scratchBuffer._buffer, scratchBuffer._allocation);

  _blases[meshId] = std::move(blas);
}

void VulkanRenderer::buildTopLevelAS()
{
  std::vector<VkAccelerationStructureInstanceKHR> instances;

  for (auto& rend : _currentRenderables) {
    for (uint32_t i = 0; i < rend._numMeshes; ++i) {
      auto& blas = _blases[rend._firstMeshId + i];

      // Get address of BLAS
      VkAccelerationStructureDeviceAddressInfoKHR addressInfo{};
      addressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
      addressInfo.accelerationStructure = blas._as;
      VkDeviceAddress blasAddress = GetAccelerationStructureDeviceAddressKHR(_device, &addressInfo);

      // Create the instance
      VkAccelerationStructureInstanceKHR instance{};
      VkTransformMatrixKHR matrix{
        rend._transform[0][0], rend._transform[1][0], rend._transform[2][0], rend._transform[3][0],
        rend._transform[0][1], rend._transform[1][1], rend._transform[2][1], rend._transform[3][1],
        rend._transform[0][2], rend._transform[1][2], rend._transform[2][2], rend._transform[3][2]
      };
      instance.transform = matrix;
      instance.mask = 0xFF;
      instance.instanceShaderBindingTableRecordOffset = 0;
      instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
      instance.accelerationStructureReference = blasAddress;

      instances.emplace_back(instance);
    }
  }

  std::size_t dataSize = sizeof(VkAccelerationStructureInstanceKHR) * instances.size();

  // Create a buffer for all instances
  AllocatedBuffer instanceBuffer{};
  bufferutil::createBuffer(
    _vmaAllocator,
    dataSize,
    VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
    0,
    instanceBuffer);

  // Create a staging buffer to upload the data to the buffer
  AllocatedBuffer stagingBuffer{};
  bufferutil::createBuffer(
    _vmaAllocator,
    dataSize,
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
    stagingBuffer);

  uint8_t* data;
  vmaMapMemory(_vmaAllocator, stagingBuffer._allocation, (void**)&data);
  memcpy(data, instances.data(), dataSize);
  vmaUnmapMemory(_vmaAllocator, stagingBuffer._allocation);

  auto cmdBuffer = beginSingleTimeCommands();

  VkBufferCopy copy{};
  copy.size = dataSize;
  vkCmdCopyBuffer(
    cmdBuffer,
    stagingBuffer._buffer,
    instanceBuffer._buffer,
    1, &copy);

  // Add a memory barrier to ensure that createBuffer's upload command
  // finishes before starting the TLAS build.
  VkMemoryBarrier barrier{ VK_STRUCTURE_TYPE_MEMORY_BARRIER };
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
  vkCmdPipelineBarrier(cmdBuffer,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
    0,
    1, &barrier,
    0, nullptr,
    0, nullptr);

  VkAccelerationStructureBuildRangeInfoKHR rangeInfo{};
  rangeInfo.primitiveOffset = 0;
  rangeInfo.primitiveCount = (uint32_t)instances.size(); // Number of instances
  rangeInfo.firstVertex = 0;
  rangeInfo.transformOffset = 0;

  VkBufferDeviceAddressInfo instanceAddrInfo{};
  instanceAddrInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
  instanceAddrInfo.buffer = instanceBuffer._buffer;

  VkDeviceAddress instanceAddr = vkGetBufferDeviceAddress(_device, &instanceAddrInfo);

  VkAccelerationStructureGeometryInstancesDataKHR instancesVk{};
  instancesVk.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
  instancesVk.arrayOfPointers = VK_FALSE;
  instancesVk.data.deviceAddress = instanceAddr;

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
  buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
  buildInfo.geometryCount = 1;
  buildInfo.pGeometries = &geometry;
  buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
  buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
  buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;
  
  // Query the worst-case AS size and scratch space size based on
  // the number of instances.
  VkAccelerationStructureBuildSizesInfoKHR sizeInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
  GetAccelerationStructureBuildSizesKHR(
    _device,
    VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
    &buildInfo,
    &rangeInfo.primitiveCount,
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

  buildInfo.dstAccelerationStructure = _tlas._as;

  // Allocate the scratch buffer holding temporary build data.
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

  // Create a one-element array of pointers to range info objects.
  VkAccelerationStructureBuildRangeInfoKHR * pRangeInfo = &rangeInfo;

  // Build the TLAS.
  CmdBuildAccelerationStructuresKHR(
    _device,
    cmdBuffer,
    1, &buildInfo,
    & pRangeInfo);

  endSingleTimeCommands(cmdBuffer);

  vmaDestroyBuffer(_vmaAllocator, scratchBuffer._buffer, scratchBuffer._allocation);
  vmaDestroyBuffer(_vmaAllocator, stagingBuffer._buffer, stagingBuffer._allocation);
  vmaDestroyBuffer(_vmaAllocator, instanceBuffer._buffer, instanceBuffer._allocation);
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

  std::set<std::string> requiredExtensions(_deviceExtensions.begin(), _deviceExtensions.end());

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

  VkPhysicalDeviceFeatures2 deviceFeatures2{};
  deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
  deviceFeatures2.pNext = &indexingFeatures;

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

  VkDeviceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.pQueueCreateInfos = queueCreateInfos.data();
  createInfo.queueCreateInfoCount = static_cast<std::uint32_t>(queueCreateInfos.size());

  createInfo.pEnabledFeatures = &deviceFeatures;

  createInfo.enabledExtensionCount = static_cast<std::uint32_t>(_deviceExtensions.size());
  createInfo.ppEnabledExtensionNames = _deviceExtensions.data();

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
  vulkan12Features.bufferDeviceAddress = VK_TRUE;
  vulkan12Features.hostQueryReset = VK_TRUE;

  dynamicRenderingFeature.pNext = &vulkan12Features;

  // Ray tracing
  VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtPipeFeatures{};
  rtPipeFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
  rtPipeFeatures.rayTracingPipeline = VK_TRUE;
  vulkan12Features.pNext = &rtPipeFeatures;

  VkPhysicalDeviceAccelerationStructureFeaturesKHR asFeatures{};
  asFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
  asFeatures.accelerationStructure = VK_TRUE;
  rtPipeFeatures.pNext = &asFeatures;

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
    res->_view = _gpuWindForceView[i];
    _vault.addResource("WindForceImage", std::unique_ptr<IRenderResource>(res), true, i, true);
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

  /*VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  VkFence fence;
  vkCreateFence(_device, &fenceInfo, nullptr, &fence);
  // Submit to the queue
  auto submitRes = vkQueueSubmit(_graphicsQ, 1, &submitInfo, fence);
  // Wait for the fence to signal that command buffer has finished executing
  vkWaitForFences(_device, 1, &fence, VK_TRUE, UINT64_MAX);
  vkDestroyFence(_device, fence, nullptr);*/

  vkQueueSubmit(_graphicsQ, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(_graphicsQ);

  vkFreeCommandBuffers(_device, _commandPool, 1, &commandBuffer);
}

VkPhysicalDeviceRayTracingPipelinePropertiesKHR VulkanRenderer::getRtPipeProps()
{
  return _rtPipeProps;
}

int VulkanRenderer::findTimerIndex(const std::string& timer)
{
  for (std::size_t i = 0; i < _perFrameTimers.size(); ++i) {
    if (_perFrameTimers[i]._name == timer) {
      return i;
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
  uint32_t frame = _currentFrame;// == 0 ? MAX_FRAMES_IN_FLIGHT - 1 : _currentFrame - 1;

  VkQueryResultFlags queryResFlags = VK_QUERY_RESULT_64_BIT;
  /*if (firstDone) {
    queryResFlags |= VK_QUERY_RESULT_WAIT_BIT;
   }
  else {
    firstDone = true;
  }*/

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

      pfTimer._buf.emplace_back(pfTimer._durationMs);
      pfTimer._buf.erase(pfTimer._buf.begin());
    }
  }
}

void VulkanRenderer::registerPerFrameTimer(const std::string& name)
{
  PerFrameTimer timer{ name };
  timer._buf.resize(1000);
  _perFrameTimers.emplace_back(std::move(timer));
}

void VulkanRenderer::startTimer(const std::string& name, VkCommandBuffer cmdBuffer)
{
  int idx = findTimerIndex(name);

  //printf("Idx for %s is %d\n", name.c_str(), idx);

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

std::vector<PerFrameTimer> VulkanRenderer::getPerFrameTimers()
{
  return _perFrameTimers;
}

std::vector<MeshId> VulkanRenderer::getMeshIds(ModelId model)
{
  std::vector<MeshId> out;

  Model& m = _models[model];

  for (auto& mesh : m._meshes) {
    out.emplace_back(mesh._id);
  }

  return out;
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

void VulkanRenderer::prefillGPURenderableBuffer(VkCommandBuffer& commandBuffer)
{
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
    auto& renderable = _currentRenderables[i];

    if (!renderable._visible) continue;

    for (int i = 0; i < renderable._numMeshes; ++i) {
      _currentMeshUsage[renderable._firstMeshId + i]++;
    }    
    
    mappedData[i]._transform = renderable._transform;
    mappedData[i]._tint = glm::vec4(renderable._tint, 1.0f);
    mappedData[i]._firstMeshId = (uint32_t)renderable._firstMeshId;
    mappedData[i]._numMeshIds = (uint32_t)renderable._numMeshes;
    mappedData[i]._bounds = glm::vec4(renderable._boundingSphereCenter, renderable._boundingSphereRadius);
    mappedData[i]._visible = renderable._visible ? 1 : 0;
  }

  VkBufferCopy copyRegion{};
  copyRegion.dstOffset = 0;
  copyRegion.srcOffset = _currentStagingOffset;
  copyRegion.size = dataSize;
  vkCmdCopyBuffer(commandBuffer, _gpuStagingBuffer[_currentFrame]._buffer, _gpuRenderableBuffer[_currentFrame]._buffer, 1, &copyRegion);

  VkBufferMemoryBarrier memBarr{};
  memBarr.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  memBarr.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  memBarr.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
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

void VulkanRenderer::prefillGPUMeshMaterialBuffer(VkCommandBuffer& commandBuffer)
{
  // Go through each mesh of each model and update corresponding spot in the buffer
  std::size_t dataSize = _currentMeshes.size() * sizeof(gpu::GPUMeshMaterialInfo);
  uint8_t* data;
  vmaMapMemory(_vmaAllocator, _gpuStagingBuffer[_currentFrame]._allocation, (void**)&data);

  // Offset according to current staging buffer usage
  data = data + _currentStagingOffset;

  gpu::GPUMeshMaterialInfo* mappedData = reinterpret_cast<gpu::GPUMeshMaterialInfo*>(data);

  for (std::size_t i = 0; i < _currentMeshes.size(); ++i) {
    auto& mesh = _currentMeshes[i];

    mappedData[i]._metallicTexIndex = mesh._metallic._bindlessIndex;
    mappedData[i]._roughnessTexIndex = mesh._roughness._bindlessIndex;
    mappedData[i]._normalTexIndex = mesh._normal._bindlessIndex;
    mappedData[i]._albedoTexIndex = mesh._albedo._bindlessIndex;
    mappedData[i]._metallicRoughnessTexIndex = mesh._metallicRoughness._bindlessIndex;
  }

  VkBufferCopy copyRegion{};
  copyRegion.dstOffset = 0;
  copyRegion.srcOffset = _currentStagingOffset;
  copyRegion.size = dataSize;
  vkCmdCopyBuffer(commandBuffer, _gpuStagingBuffer[_currentFrame]._buffer, _gpuMeshMaterialInfoBuffer[_currentFrame]._buffer, 1, &copyRegion);

  VkBufferMemoryBarrier memBarr{};
  memBarr.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  memBarr.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  memBarr.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  memBarr.srcQueueFamilyIndex = _queueIndices.graphicsFamily.value();
  memBarr.dstQueueFamilyIndex = _queueIndices.graphicsFamily.value();
  memBarr.buffer = _gpuMeshMaterialInfoBuffer[_currentFrame]._buffer;
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

void VulkanRenderer::prefilGPULightBuffer(VkCommandBuffer& commandBuffer)
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
  std::size_t dataSize = _currentWindMap.height * _currentWindMap.width * 4;
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
  _currentStagingOffset += dataSize;
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
  _lights.resize(MAX_NUM_LIGHTS);

  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_real_distribution<> col(0.0, 1.0);
  std::uniform_real_distribution<> pos(0.0, 100.0);
  std::uniform_real_distribution<> rad(0.0, 10.0);
  std::uniform_real_distribution<> speed(1.0, 2.0);

  for (std::size_t i = 0; i < _lights.size(); ++i) {
    auto& light = _lights[i];
    float zNear = 0.1f;
    float zFar = 50.0f;

    light._type = Light::Point;

    glm::vec3 randomPos{
      pos(rng),
      .5f,
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
  * 6: TLAS for ray tracing (TLAS)
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
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR;
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
    lightLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
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
    materialLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings.emplace_back(std::move(materialLayoutBinding));

    VkDescriptorSetLayoutBinding tlasLayoutBinding{};
    tlasLayoutBinding.binding = _tlasBinding;
    tlasLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    tlasLayoutBinding.descriptorCount = 1;
    tlasLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    bindings.emplace_back(std::move(tlasLayoutBinding));

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
    pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

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

      matBufferInfo.buffer = _gpuMeshMaterialInfoBuffer[i]._buffer;
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
  _renderPasses.emplace_back(new CullRenderPass());
  _renderPasses.emplace_back(new ShadowRenderPass());
  _renderPasses.emplace_back(new GrassShadowRenderPass());
  _renderPasses.emplace_back(new GeometryRenderPass());
  _renderPasses.emplace_back(new GrassRenderPass());
  _renderPasses.emplace_back(new ShadowRayTracingPass());
  _renderPasses.emplace_back(new SSAORenderPass());
  _renderPasses.emplace_back(new SSAOBlurRenderPass());
  _renderPasses.emplace_back(new DeferredLightingRenderPass());
  _renderPasses.emplace_back(new FXAARenderPass());
  _renderPasses.emplace_back(new DebugViewRenderPass());
  _renderPasses.emplace_back(new UIRenderPass());
  _renderPasses.emplace_back(new PresentationRenderPass());

  return true;
}

bool VulkanRenderer::initViewClusters()
{
  // Debug render
  /*std::vector<Vertex> vert;
  std::vector<std::uint32_t> inds;
  graphicsutil::createUnitCube(&vert, &inds, false);

  auto meshId = registerMesh(vert, inds);*/

  std::size_t w = _swapChain._swapChainExtent.width;
  std::size_t h = _swapChain._swapChainExtent.height;
  double nearCam = _latestCamera.getNear();
  double farCam = _latestCamera.getFar();

  auto proj = _latestCamera.getProjection();
  auto view = _latestCamera.getCamMatrix();
  auto invView = glm::inverse(view);
  //proj[1][1] *= -1;
  auto invProj = glm::inverse(proj);

  std::size_t numClustersX = w / NUM_PIXELS_CLUSTER_X;
  std::size_t numClustersY = h / NUM_PIXELS_CLUSTER_Y;
  std::size_t numSlices = NUM_CLUSTER_DEPTH_SLIZES;

  _viewClusters.resize(numClustersX * numClustersY * numSlices);

  // Step sizes in NDC space (x and y -1 to 1, z 0 to 1)
  double xStep = 2.0 / double(numClustersX);
  double yStep = 2.0 / double(numClustersY);

  for (std::size_t z = 0; z < numSlices; ++z)
  for (std::size_t y = 0; y < numClustersY; ++y)
  for (std::size_t x = 0; x < numClustersX; ++x) {
    // Find NDC points of this particular cluster
    double ndcMinX = double(x) * xStep - 1.0;
    double ndcMaxX = double(x + 1) * xStep - 1.0;
    double ndcMinY = double(y) * yStep - 1.0;
    double ndcMaxY = double(y + 1) * yStep - 1.0;

    glm::vec4 minBound(ndcMinX, ndcMinY, 1.0, 1.0);
    glm::vec4 maxBound(ndcMaxX, ndcMaxY, 1.0, 1.0);

    // Tramsform to view space by inverse projection
    auto minBoundView = invProj * minBound;
    minBoundView /= minBoundView.w;

    auto maxBoundView = invProj * maxBound;
    maxBoundView /= maxBoundView.w;

    glm::vec3 minEye{ minBoundView.x, minBoundView.y, minBoundView.z };
    glm::vec3 maxEye{ maxBoundView.x, maxBoundView.y, maxBoundView.z };

    // calculate near and far depth edges of the cluster
    double clusterNear = -nearCam * pow(farCam / nearCam, double(z) / double(NUM_CLUSTER_DEPTH_SLIZES));
    double clusterFar = -nearCam * pow(farCam / nearCam, (double(z) + 1.0) / double(NUM_CLUSTER_DEPTH_SLIZES));

    // this calculates the intersection between:
    // - a line from the camera (origin) to the eye point (at the camera's far plane)
    // - the cluster's z-planes (near + far)
    // we could divide by u_zFar as well
    glm::vec3 minNear = minEye * (float)clusterNear / minEye.z;
    glm::vec3 minFar = minEye * (float)clusterFar / minEye.z;
    glm::vec3 maxNear = maxEye * (float)clusterNear / maxEye.z;
    glm::vec3 maxFar = maxEye * (float)clusterFar / maxEye.z;

    glm::vec3 minBounds = glm::min(glm::min(minNear, minFar), glm::min(maxNear, maxFar));
    glm::vec3 maxBounds = glm::max(glm::max(minNear, minFar), glm::max(maxNear, maxFar));

    ViewCluster cluster{
      glm::vec3(minBounds.x, minBounds.y, minBounds.z),
      glm::vec3(maxBounds.x, maxBounds.y, maxBounds.z) };

    // Find index of this cluster
    std::size_t index =
      z * numClustersX * numClustersY +
      y * numClustersX +
      x;

    //printf("Index: %zu\n", index);

    /*printf("\nCluster index %zu has min and max: %lf %lf %lf | %lf %lf %lf\n", index,
      minBound.x, minBound.y, minBound.z,
      maxBound.x, maxBound.y, maxBound.z);
    printf("(NDC was %lf %lf %lf | %lf %lf %lf)\n", ndcMinX, ndcMinY, ndcMinZ, ndcMaxX, ndcMaxY, ndcMaxZ);*/

    // For debugging, world space
    /*auto worldMin = invView * glm::vec4(minBounds, 1.0);
    auto worldMax = invView * glm::vec4(maxBounds, 1.0);

    //printf("World: %lf %lf %lf | %lf %lf %lf\n", worldMin.x, worldMin.y, worldMin.z, worldMax.x, worldMax.y, worldMax.z);

    // Debug
    //glm::vec3 center = (worldMin + worldMax) / 2.0f;
    glm::vec3 center = (minBounds + maxBounds) / 2.0f;

    glm::mat4 trans = glm::translate(glm::mat4(1.0f), center);
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(
      abs(maxBounds.x - minBounds.x),
      abs(maxBounds.y - minBounds.y),
      abs(maxBounds.z - minBounds.z)));
    auto rend = registerRenderable(meshId, invView * trans * scale, glm::vec3(1.0f), 5000.0f);
    setRenderableTint(rend, graphicsutil::randomColor());*/

    _viewClusters[index] = std::move(cluster);
  }

  // Create a staging buffer on CPU side first
  AllocatedBuffer stagingBuffer;
  std::size_t dataSize = _viewClusters.size() * sizeof(gpu::GPUViewCluster);

  bufferutil::createBuffer(_vmaAllocator, dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, stagingBuffer);

  gpu::GPUViewCluster* mappedData;
  vmaMapMemory(_vmaAllocator, stagingBuffer._allocation, &(void*)mappedData);

  for (std::size_t i = 0; i < _viewClusters.size(); ++i) {
    mappedData[i]._max = glm::vec4(_viewClusters[i]._maxBounds, 0.0);
    mappedData[i]._min = glm::vec4(_viewClusters[i]._minBounds, 0.0);
  }

  vmaUnmapMemory(_vmaAllocator, stagingBuffer._allocation);

  auto cmdBuffer = beginSingleTimeCommands();

  VkBufferCopy copyRegion{};
  copyRegion.size = dataSize;

  for (std::size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    vkCmdCopyBuffer(cmdBuffer, stagingBuffer._buffer, _gpuViewClusterBuffer[i]._buffer, 1, &copyRegion);
  }

  endSingleTimeCommands(cmdBuffer);

  vmaDestroyBuffer(_vmaAllocator, stagingBuffer._buffer, stagingBuffer._allocation);

  return true;
}

bool VulkanRenderer::initGigaMeshBuffer()
{
  // Allocate "big enough" size... 
  bufferutil::createBuffer(_vmaAllocator, _gigaMeshBuffer._size, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 0, _gigaMeshBuffer._buffer);
  return true;
}

bool VulkanRenderer::initGpuBuffers()
{
  _gpuStagingBuffer.resize(MAX_FRAMES_IN_FLIGHT);
  _gpuRenderableBuffer.resize(MAX_FRAMES_IN_FLIGHT);
  _gpuMeshMaterialInfoBuffer.resize(MAX_FRAMES_IN_FLIGHT);
  _gpuSceneDataBuffer.resize(MAX_FRAMES_IN_FLIGHT);
  _gpuWindForceSampler.resize(MAX_FRAMES_IN_FLIGHT);
  _gpuWindForceImage.resize(MAX_FRAMES_IN_FLIGHT);
  _gpuWindForceView.resize(MAX_FRAMES_IN_FLIGHT);
  _gpuLightBuffer.resize(MAX_FRAMES_IN_FLIGHT);
  _gpuViewClusterBuffer.resize(MAX_FRAMES_IN_FLIGHT);

  for (std::size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    // Create a staging buffer that will be used to temporarily hold data that is to be copied to the gpu buffers.
    bufferutil::createBuffer(
      _vmaAllocator,
      MAX_NUM_RENDERABLES * sizeof(gpu::GPURenderable) * 2, // We use this as the size, as the GPU buffers can never get bigger than this
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
      _gpuStagingBuffer[i]);

    // Create a buffer that will contain renderable information for use by the frustum culling compute shader.
    bufferutil::createBuffer(
      _vmaAllocator,
      MAX_NUM_RENDERABLES * sizeof(gpu::GPURenderable),
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      0,
      _gpuRenderableBuffer[i]);

    // Create a buffer that will contain renderable information for use by the frustum culling compute shader.
    bufferutil::createBuffer(
      _vmaAllocator,
      MAX_NUM_MESHES * sizeof(gpu::GPUMeshMaterialInfo),
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      0,
      _gpuMeshMaterialInfoBuffer[i]);

    // Used as a UBO in most shaders for accessing scene data.
    bufferutil::createBuffer(
      _vmaAllocator,
      sizeof(gpu::GPUSceneData),
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, // If this turns out to be a bottle neck we have to switch to a staging buffer
      _gpuSceneDataBuffer[i]);

    bufferutil::createBuffer(
      _vmaAllocator,
      MAX_NUM_LIGHTS * sizeof(gpu::GPULight),
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      0,
      _gpuLightBuffer[i]);

    bufferutil::createBuffer(
      _vmaAllocator,
      120 * 120 * 10 * sizeof(gpu::GPUViewCluster),
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      0,
      _gpuViewClusterBuffer[i]);

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
    res->_view = _gpuWindForceView[i];
    _vault.addResource("WindForceImage", std::unique_ptr<IRenderResource>(res), true, i, true);
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
  init_info.ColorAttachmentFormat = VK_FORMAT_R8G8B8A8_UNORM;

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
}

void VulkanRenderer::drawFrame(bool applyPostProcessing, bool debug)
{
  if (_currentRenderables.empty()) return;
  // At the start of the frame, we want to wait until the previous frame has finished, so that the command buffer and semaphores are available to use.
  vkWaitForFences(_device, 1, &_inFlightFences[_currentFrame], VK_TRUE, UINT64_MAX);

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
  VkFormat format,
  int width,
  int height,
  const std::vector<uint8_t>& data,
  VkSampler& samplerOut,
  AllocatedImage& imageOut,
  VkImageView& viewOut)
{
  AllocatedImage image;

  imageutil::createImage(
    width, height,
    format,
    VK_IMAGE_TILING_OPTIMAL,
    _vmaAllocator,
    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    image);

  auto view = imageutil::createImageView(
    _device,
    image._image,
    format,
    VK_IMAGE_ASPECT_COLOR_BIT);

  // Create a staging buffer on CPU side first
  AllocatedBuffer stagingBuffer;
  std::size_t dataSize = data.size();

  bufferutil::createBuffer(_vmaAllocator, dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, stagingBuffer);

  glm::uint8_t* mappedData;
  vmaMapMemory(_vmaAllocator, stagingBuffer._allocation, &(void*)mappedData);
  std::memcpy(mappedData, data.data(), data.size());
  vmaUnmapMemory(_vmaAllocator, stagingBuffer._allocation);

  auto cmdBuffer = beginSingleTimeCommands();

  // Transition image to dst optimal
  imageutil::transitionImageLayout(
    cmdBuffer,
    image._image,
    format,
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  VkExtent3D extent{};
  extent.depth = 1;
  extent.height = height;
  extent.width = width;

  VkOffset3D offset{};

  VkBufferImageCopy imCopy{};
  imCopy.imageExtent = extent;
  imCopy.imageOffset = offset;
  imCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imCopy.imageSubresource.layerCount = 1;

  vkCmdCopyBufferToImage(
    cmdBuffer,
    stagingBuffer._buffer,
    image._image,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    1,
    &imCopy);

  // Transition to shader read
  imageutil::transitionImageLayout(
    cmdBuffer,
    image._image,
    format,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  endSingleTimeCommands(cmdBuffer);

  vmaDestroyBuffer(_vmaAllocator, stagingBuffer._buffer, stagingBuffer._allocation);

  SamplerCreateParams params{};
  params.renderContext = this;

  imageOut = image;
  viewOut = view;
  samplerOut = createSampler(params);
}

uint32_t VulkanRenderer::addTextureToBindless(VkImageLayout layout, VkImageView view, VkSampler sampler)
{
  VkWriteDescriptorSet descriptorWrite{};

  VkDescriptorImageInfo imageInfo{};
  imageInfo.imageLayout = layout;
  imageInfo.imageView = view;
  imageInfo.sampler = sampler;

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = _bindlessDescriptorSets[i];
    descriptorWrite.dstBinding = _bindlessTextureBinding;
    descriptorWrite.dstArrayElement = _currentBindlessTextureIndex;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(_device, 1, &descriptorWrite, 0, nullptr);
  }

  return _currentBindlessTextureIndex++;
}

void VulkanRenderer::executeFrameGraph(VkCommandBuffer commandBuffer, int imageIndex)
{
  // Start cmd buffer
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = 0; // Optional
  beginInfo.pInheritanceInfo = nullptr; // Optional

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    printf("failed to begin recording command buffer!\n");
    return;
  }

  resetPerFrameQueries(_commandBuffers[_currentFrame]);

  // The swapchain image has to go to transfer DST, which is the last thing the frame graph does.
  imageutil::transitionImageLayout(commandBuffer, _swapChain._swapChainImages[imageIndex], _swapChain._swapChainImageFormat,
    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  // Prefill renderable buffer
  // This is unnecessary to do every frame!
  if (_renderablesChanged[_currentFrame]) {
    prefillGPURenderableBuffer(commandBuffer);
    _renderablesChanged[_currentFrame] = false;
  }

  if (_meshesChanged[_currentFrame]) {
    prefillGPUMeshMaterialBuffer(commandBuffer);
    _meshesChanged[_currentFrame] = false;
  }

  // Only do if lights have updated
  prefilGPULightBuffer(commandBuffer);

  // Update windforce texture
  updateWindForceImage(commandBuffer);

  // Bind giga buffers and bindless descriptors
  VkDeviceSize offsets[] = { 0 };
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, &_gigaMeshBuffer._buffer._buffer, offsets);
  vkCmdBindIndexBuffer(commandBuffer, _gigaMeshBuffer._buffer._buffer, 0, VK_INDEX_TYPE_UINT32);

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _bindlessPipelineLayout, 0, 1, &_bindlessDescriptorSets[_currentFrame], 0, nullptr);
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, _bindlessPipelineLayout, 0, 1, &_bindlessDescriptorSets[_currentFrame], 0, nullptr);
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, _bindlessPipelineLayout, 0, 1, &_bindlessDescriptorSets[_currentFrame], 0, nullptr);

  _fgb.executeGraph(commandBuffer, this);

  // The swapchain image has to go to present, which is the last thing the frame graph does.
  imageutil::transitionImageLayout(commandBuffer, _swapChain._swapChainImages[imageIndex], _swapChain._swapChainImageFormat,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    printf("failed to record command buffer!\n");
  }
}

void VulkanRenderer::notifyFramebufferResized()
{
  _framebufferResized = true;
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

/*void VulkanRenderer::drawGigaBuffer(VkCommandBuffer* commandBuffer)
{
  // There is an assumption that the giga buffers have been bound already.

  for (auto& renderable : _currentRenderables) {
    if (!renderable._visible) continue;

    auto& mesh = _currentMeshes[renderable._meshId];

    // Draw command
    vkCmdDrawIndexed(
      *commandBuffer,
      (uint32_t)mesh._numIndices,
      1,
      (uint32_t)mesh._indexOffset,
      (uint32_t)mesh._vertexOffset,
      0);
  }
}*/

void VulkanRenderer::drawGigaBufferIndirect(VkCommandBuffer* commandBuffer, VkBuffer drawCalls)
{
  vkCmdDrawIndexedIndirect(*commandBuffer, drawCalls, 0, (uint32_t)_currentMeshes.size(), sizeof(gpu::GPUDrawCallCmd));
}

void VulkanRenderer::drawNonIndexIndirect(VkCommandBuffer* cmdBuffer, VkBuffer drawCalls, uint32_t drawCount, uint32_t stride)
{
  vkCmdDrawIndirect(*cmdBuffer, drawCalls, 0, drawCount, stride);
}

void VulkanRenderer::drawMeshId(VkCommandBuffer* cmdBuffer, MeshId meshId, uint32_t vertCount, uint32_t instanceCount)
{
  auto& mesh = _currentMeshes[meshId];
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

std::vector<Mesh>& VulkanRenderer::getCurrentMeshes()
{
  return _currentMeshes;
}

std::unordered_map<MeshId, std::size_t>& VulkanRenderer::getCurrentMeshUsage()
{
  return _currentMeshUsage;
}

size_t VulkanRenderer::getCurrentNumRenderables()
{
  return _currentRenderables.size();
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
  auto wind = glm::normalize(_currentWindMap.windDir);
  cullPushConstants._windDirX = wind.x;
  cullPushConstants._windDirY = wind.y;

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
  return _currentWindMap.windDir;
}

}