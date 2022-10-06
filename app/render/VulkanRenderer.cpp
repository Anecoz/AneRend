#include "VulkanRenderer.h"

#include "MaterialFactory.h"
#include "QueueFamilyIndices.h"
#include "ImageHelpers.h"

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
    printf("validation layer: %s\n", pCallbackData->pMessage);
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

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vmaDestroyBuffer(_vmaAllocator, _uniformBuffers[i]._buffer, _uniformBuffers[i]._allocation);
  }

  vkDestroyDescriptorPool(_device, _descriptorPool, nullptr);
  vkDestroyDescriptorPool(_device, _imguiDescriptorPool, nullptr);

  ImGui_ImplVulkan_Shutdown();

  for (const auto& material: _materials) {
    vkDestroyDescriptorSetLayout(_device, material.second._descriptorSetLayout, nullptr);
    vkDestroyPipeline(_device, material.second._pipeline, nullptr);
    vkDestroyPipelineLayout(_device, material.second._pipelineLayout, nullptr);
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

  printf("Creating uniform buffers...");
  res &= createUniformBuffers();
  if (!res) return false;
  printf("Done!\n");

  printf("Creating descriptor pool...");
  res &= createDescriptorPool();
  if (!res) return false;
  printf("Done!\n");

  printf("Loading known materials...");
  res &= loadKnownMaterials();
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

  printf("Init imgui...");
  res &= initImgui();
  if (!res) return false;
  printf("Done!\n");

  return res;
}

RenderableId VulkanRenderer::registerRenderable(
  const std::vector<Vertex>& vertices,
  const std::vector<std::uint32_t>& indices,
  MaterialID materialId,
  std::size_t instanceCount,
  std::vector<std::uint8_t> instanceData)
{
  if (!materialIdExists(materialId)) {
    printf("Material id %zu does not exist!\n", materialId);
    return 0;
  }

  Renderable renderable{};

  std::uint8_t* ptr = (std::uint8_t*)vertices.data();
  if (!createVertexBuffer(ptr, sizeof(Vertex) * vertices.size(), renderable._vertexBuffer)) {
    printf("Could not create vertex buffer!\n");
    return 0;
  }

  if (!createIndexBuffer(indices, renderable._indexBuffer)) {
    printf("Could not create index buffer!\n");
    return 0;
  }

  auto id = _nextRenderableId++;

  renderable._id = id;
  renderable._materialId = materialId;
  renderable._numIndices = indices.size();
  renderable._numInstances = 0;

  if (instanceCount != 0) {
    std::uint8_t* instancePtr = instanceData.data();
    if (!createVertexBuffer(instancePtr, instanceData.size(), renderable._instanceData)) {
      printf("Could not create instance buffer!\n");
      return 0;
    }
    renderable._numInstances = instanceCount;
  }

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

void VulkanRenderer::cleanupRenderable(const Renderable& renderable)
{
  vmaDestroyBuffer(_vmaAllocator, renderable._vertexBuffer._buffer, renderable._vertexBuffer._allocation);
  vmaDestroyBuffer(_vmaAllocator, renderable._indexBuffer._buffer, renderable._indexBuffer._allocation);
  if (renderable._numInstances > 0) {
    vmaDestroyBuffer(_vmaAllocator, renderable._instanceData._buffer, renderable._instanceData._allocation);
  }
}

void VulkanRenderer::queuePushConstant(RenderableId id, std::uint32_t size, void* pushConstants)
{
  // Sanity check, does the material of this renderable support push constants?
  // Also, does it even exist?
  bool found = false;
  for (auto& renderable: _currentRenderables) {
    if (renderable._id == id) {
      found = true;
      if (!_materials[renderable._materialId]._supportsPushConstants) {
        printf("Could not queue push constant, because material doesn't support it\n");
      }
    }
  }

  if (!found) {
    printf("Could not queue push constant, because the renderable doesn't exist\n");
    return;
  }

  PushConstantQueueEntry entry;

  entry._renderableId = id;
  entry._size = size;
  memcpy(&entry._pushConstants[0], pushConstants, size);

  _pushQueue.emplace_back(std::move(entry));
}

void VulkanRenderer::update(const Camera& camera, double delta)
{
  // Update UBO
  auto camPos = camera.getPosition();
  updateUniformBuffer(_currentFrame, glm::vec4(camPos.x, camPos.y, camPos.z, 0.0f), camera.getCamMatrix(), camera.getProjection());
}

bool VulkanRenderer::materialIdExists(MaterialID id) const
{
  return _materials.find(id) != _materials.end();
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

bool VulkanRenderer::hasStencilComponent(VkFormat format)
{
  return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
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

void VulkanRenderer::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags properties, AllocatedBuffer& buffer)
{
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo vmaAllocInfo{};
  vmaAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
  vmaAllocInfo.flags = properties;

  if (vmaCreateBuffer(_vmaAllocator, &bufferInfo, &vmaAllocInfo,
              &buffer._buffer,
              &buffer._allocation,
              nullptr) != VK_SUCCESS) {
     printf("Could not create vma buffer!\n");
  }
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
  // Standard material
  auto standardMaterial = MaterialFactory::createStandardMaterial(_device, _swapChain._swapChainImageFormat, findDepthFormat());
  if (!standardMaterial) {
    printf("Could not create standard material!\n");
    return false;
  }

  _materials[STANDARD_MATERIAL_ID] = std::move(standardMaterial);

  // Standard instanced material
  auto standardInstancedMaterial = MaterialFactory::createStandardInstancedMaterial(_device, _swapChain._swapChainImageFormat, findDepthFormat());
  if (!standardInstancedMaterial) {
    printf("Could not create standard instanced material!\n");
    return false;
  }
  _materials[STANDARD_INSTANCED_MATERIAL_ID] = std::move(standardInstancedMaterial);

  std::vector<VkDescriptorSet> descriptorSets;

  // Have to create a descriptor set (times num frames in flight) for the generic material UBO
  std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, _materials[STANDARD_MATERIAL_ID]._descriptorSetLayout);
  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = _descriptorPool;
  allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
  allocInfo.pSetLayouts = layouts.data();

  descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
  if (vkAllocateDescriptorSets(_device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
    printf("failed to allocate descriptor sets!\n");
    return false;
  }

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = _uniformBuffers[i]._buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    std::array<VkWriteDescriptorSet, 1> descriptorWrites{};

    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = descriptorSets[i];
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
  }
  _materialDescriptorSets[STANDARD_MATERIAL_ID] = std::move(descriptorSets);

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

  createImage(_swapChain._swapChainExtent.width, _swapChain._swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL,
    _vmaAllocator, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _depthImage);
  _depthImageView = createImageView(_device, _depthImage._image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

  transitionImageLayout(_depthImage._image, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

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

void VulkanRenderer::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  VkPipelineStageFlags sourceStage;
  VkPipelineStageFlags destinationStage;

  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  }
  else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  }
  else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  }
  else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
    barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    barrier.dstAccessMask = 0;
    sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  }
  else {
    printf("unsupported layout transition!\n");
  }

  if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

    if (hasStencilComponent(format)) {
      barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
  }
  else {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  }

  vkCmdPipelineBarrier(
    commandBuffer,
    sourceStage, destinationStage,
    0,
    0, nullptr,
    0, nullptr,
    1, &barrier
  );

  endSingleTimeCommands(commandBuffer);
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
  createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, stagingBuffer);

  void* mappedData;
  vmaMapMemory(_vmaAllocator, stagingBuffer._allocation, &mappedData);
  memcpy(mappedData, data, (size_t) bufferSize);
  vmaUnmapMemory(_vmaAllocator, stagingBuffer._allocation);

  // Device local buffer, optimised for GPU
  createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 0, buffer);

  copyBuffer(stagingBuffer._buffer, buffer._buffer, bufferSize);

  vmaDestroyBuffer(_vmaAllocator, stagingBuffer._buffer, stagingBuffer._allocation);

  return true;
}

bool VulkanRenderer::createIndexBuffer(const std::vector<std::uint32_t>& indices, AllocatedBuffer& buffer)
{
  VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

  AllocatedBuffer stagingBuffer;
  createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, stagingBuffer);

  void* data;
  vmaMapMemory(_vmaAllocator, stagingBuffer._allocation, &data);
  memcpy(data, indices.data(), (size_t) bufferSize);
  vmaUnmapMemory(_vmaAllocator, stagingBuffer._allocation);

  createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 0, buffer);

  copyBuffer(stagingBuffer._buffer, buffer._buffer, bufferSize);

  vmaDestroyBuffer(_vmaAllocator, stagingBuffer._buffer, stagingBuffer._allocation);

  return true;
}

bool VulkanRenderer::createUniformBuffers()
{
  VkDeviceSize bufferSize = sizeof(UniformBufferObject);

  _uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, _uniformBuffers[i]);
  }

  return true;
}

bool VulkanRenderer::createDescriptorPool()
{
  std::array<VkDescriptorPoolSize, 1> poolSizes{};
  poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
  //poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  //poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

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

void VulkanRenderer::updateUniformBuffer(std::uint32_t currentImage, const glm::vec4& cameraPos, const glm::mat4& view, const glm::mat4& projection)
{
  UniformBufferObject ubo{};
  ubo.view = view;
  ubo.proj = projection;
  ubo.cameraPos = cameraPos;

  // GLM was originally designed for OpenGL, where the Y coordinate of the clip coordinates is inverted.
  // The easiest way to compensate for that is to flip the sign on the scaling factor of the Y axis in the projection matrix. 
  // If you don't do this, then the image will be rendered upside down.
  ubo.proj[1][1] *= -1;

  void* data;
  vmaMapMemory(_vmaAllocator, _uniformBuffers[currentImage]._allocation, &data);
  memcpy(data, &ubo, sizeof(ubo));
  vmaUnmapMemory(_vmaAllocator, _uniformBuffers[currentImage]._allocation);
}

void VulkanRenderer::recordCommandBuffer(VkCommandBuffer commandBuffer, std::uint32_t imageIndex)
{
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = 0; // Optional
  beginInfo.pInheritanceInfo = nullptr; // Optional

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    printf("failed to begin recording command buffer!\n");
    return;
  }

  // The swapchain image has to go from present layout to color attachment optimal
  // TODO: Helper for transition
  VkImageMemoryBarrier preImageMemBarrier{};
  preImageMemBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  preImageMemBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  preImageMemBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  preImageMemBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  preImageMemBarrier.image = _swapChain._swapChainImages[imageIndex];
  preImageMemBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  preImageMemBarrier.subresourceRange.baseMipLevel = 0;
  preImageMemBarrier.subresourceRange.levelCount = 1;
  preImageMemBarrier.subresourceRange.baseArrayLayer = 0;
  preImageMemBarrier.subresourceRange.layerCount = 1;

  vkCmdPipelineBarrier(
    commandBuffer,
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,  // srcStageMask
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // dstStageMask
    0,
    0,
    nullptr,
    0,
    nullptr,
    1, // imageMemoryBarrierCount
    &preImageMemBarrier // pImageMemoryBarriers
  );

  // Dynamic rendering
  // TODO: Helper for starting this "pass". Want to add depth-only pass soon
  std::array<VkClearValue, 2> clearValues{};
  clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
  clearValues[1].depthStencil = {1.0f, 0};
  VkRenderingAttachmentInfoKHR colorAttachmentInfo{};
  colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
  colorAttachmentInfo.imageView = _swapChain._swapChainImageViews[imageIndex];
  colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
  colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachmentInfo.clearValue = clearValues[0];

  VkRenderingAttachmentInfoKHR depthAttachmentInfo{};
  depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
  depthAttachmentInfo.imageView = _depthImageView;
  depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
  depthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  depthAttachmentInfo.clearValue = clearValues[1];

  VkRenderingInfoKHR renderInfo{};
  renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
  renderInfo.renderArea.extent = _swapChain._swapChainExtent;
  renderInfo.renderArea.offset = {0, 0};
  renderInfo.layerCount = 1;
  renderInfo.colorAttachmentCount = 1;
  renderInfo.pColorAttachments = &colorAttachmentInfo;
  renderInfo.pDepthAttachment = &depthAttachmentInfo;

  vkCmdBeginRendering(commandBuffer, &renderInfo);

  // Since we used dynamic viewport and scissor setup earlier, we have to specify them now
  // TODO: This should be part of the "pass" I guess, shadow pass for instance wants w and h to be that of the shadow map
  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(_swapChain._swapChainExtent.width);
  viewport.height = static_cast<float>(_swapChain._swapChainExtent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = _swapChain._swapChainExtent;
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

  // TODO: We've made sure that the vector isn't empty, so this is safe, but should probably be handled some other way
  // TODO: Break this out to own function? We want to do a shadowpass for instance, need to call all of this again basically.
  // Renderables are sorted in material order, standard first
  for (int i = STANDARD_MATERIAL_ID; i <= STANDARD_INSTANCED_MATERIAL_ID; ++i) {
    auto& material = _materials[i];

    // Bind pipeline and start actual rendering
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, material._pipeline);

    // Bind descriptor sets for UBOs
    auto& descriptorSets = _materialDescriptorSets[_currentRenderables[0]._materialId];
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, material._pipelineLayout, 0, 1, &descriptorSets[_currentFrame], 0, nullptr);

    for (auto& renderable: _currentRenderables) {
      // Move on to next material
      if (renderable._materialId != i) continue;

      // NOTE:  Driver developers recommend that you also store multiple buffers, like the vertex and index buffer, 
      //        into a single VkBuffer and use offsets in commands like vkCmdBindVertexBuffers. (cache friendlier)
      VkDeviceSize offsets[] = {0};
      vkCmdBindVertexBuffers(commandBuffer, 0, 1, &renderable._vertexBuffer._buffer, offsets);
      if (material._hasInstanceBuffer) {
        vkCmdBindVertexBuffers(commandBuffer, 1, 1, &renderable._instanceData._buffer, offsets);
      }
      vkCmdBindIndexBuffer(commandBuffer, renderable._indexBuffer._buffer, 0, VK_INDEX_TYPE_UINT32);

      // Bind specific push constants for this renderable
      // TODO: This should be pre-sorted somehow
      for (auto it = _pushQueue.begin(); it != _pushQueue.end(); ++it) {
        if (it->_renderableId == renderable._id) {
          vkCmdPushConstants(commandBuffer, material._pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, it->_size, &(it->_pushConstants[0]));
          _pushQueue.erase(it);
          break;
        }
      }

      // Draw command
      if (material._hasInstanceBuffer) {
        vkCmdDrawIndexed(commandBuffer, static_cast<std::uint32_t>(renderable._numIndices), renderable._numInstances, 0, 0, 0);
      }
      else {
        vkCmdDrawIndexed(commandBuffer, static_cast<std::uint32_t>(renderable._numIndices), 1, 0, 0, 0);
      }
    }
  }

  // Imgui
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

  // End
  vkCmdEndRendering(commandBuffer);

  // Layout transition of the swapchain image has to be done here.
  // TODO: Helper for image transition
  VkImageMemoryBarrier postImageMemBarrier{};
  postImageMemBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  postImageMemBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  postImageMemBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  postImageMemBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  postImageMemBarrier.image = _swapChain._swapChainImages[imageIndex];
  postImageMemBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  postImageMemBarrier.subresourceRange.baseMipLevel = 0;
  postImageMemBarrier.subresourceRange.levelCount = 1;
  postImageMemBarrier.subresourceRange.baseArrayLayer = 0;
  postImageMemBarrier.subresourceRange.layerCount = 1;

  vkCmdPipelineBarrier(
    commandBuffer,
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,  // srcStageMask
    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, // dstStageMask
    0,
    0,
    nullptr,
    0,
    nullptr,
    1, // imageMemoryBarrierCount
    &postImageMemBarrier // pImageMemoryBarriers
  );

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

void VulkanRenderer::drawFrame()
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
  recordCommandBuffer(_commandBuffers[_currentFrame], imageIndex);

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