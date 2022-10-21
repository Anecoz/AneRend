#include "VulkanRenderer.h"

#include "MaterialFactory.h"
#include "QueueFamilyIndices.h"
#include "ImageHelpers.h"
#include "BufferHelpers.h"

#include "../util/Utils.h"
#include "../LodePng/lodepng.h"
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_glfw.h"
#include "../imgui/imgui_impl_vulkan.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>
#include <iostream>
#include <set>

namespace {

// EXTRA FUNCTION LOADS
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) {
    return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr) {
    func(instance, debugMessenger, pAllocator);
  }
}

VKAPI_ATTR VkBool32 VKAPI_CALL g_DebugCallback(
  VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
  VkDebugUtilsMessageTypeFlagsEXT messageType,
  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
  void* pUserData) {

  if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    printf("\nvalidation layer: %s\n", pCallbackData->pMessage);
  }

  return VK_FALSE;
}

}

namespace render {

VulkanRenderer::VulkanRenderer(GLFWwindow* window)
  : _window(window)
  , _enableValidationLayers(true)
{}

VulkanRenderer::~VulkanRenderer()
{
  // Let logical device finish operations first
  vkDeviceWaitIdle(_device);

  _shadowPass.cleanup(_vmaAllocator, _device);
  _ppPass.cleanup(_vmaAllocator, _device);

  vmaDestroyBuffer(_vmaAllocator, _gigaMeshBuffer._buffer._buffer, _gigaMeshBuffer._buffer._allocation);

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

  for (const auto& material: _materials) {
    for (auto& ubo: material.second._ubos[Material::STANDARD_INDEX]) {
      vmaDestroyBuffer(_vmaAllocator, ubo._buffer, ubo._allocation);
    }
    for (auto& ubo: material.second._ubos[Material::SHADOW_INDEX]) {
      vmaDestroyBuffer(_vmaAllocator, ubo._buffer, ubo._allocation);
    }

    vkDestroyDescriptorSetLayout(_device, material.second._descriptorSetLayouts[Material::STANDARD_INDEX], nullptr);
    vkDestroyPipeline(_device, material.second._pipelines[Material::STANDARD_INDEX], nullptr);
    vkDestroyPipelineLayout(_device, material.second._pipelineLayouts[Material::STANDARD_INDEX], nullptr);

    vkDestroyDescriptorSetLayout(_device, material.second._descriptorSetLayouts[Material::SHADOW_INDEX], nullptr);
    vkDestroyPipeline(_device, material.second._pipelines[Material::SHADOW_INDEX], nullptr);
    vkDestroyPipelineLayout(_device, material.second._pipelineLayouts[Material::SHADOW_INDEX], nullptr);
  }

  for (const auto& material: _ppMaterials) {
    vkDestroyDescriptorSetLayout(_device, material.first._descriptorSetLayouts[Material::POST_PROCESSING_INDEX], nullptr);
    vkDestroyPipeline(_device, material.first._pipelines[Material::POST_PROCESSING_INDEX], nullptr);
    vkDestroyPipelineLayout(_device, material.first._pipelineLayouts[Material::POST_PROCESSING_INDEX], nullptr);
  }

  cleanupSwapChain();

  for (const auto& renderable: _currentRenderables) {
    cleanupRenderable(renderable);
  }

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    vkDestroySemaphore(_device, _imageAvailableSemaphores[i], nullptr);
    vkDestroySemaphore(_device, _renderFinishedSemaphores[i], nullptr);
    vkDestroyFence(_device, _inFlightFences[i], nullptr);
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
  
  printf("Creating depth resources...");
  res &= createDepthResources();
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

  printf("Init shadow pass...");
  res &= initShadowpass();
  if (!res) return false;
  printf("Done!\n");

  printf("Init lights...");
  res &= initLights();
  if (!res) return false;
  printf("Done!\n");

  printf("Init pp pass...");
  res &= initPostProcessingPass();
  if (!res) return false;
  printf("Done!\n");

  printf("Loading known materials...");
  res &= loadKnownMaterials();
  if (!res) return false;
  printf("Done!\n");

  printf("Init fat mesh buffer...");
  res &= initGigaMeshBuffer();
  if (!res) return false;
  printf("Done!\n");

  printf("Init shadow debug...");
  res &= initShadowDebug();
  if (!res) return false;
  printf("Done!\n");

  printf("Init pp renderable...");
  res &= initPostProcessingRenderable();
  if (!res) return false;
  printf("Done!\n");

  printf("Init imgui...");
  res &= initImgui();
  if (!res) return false;
  printf("Done!\n");

  return res;
}

MeshId VulkanRenderer::registerMesh(const std::vector<Vertex>& vertices, const std::vector<std::uint32_t>& indices)
{
  // Check if we need to pad with 0's before the vertices
  std::vector<std::uint8_t> verticesCopy;
  verticesCopy.resize(vertices.size() * sizeof(Vertex));
  memcpy(verticesCopy.data(), vertices.data(), verticesCopy.size());
  
  std::size_t numZeroes = sizeof(Vertex) - (_gigaMeshBuffer._freeSpacePointer % sizeof(Vertex));

  if (numZeroes != 0) {
    std::vector<std::uint8_t> zeroVec(numZeroes, 0);
    verticesCopy.insert(verticesCopy.begin(), zeroVec.begin(), zeroVec.end());
  }

  // Create a staging buffer on CPU side first
  AllocatedBuffer stagingBuffer;
  std::size_t vertSize = verticesCopy.size();
  std::size_t indSize = indices.size() * sizeof(std::uint32_t);
  std::size_t dataSize = vertSize + indSize;

  bufferutil::createBuffer(_vmaAllocator, dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, stagingBuffer);

  void* mappedData;
  vmaMapMemory(_vmaAllocator, stagingBuffer._allocation, &mappedData);
  memcpy(mappedData, verticesCopy.data(), vertSize);
  if (indSize > 0) {
    memcpy((uint8_t*)mappedData + vertSize, indices.data(), indSize);
  }
  vmaUnmapMemory(_vmaAllocator, stagingBuffer._allocation);

  // Find where to copy data in the fat buffer
  auto cmdBuffer = beginSingleTimeCommands();

  VkBufferCopy copyRegion{};
  copyRegion.dstOffset = _gigaMeshBuffer._freeSpacePointer;
  copyRegion.size = dataSize;
  vkCmdCopyBuffer(cmdBuffer, stagingBuffer._buffer, _gigaMeshBuffer._buffer._buffer, 1, &copyRegion);

  endSingleTimeCommands(cmdBuffer);

  Mesh mesh {};
  mesh._id = ++_nextMeshId;
  mesh._numVertices = verticesCopy.size();
  mesh._numIndices = indices.size();
  mesh._vertexOffset = (_gigaMeshBuffer._freeSpacePointer + numZeroes) / sizeof(Vertex);
  mesh._indexOffset = (_gigaMeshBuffer._freeSpacePointer + vertSize) / sizeof(uint32_t);

  _currentMeshes.emplace_back(std::move(mesh));

  // Advance free space pointer
  _gigaMeshBuffer._freeSpacePointer += dataSize;

  vmaDestroyBuffer(_vmaAllocator, stagingBuffer._buffer, stagingBuffer._allocation);

  return _currentMeshes.size() - 1;
}

RenderableId VulkanRenderer::registerRenderable(
  MeshId meshId,
  MaterialID materialId,
  const glm::mat4& transform,
  const glm::vec3& sphereBoundCenter,
  float sphereBoundRadius)
{
  if (!materialIdExists(materialId)) {
    printf("Material id %zu does not exist!\n", materialId);
    return 0;
  }

  if (!meshIdExists(meshId)) {
    printf("Mesh id %zu does not exist!\n", meshId);
    return 0;
  }

  Renderable renderable{};

  /*std::uint8_t* ptr = (std::uint8_t*)vertices.data();
  if (!createVertexBuffer(ptr, sizeof(Vertex) * vertices.size(), renderable._vertexBuffer)) {
    printf("Could not create vertex buffer!\n");
    return 0;
  }

  if (!indices.empty()) {
    if (!createIndexBuffer(indices, renderable._indexBuffer)) {
      printf("Could not create index buffer!\n");
      return 0;
    }
  }*/

  auto id = _nextRenderableId++;

  renderable._id = id;
  renderable._materialId = materialId;
  renderable._meshId = meshId;
  renderable._transform = transform;
  renderable._boundingSphereCenter = sphereBoundCenter;
  renderable._boundingSphereRadius = sphereBoundRadius;

  _currentRenderables.emplace_back(std::move(renderable));

  // Sort by material
  std::sort(_currentRenderables.begin(), _currentRenderables.end(), [](const Renderable& a, const Renderable& b) {
    return a._materialId < b._materialId;
  });

  return id;
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

void VulkanRenderer::queuePushConstant(RenderableId id, std::uint32_t size, void* pushConstants)
{
  // Sanity check, does the material of this renderable support push constants?
  // Also, does it even exist?
  Renderable* renderable = nullptr;
  for (auto& r: _currentRenderables) {
    if (r._id == id) {
      renderable = &r;
      if (!_materials[r._materialId]._supportsPushConstants) {
        printf("Could not queue push constant, because material doesn't support it\n");
        return;
      }
    }
  }

  if (!renderable) {
    printf("Could not queue push constant, because the renderable doesn't exist\n");
    return;
  }

  renderable->_pushConstantsSize = size;
  memcpy(&renderable->_pushConstants[0], pushConstants, size);
}

void VulkanRenderer::update(const Camera& camera, const Camera& shadowCamera, const glm::vec4& lightDir, double delta)
{
  // TODO: We can't write to this frames UBO's until the GPU is finished with it.
  // If we run unlimited FPS we are quite likely to end up doing just that.
  // The ubo memory write _will_ be visible by the next queueSubmit, so that isn't the issue.
  // but we might be writing into memory that is currently accessed by the GPU.
  vkWaitForFences(_device, 1, &_inFlightFences[_currentFrame], VK_TRUE, UINT64_MAX);

  // Update UBOs
  StandardUBO standardUbo{};
  standardUbo.view = camera.getCamMatrix();
  standardUbo.proj = camera.getProjection();
  standardUbo.cameraPos = glm::vec4(camera.getPosition(), 1.0);
  standardUbo.lightDir = lightDir;
  standardUbo.proj[1][1] *= -1;

  for (std::size_t i = 0; i < _lights.size(); ++i) {
    auto shadowProj = _lights[i]._proj;
    shadowProj[1][1] *= -1;

    _lights[i].debugUpdatePos(delta);
    _lights[i].updateViewMatrices();
    for (std::size_t v = 0; v < 6; ++v) {
      standardUbo.shadowMatrix[i * 6 + v] = shadowProj * _lights[i]._view[v];
    }

    standardUbo.lightPos[i] = glm::vec4(_lights[i]._pos, 1.0f);
    standardUbo.lightColor[i] = glm::vec4(_lights[i]._color, 1.0f);
  }

  ShadowUBO shadowUbo{};
  shadowUbo.shadowMatrix = standardUbo.shadowMatrix[0];

  for (auto& mat: _materials) {
    mat.second.updateUbos(_currentFrame, _vmaAllocator, standardUbo, shadowUbo);
  }
}

bool VulkanRenderer::materialIdExists(MaterialID id) const
{
  return _materials.find(id) != _materials.end();
}

bool VulkanRenderer::meshIdExists(MeshId meshId) const
{
  return meshId < (int64_t)_currentMeshes.size();
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

  bool swapChainAdequate = false;
  if (extensionsSupported) {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, _surface);
    swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
  }

  return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
           deviceFeatures.geometryShader && queueFamIndices.isComplete() && extensionsSupported 
           && swapChainAdequate && deviceFeatures.samplerAnisotropy;
}

QueueFamilyIndices VulkanRenderer::findQueueFamilies(VkPhysicalDevice device)
{
  QueueFamilyIndices indices;

  std::uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

  int i = 0;
  for (const auto& queueFamily : queueFamilies) {
    if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      indices.graphicsFamily = i;
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
  std::set<uint32_t> uniqueQueueFamilies = {familyIndices.graphicsFamily.value(), familyIndices.presentFamily.value()};

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

  VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeature{};
  dynamicRenderingFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
  dynamicRenderingFeature.dynamicRendering = VK_TRUE;

  createInfo.pNext = &dynamicRenderingFeature;

  if (vkCreateDevice(_physicalDevice, &createInfo, nullptr, &_device) != VK_SUCCESS) {
    printf("Could not create logical device!\n");
    return false;
  }

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
  vkDestroyImageView(_device, _depthImageView, nullptr);
  vmaDestroyImage(_vmaAllocator, _depthImage._image, _depthImage._allocation);

  _swapChain.cleanup(_device);
}

void VulkanRenderer::recreateSwapChain()
{
  vkDeviceWaitIdle(_device);

  cleanupSwapChain();

  createSwapChain();
  createDepthResources();
}

void VulkanRenderer::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
  // Create a command buffer which we immediately will start recording the copy command to
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkBufferCopy copyRegion{};
  copyRegion.size = size;
  vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

  endSingleTimeCommands(commandBuffer);
}

bool VulkanRenderer::createSwapChain()
{
  int width, height;
  glfwGetFramebufferSize(_window, &width, &height);
  return _swapChain.create(_device, _physicalDevice, _surface, findQueueFamilies(_physicalDevice), width, height) &&
         _swapChain.createImageViews(_device);
}

bool VulkanRenderer::loadKnownMaterials()
{
  // Create vectors for the light cubemap textures and samplers
  std::vector<VkImageView*> lightCubeViews;
  std::vector<VkSampler*> lightSamplers;
  for (auto& light: _lights) {
    lightCubeViews.emplace_back(&light._cubeShadowView);
    lightSamplers.emplace_back(&light._sampler);
  }

  // Standard material
  auto standardMaterial = MaterialFactory::createStandardMaterial(
    _device, _swapChain._swapChainImageFormat, findDepthFormat(),
    MAX_FRAMES_IN_FLIGHT, _descriptorPool, _vmaAllocator,
    lightCubeViews, lightSamplers, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);

  if (!standardMaterial) {
    printf("Could not create standard material!\n");
    return false;
  }

  _materials[STANDARD_MATERIAL_ID] = std::move(standardMaterial);

  // Standard instanced material
  auto standardInstancedMaterial = MaterialFactory::createStandardInstancedMaterial(
    _device, _swapChain._swapChainImageFormat, findDepthFormat(),
    MAX_FRAMES_IN_FLIGHT, _descriptorPool, _vmaAllocator,
    lightCubeViews, lightSamplers, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);

  if (!standardInstancedMaterial) {
    printf("Could not create standard instanced material!\n");
    return false;
  }
  _materials[STANDARD_INSTANCED_MATERIAL_ID] = std::move(standardInstancedMaterial);

  // Shadow debug
  auto shadowDebugMat = MaterialFactory::createShadowDebugMaterial(
    _device, _swapChain._swapChainImageFormat, findDepthFormat(),
    MAX_FRAMES_IN_FLIGHT, _descriptorPool, _vmaAllocator,
    &_lights[0]._shadowImageViews[0], &_shadowPass._shadowSampler, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);

  if (!shadowDebugMat) {
    printf("Could not create shadow debug material!\n");
    return false;
  }
  _materials[SHADOW_DEBUG_MATERIAL_ID] = std::move(shadowDebugMat);

  // Post processing
  auto cmdBuffer = beginSingleTimeCommands();

  // FXAA
  {
    std::size_t idx;
    _ppPass.createResources(_device, _vmaAllocator, cmdBuffer, &idx);
    auto ppMat = MaterialFactory::createPPFxaaMaterial(
      _device, _swapChain._swapChainImageFormat, findDepthFormat(),
      MAX_FRAMES_IN_FLIGHT, _descriptorPool, _vmaAllocator,
      &_ppPass._ppResources[idx]._ppInputImageView, &_ppPass._ppResources[idx]._ppSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    if (!ppMat) {
      printf("Could not create pp material!\n");
      return false;
    }
    _ppMaterials.emplace_back(std::make_pair(std::move(ppMat), idx));
  }

  // Blur
  /*{
    std::size_t idx;
    _ppPass.createResources(_device, _vmaAllocator, cmdBuffer, &idx);
    auto ppMat = MaterialFactory::createPPBlurMaterial(
      _device, _swapChain._swapChainImageFormat, findDepthFormat(),
      MAX_FRAMES_IN_FLIGHT, _descriptorPool, _vmaAllocator,
      &_ppPass._ppResources[idx]._ppInputImageView, &_ppPass._ppResources[idx]._ppSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    if (!ppMat) {
      printf("Could not create pp material!\n");
      return false;
    }
    _ppMaterials.emplace_back(std::make_pair(std::move(ppMat), idx));
  }*/

  /*// Flip
  {
    std::size_t idx;
    _ppPass.createResources(_device, _vmaAllocator, cmdBuffer, &idx);
    auto flipMat = MaterialFactory::createPPFlipMaterial(
      _device, _swapChain._swapChainImageFormat, findDepthFormat(),
      MAX_FRAMES_IN_FLIGHT, _descriptorPool, _vmaAllocator,
      &_ppPass._ppResources[idx]._ppInputImageView, &_ppPass._ppResources[idx]._ppSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    if (!flipMat) {
      printf("Could not create pp flip material!\n");
      return false;
    }
    _ppMaterials.emplace_back(std::make_pair(std::move(flipMat), idx));
  }

  // Color inv
  {
    std::size_t idx;
    _ppPass.createResources(_device, _vmaAllocator, cmdBuffer, &idx);
    auto ppMat = MaterialFactory::createPPColorInvMaterial(
      _device, _swapChain._swapChainImageFormat, findDepthFormat(),
      MAX_FRAMES_IN_FLIGHT, _descriptorPool, _vmaAllocator,
      &_ppPass._ppResources[idx]._ppInputImageView, &_ppPass._ppResources[idx]._ppSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    if (!ppMat) {
      printf("Could not create pp material!\n");
      return false;
    }
    _ppMaterials.emplace_back(std::make_pair(std::move(ppMat), idx));
  }*/
  endSingleTimeCommands(cmdBuffer);

  return true;
}

bool VulkanRenderer::createCommandPool()
{
  QueueFamilyIndices queueFamilyIndices = findQueueFamilies(_physicalDevice);

  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

  if (vkCreateCommandPool(_device, &poolInfo, nullptr, &_commandPool) != VK_SUCCESS) {
    printf("failed to create command pool!\n");
    return false;
  }

  return true;
}

bool VulkanRenderer::createDepthResources()
{
  VkFormat depthFormat = findDepthFormat();

  imageutil::createImage(_swapChain._swapChainExtent.width, _swapChain._swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL,
    _vmaAllocator, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, _depthImage);
  _depthImageView = imageutil::createImageView(_device, _depthImage._image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

  auto cmdBuffer = beginSingleTimeCommands();
  imageutil::transitionImageLayout(cmdBuffer, _depthImage._image, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
  endSingleTimeCommands(cmdBuffer);

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
  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  vkQueueSubmit(_graphicsQ, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(_graphicsQ);

  vkFreeCommandBuffers(_device, _commandPool, 1, &commandBuffer);
}

void VulkanRenderer::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkBufferImageCopy region{};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;

  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;

  region.imageOffset = {0, 0, 0};
  region.imageExtent = {
    width,
    height,
    1
  };

  vkCmdCopyBufferToImage(
    commandBuffer,
    buffer,
    image,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    1,
    &region
  );

  endSingleTimeCommands(commandBuffer);
}

bool VulkanRenderer::createVertexBuffer(std::uint8_t* data, std::size_t dataSize, AllocatedBuffer& buffer)
{
  VkDeviceSize bufferSize = dataSize;

  // Create a staging buffer
  AllocatedBuffer stagingBuffer;
  bufferutil::createBuffer(_vmaAllocator, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, stagingBuffer);

  void* mappedData;
  vmaMapMemory(_vmaAllocator, stagingBuffer._allocation, &mappedData);
  memcpy(mappedData, data, (size_t) bufferSize);
  vmaUnmapMemory(_vmaAllocator, stagingBuffer._allocation);

  // Device local buffer, optimised for GPU
  bufferutil::createBuffer(_vmaAllocator, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 0, buffer);

  copyBuffer(stagingBuffer._buffer, buffer._buffer, bufferSize);

  vmaDestroyBuffer(_vmaAllocator, stagingBuffer._buffer, stagingBuffer._allocation);

  return true;
}

bool VulkanRenderer::createIndexBuffer(const std::vector<std::uint32_t>& indices, AllocatedBuffer& buffer)
{
  VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

  AllocatedBuffer stagingBuffer;
  bufferutil::createBuffer(_vmaAllocator, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, stagingBuffer);

  void* data;
  vmaMapMemory(_vmaAllocator, stagingBuffer._allocation, &data);
  memcpy(data, indices.data(), (size_t) bufferSize);
  vmaUnmapMemory(_vmaAllocator, stagingBuffer._allocation);

  bufferutil::createBuffer(_vmaAllocator, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 0, buffer);

  copyBuffer(stagingBuffer._buffer, buffer._buffer, bufferSize);

  vmaDestroyBuffer(_vmaAllocator, stagingBuffer._buffer, stagingBuffer._allocation);

  return true;
}

bool VulkanRenderer::createDescriptorPool()
{
  VkDescriptorPoolSize poolSizes[] =
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

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
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

void VulkanRenderer::drawRenderables(
  VkCommandBuffer& commandBuffer,
  std::size_t materialIndex,
  bool shadowSupportRequired,
  VkViewport& viewport, VkRect2D& scissor,
  void* extraPushConstants,
  std::size_t extraPushConstantsSize)
{
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, &_gigaMeshBuffer._buffer._buffer, offsets);
  vkCmdBindIndexBuffer(commandBuffer, _gigaMeshBuffer._buffer._buffer, 0, VK_INDEX_TYPE_UINT32);

  for (int i = 0; i <= MAX_MATERIAL_ID - 1; ++i) {
    if (_materials.find(i) == _materials.end()) continue;
    auto& material = _materials[i];
    if (shadowSupportRequired && !material._supportsShadowPass) continue;
    // TODO:
    if (material._supportsPostProcessingPass) continue;

    // Bind pipeline and start actual rendering
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, material._pipelines[materialIndex]);

    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    // Bind descriptor sets for UBOs
    auto& descriptorSets = material._descriptorSets[materialIndex][_currentFrame];
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, material._pipelineLayouts[materialIndex], 0, 1, &descriptorSets, 0, nullptr);

    for (auto& renderable: _currentRenderables) {
      // Move on to next material
      if (renderable._materialId != i) continue;
      if (!renderable._visible) continue;

      auto& mesh = _currentMeshes[renderable._meshId];

      // Bind specific push constants for this renderable
      if (renderable._pushConstantsSize > 0) {
        uint32_t pushSize = renderable._pushConstantsSize;
        if (extraPushConstants) {
          std::memcpy(&renderable._pushConstants[0] + renderable._pushConstantsSize, extraPushConstants, extraPushConstantsSize);
          pushSize += (uint32_t)extraPushConstantsSize;
        }
        vkCmdPushConstants(commandBuffer, material._pipelineLayouts[materialIndex], VK_SHADER_STAGE_VERTEX_BIT, 0, pushSize, &renderable._pushConstants[0]);
      }
      else if (extraPushConstants) {
        vkCmdPushConstants(commandBuffer, material._pipelineLayouts[materialIndex], VK_SHADER_STAGE_VERTEX_BIT, 0, (uint32_t)extraPushConstantsSize, extraPushConstants);
      }

      // Draw command
      vkCmdDrawIndexed(
        commandBuffer, 
        (uint32_t)mesh._numIndices,
        1,
        (uint32_t)mesh._indexOffset,
        (uint32_t)mesh._vertexOffset,
        0);
    }
  }
}

void VulkanRenderer::recordCommandBufferShadow(VkCommandBuffer commandBuffer, bool debug)
{
  for (auto& light: _lights) {
    auto shadowProj = light._proj;
    shadowProj[1][1] *= -1;
    for (std::size_t view = 0; view < light._shadowImageViews.size(); ++view) {
      imageutil::transitionImageLayout(commandBuffer, light._shadowImage._image, _shadowPass._shadowImageFormat,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, (uint32_t)view, 1);

      _shadowPass.beginRendering(commandBuffer, light._shadowImageViews[view]);
      
      auto shadowMatrix = shadowProj * light._view[view];
      drawRenderables(commandBuffer, Material::SHADOW_INDEX, true, _shadowPass.viewport(), _shadowPass.scissor(),
        (void*)&shadowMatrix, 4 * 4 * 4);

      vkCmdEndRendering(commandBuffer);

      // Transition shadow map image to shader readable
      imageutil::transitionImageLayout(commandBuffer, light._shadowImage._image, _shadowPass._shadowImageFormat,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, (uint32_t)view, 1);
    }
  }
}

void VulkanRenderer::recordCommandBufferGeometry(VkCommandBuffer commandBuffer, bool debug)
{
  auto viewport = _geometryPass.viewport(_swapChain._swapChainExtent.width, _swapChain._swapChainExtent.height);
  auto scissor = _geometryPass.scissor( _swapChain._swapChainExtent);

  drawRenderables(commandBuffer, Material::STANDARD_INDEX, false, viewport, scissor);
}

void VulkanRenderer::recordCommandBufferPP(VkCommandBuffer commandBuffer, std::uint32_t imageIndex, bool debug)
{
  /*Renderable* rend = nullptr;
  for (auto& renderable : _currentRenderables) {
    if (renderable._id == _ppPass._quadModelId) {
      rend = &renderable;
      break;
    }
  }

  auto viewport = _ppPass.viewport(_swapChain._swapChainExtent.width, _swapChain._swapChainExtent.height);
  auto scissor = _ppPass.scissor(_swapChain._swapChainExtent);

  for (std::size_t i = 0; i < _ppMaterials.size(); ++i) {
    bool isLast = i == (_ppMaterials.size() - 1);
    std::size_t nextIndex = isLast? 0 : _ppMaterials[i+1].second;
  
    auto& material = _ppMaterials[i];

    // Transition ppInput image to shader read
    imageutil::transitionImageLayout(commandBuffer, _ppPass._ppResources[material.second]._ppInputImage._image, _ppPass._ppImageFormat,
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // Transition the image we render to to color attachment (need to do before start rendering)
    if (!isLast) {
      imageutil::transitionImageLayout(commandBuffer, _ppPass._ppResources[nextIndex]._ppInputImage._image, _ppPass._ppImageFormat,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    }

    if (isLast) {
      _ppPass.beginRendering(commandBuffer, _swapChain._swapChainImageViews[imageIndex], _swapChain._swapChainExtent);
    }
    else {
      _ppPass.beginRendering(commandBuffer, _ppPass._ppResources[nextIndex]._ppInputImageView, _swapChain._swapChainExtent);
    }

    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    auto& descriptorSets = material.first._descriptorSets[Material::POST_PROCESSING_INDEX][_currentFrame];

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, material.first._pipelines[Material::POST_PROCESSING_INDEX]);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, material.first._pipelineLayouts[Material::POST_PROCESSING_INDEX], 0, 1, &descriptorSets, 0, nullptr);

    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &rend->_vertexBuffer._buffer, offsets);
    vkCmdDraw(commandBuffer, (uint32_t)rend->_numVertices, 1, 0, 0);

    if (!isLast) {
      vkCmdEndRendering(commandBuffer);
    }
  }*/
}

void VulkanRenderer::recordCommandBuffer(VkCommandBuffer commandBuffer, std::uint32_t imageIndex, bool pp, bool debug)
{
  pp = false;

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = 0; // Optional
  beginInfo.pInheritanceInfo = nullptr; // Optional

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    printf("failed to begin recording command buffer!\n");
    return;
  }

  // The swapchain image has to go from present layout to color attachment optimal
  imageutil::transitionImageLayout(commandBuffer,  _swapChain._swapChainImages[imageIndex], _swapChain._swapChainImageFormat,
                        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

  // Shadow pass
  //recordCommandBufferShadow(commandBuffer, debug);

  // Geometry pass
  // Transition the ppInput image to color attachment
  if (!_ppMaterials.empty() && pp) {
    imageutil::transitionImageLayout(commandBuffer, _ppPass._ppResources[0]._ppInputImage._image, _ppPass._ppImageFormat,
      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    _geometryPass.beginRendering(
      commandBuffer,
      _ppPass._ppResources[0]._ppInputImageView,
      _depthImageView,
      _swapChain._swapChainExtent);
    recordCommandBufferGeometry(commandBuffer, debug);
    vkCmdEndRendering(commandBuffer);

    // Post-processing pass
    recordCommandBufferPP(commandBuffer, imageIndex, debug);
  }
  else {
    // No post process effects, simply do the geometry pass
    _geometryPass.beginRendering(
      commandBuffer,
      _swapChain._swapChainImageViews[imageIndex],
      _depthImageView,
      _swapChain._swapChainExtent);
    recordCommandBufferGeometry(commandBuffer, debug);
  }

  // TODO: Vertex offset for rendering the quad...
  /*if (debug) {
    // Shadow map overlay
    // TODO: Stack debug texture overlays, not just shadow map
    Renderable* rend = nullptr;
    for (auto& renderable : _currentRenderables) {
      if (renderable._id == _shadowPass._debugModelId) {
        rend = &renderable;
        break;
      }
    }

    // Hijack begin render from geometry pass...
    auto& material = _materials[SHADOW_DEBUG_MATERIAL_ID];
    auto& descriptorSets = material._descriptorSets[Material::STANDARD_INDEX][_currentFrame];
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, material._pipelines[Material::STANDARD_INDEX]);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, material._pipelineLayouts[Material::STANDARD_INDEX], 0, 1, &descriptorSets, 0, nullptr);
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &rend->_vertexBuffer._buffer, offsets);
    vkCmdDraw(commandBuffer, (uint32_t)rend->_numVertices, 1, 0, 0);
  }*/

  // Imgui, hijack geometrypass begin...
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

  vkCmdEndRendering(commandBuffer);

  // Swapchain image has to go from color attachment to present.
  imageutil::transitionImageLayout(commandBuffer, _swapChain._swapChainImages[imageIndex], _swapChain._swapChainImageFormat,
                        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    printf("failed to record command buffer!\n");
  }
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

bool VulkanRenderer::initShadowpass()
{
  auto cmdBuffer = beginSingleTimeCommands();
  _shadowPass.createShadowResources(_device, _vmaAllocator, cmdBuffer, findDepthFormat(), 1024, 1024);
  endSingleTimeCommands(cmdBuffer);

  return true;
}

bool VulkanRenderer::initLights()
{
  _lights.resize(4);

  auto cmdBuffer = beginSingleTimeCommands();
  auto depthFormat = findDepthFormat();
  for (std::size_t i = 0; i < _lights.size(); ++i) {
    auto& light = _lights[i];
    float zNear = 0.1f;
    float zFar = 50.0f;

    light._type = Light::Point;
    light._color = glm::vec3(1.0f, 1.0f * i / (_lights.size() - 1), 0.0f);
    light._pos = glm::vec3(1.0f * i * 3, 5.0f, 0.0f);
    light._proj = glm::perspective(glm::radians(90.0f), 1.0f, zNear, zFar);
    light._debugCircleRadius = 5.0 + 15.0 * (double)i;
    light._debugOrigX = light._pos.x;
    light._debugOrigZ = light._pos.z;

    // Construct view matrices
    light.updateViewMatrices();

    // Construct the image
    imageutil::createImage(
      _shadowPass._shadowExtent.width, _shadowPass._shadowExtent.height,
      depthFormat, VK_IMAGE_TILING_OPTIMAL, _vmaAllocator,
      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, light._shadowImage,
      6, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT);

    // Create views
    light._cubeShadowView = imageutil::createImageView(_device, light._shadowImage._image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 0, 6, VK_IMAGE_VIEW_TYPE_CUBE);

    for (std::size_t view = 0; view < 6; ++view) {
      light._shadowImageViews.emplace_back(
        imageutil::createImageView(
          _device, light._shadowImage._image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, (uint32_t)view, 1));

      // Transition image
      imageutil::transitionImageLayout(cmdBuffer, light._shadowImage._image, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, (uint32_t)view, 1);
    }

    // Create the sampler
    VkSamplerCreateInfo sampler{};
    sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler.magFilter = VK_FILTER_NEAREST;
    sampler.minFilter = VK_FILTER_NEAREST;
    sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sampler.addressModeV = sampler.addressModeU;
    sampler.addressModeW = sampler.addressModeU;
    sampler.mipLodBias = 0.0f;
    sampler.maxAnisotropy = 1.0f;
    sampler.compareOp = VK_COMPARE_OP_NEVER;
    sampler.minLod = 0.0f;
    sampler.maxLod = 1.0f;
    sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

    if (vkCreateSampler(_device, &sampler, nullptr, &light._sampler) != VK_SUCCESS) {
      printf("Could not create shadow map sampler!\n");
      return false;
    }
  }
  endSingleTimeCommands(cmdBuffer);

  return true;
}

bool VulkanRenderer::initShadowDebug()
{
  auto meshId = registerMesh(_shadowPass._debugModel._vertices, {});
  _shadowPass._debugModelId = registerRenderable(meshId, STANDARD_MATERIAL_ID, glm::mat4(1.0f), glm::vec3(1.0f), 0.0f);
  setRenderableVisible(_shadowPass._debugModelId, false);

  return true;
}

bool VulkanRenderer::initPostProcessingRenderable()
{
  auto meshId = registerMesh(_ppPass._quadModel._vertices, {});
  _ppPass._quadModelId = registerRenderable(meshId, STANDARD_MATERIAL_ID, glm::mat4(1.0f), glm::vec3(1.0f), 0.0f);
  setRenderableVisible(_ppPass._quadModelId, false);

  return true;
}

bool VulkanRenderer::initPostProcessingPass()
{
  _ppPass.init(_swapChain._swapChainImageFormat, _swapChain._swapChainExtent.width, _swapChain._swapChainExtent.height);

  return true;
}

bool VulkanRenderer::initGigaMeshBuffer()
{
  // Allocate "big enough" size... 
  bufferutil::createBuffer(_vmaAllocator, _gigaMeshBuffer._size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 0, _gigaMeshBuffer._buffer);
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
  ImGuiIO& io = ImGui::GetIO(); (void)io;

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
  init_info.ColorAttachmentFormat = _swapChain._swapChainImageFormat;

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
  recordCommandBuffer(_commandBuffers[_currentFrame], imageIndex, applyPostProcessing, debug);

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

  if (vkQueueSubmit(_graphicsQ, 1, &submitInfo, _inFlightFences[_currentFrame]) != VK_SUCCESS) {
    printf("failed to submit draw command buffer!\n");
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
  
  // Advance frame
  _currentFrame = (_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanRenderer::notifyFramebufferResized()
{
  _framebufferResized = true;
}
}