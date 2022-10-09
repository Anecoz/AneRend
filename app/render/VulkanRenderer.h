#pragma once

#define NOMINMAX 1

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <glm/glm.hpp>

#include "vk_mem_alloc.h"

#include "Vertex.h"
#include "MaterialId.h"
#include "Material.h"
#include "Camera.h"
#include "AllocatedBuffer.h"
#include "AllocatedImage.h"
#include "Swapchain.h"
#include "Shadowpass.h"

#include <array>
#include <unordered_map>
#include <optional>
#include <vector>

namespace render {

typedef std::int64_t RenderableId;

class VulkanRenderer 
{
public:
  VulkanRenderer(GLFWwindow* window);
  ~VulkanRenderer();

  VulkanRenderer(const VulkanRenderer&) = delete;
  VulkanRenderer(VulkanRenderer&&) = delete;
  VulkanRenderer& operator=(const VulkanRenderer&) = delete;
  VulkanRenderer& operator=(VulkanRenderer&&) = delete;

  // Initializes surface, device, all material pipelines etc.
  bool init();

  // After doing this, the renderable will be rendered every frame with the given material, until unregistered.
  RenderableId registerRenderable(
    const std::vector<Vertex>& vertices,
    const std::vector<std::uint32_t>& indices,
    MaterialID materialId,
    std::size_t instanceCount = 0,
    std::vector<std::uint8_t> instanceData = {});

  // Completely removes all data related to this id and will stop rendering it.
  void unregisterRenderable(RenderableId id);

  void setRenderableVisible(RenderableId id, bool visible);

  // Queues push constant data to be used with the specific renderable next frame.
  // Typically per-object type stuff here, such as model matrix.
  // The material for the renderable has to be compatible with the push constant.
  // These will be used every frame until something else is specified.
  // NOTE: There is a size limit to push constants... good luck!
  void queuePushConstant(RenderableId id, std::uint32_t size, void* pushConstants);

  void update(const Camera& camera, const Camera& shadowCamera, double delta);

  // Prepares some things for drawing (imgui stuff as of now).
  // Has to be called before drawFrame()!
  void prepare();

  // Goes through all registered renderables, updates any resource descriptor sets (like UBOs),
  // updates push constants, renders and queues for presentation.
  void drawFrame(bool debug);

  // Will recreate swapchain to accomodate the new window size.
  void notifyFramebufferResized();

private:
  RenderableId _nextRenderableId = 1;

  struct Renderable {
    RenderableId _id;

    MaterialID _materialId;

    bool _visible = true;

    AllocatedBuffer _vertexBuffer;
    AllocatedBuffer _indexBuffer;

    AllocatedBuffer _instanceData;

    std::size_t _numIndices;
    std::size_t _numVertices;
    std::size_t _numInstances = 1;

    std::array<std::uint8_t, 128> _pushConstants;
    std::uint32_t _pushConstantsSize = 0;
  };

  std::vector<Renderable> _currentRenderables;
  void cleanupRenderable(const Renderable& renderable);

  std::unordered_map<MaterialID, Material> _materials;
  std::unordered_map<MaterialID, std::vector<VkDescriptorSet>> _materialDescriptorSets;
  std::unordered_map<MaterialID, std::vector<VkDescriptorSet>> _materialDescriptorSetsShadow;
  bool materialIdExists(MaterialID) const;

  struct StandardUBO {
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 shadowMatrix;
    glm::vec4 cameraPos;
  };

  struct ShadowUBO {
    glm::mat4 shadowMatrix;
  };

  Shadowpass _shadowpass;

  VmaAllocator _vmaAllocator;

  bool createInstance();
  bool setupDebugMessenger();
  bool pickPhysicalDevice();
  bool createLogicalDevice();
  bool createVmaAllocator();
  bool createSurface();
  bool createSwapChain();
  bool loadKnownMaterials();
  bool createCommandPool();
  bool createCommandBuffers();
  bool createSyncObjects();
  bool createUniformBuffers();
  bool createDescriptorPool();
  bool createDepthResources();
  bool initShadowpass();
  bool initShadowDebug();
  bool initImgui();

  bool checkValidationLayerSupport();
  bool checkDeviceExtensionSupport(VkPhysicalDevice device);
  void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

  void cleanupSwapChain();
  void recreateSwapChain();

  bool createIndexBuffer(const std::vector<std::uint32_t>& indices, AllocatedBuffer& buffer);
  bool createVertexBuffer(std::uint8_t* data, std::size_t dataSize, AllocatedBuffer& buffer);
  void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags properties, AllocatedBuffer& buffer);
  void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

  void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

  VkCommandBuffer beginSingleTimeCommands();
  void endSingleTimeCommands(VkCommandBuffer buffer);

  std::vector<const char*> getRequiredExtensions();

  bool isDeviceSuitable(VkPhysicalDevice device);
  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

  VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
  VkFormat findDepthFormat();

  void updateStandardUBO(std::uint32_t currentImage, const Camera& camera, const Camera& shadowCamera);
  void updateShadowUBO(std::uint32_t currentImage,const Camera& shadowCamera);

  void recordCommandBuffer(VkCommandBuffer commandBuffer, std::uint32_t imageIndex, bool debug);
  void recordCommandBufferShadow(VkCommandBuffer commandBuffer, bool debug);

  GLFWwindow* _window;
  bool _enableValidationLayers;

  VkInstance _instance;
  VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
  VkDevice _device;

  VkQueue _graphicsQ;
  VkQueue _presentQ;

  VkDebugUtilsMessengerEXT _debugMessenger;

  VkSurfaceKHR _surface;
  Swapchain _swapChain;

  AllocatedImage _depthImage;
  VkImageView _depthImageView;

  bool _framebufferResized = false;

  VkDescriptorPool _descriptorPool;
  VkDescriptorPool _imguiDescriptorPool;

  std::vector<AllocatedBuffer> _standardUBOs;
  std::vector<AllocatedBuffer> _shadowUBOs;

  std::vector<VkCommandBuffer> _commandBuffers;
  VkCommandPool _commandPool;

  std::vector<VkSemaphore> _imageAvailableSemaphores;
  std::vector<VkSemaphore> _renderFinishedSemaphores;
  std::vector<VkFence> _inFlightFences;

  std::vector<const char*> _validationLayers = {
    "VK_LAYER_KHRONOS_validation"
  };

  const std::vector<const char*> _deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME
  };

  const int MAX_FRAMES_IN_FLIGHT = 2;
  std::uint32_t _currentFrame = 0;
};
}