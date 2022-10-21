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
#include "Geometrypass.h"
#include "PostProcessingPass.h"
#include "Light.h"

#include <array>
#include <unordered_map>
#include <optional>
#include <vector>
#include <tuple>

namespace render {

typedef std::int64_t RenderableId;
typedef std::int64_t MeshId;

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

  // Registers a mesh that a renderable can reference.
  MeshId registerMesh(
    const std::vector<Vertex>& vertices,
    const std::vector<std::uint32_t>& indices);

  // After doing this, the renderable will be rendered every frame with the given material, until unregistered.
  RenderableId registerRenderable(
    MeshId meshId,
    MaterialID materialId,
    const glm::mat4& transform,
    const glm::vec3& sphereBoundCenter,
    float sphereBoundRadius);

  // Completely removes all data related to this id and will stop rendering it.
  void unregisterRenderable(RenderableId id);

  // Hide or show a renderable.
  void setRenderableVisible(RenderableId id, bool visible);

  // Queues push constant data to be used with the specific renderable next frame.
  // Typically per-object type stuff here, such as model matrix.
  // The material for the renderable has to be compatible with the push constant.
  // These will be used every frame until something else is specified.
  // NOTE: There is a size limit to push constants... good luck!
  void queuePushConstant(RenderableId id, std::uint32_t size, void* pushConstants);

  void update(const Camera& camera, const Camera& shadowCamera, const glm::vec4& lightDir, double delta);

  // Prepares some things for drawing (imgui stuff as of now).
  // Has to be called before drawFrame()!
  void prepare();

  // Goes through all registered renderables, updates any resource descriptor sets (like UBOs),
  // updates push constants, renders and queues for presentation.
  void drawFrame(bool applyPostProcessing, bool debug);

  // Will recreate swapchain to accomodate the new window size.
  void notifyFramebufferResized();

private:
  RenderableId _nextRenderableId = 1;
  MeshId _nextMeshId = 0;

  struct Mesh {
    MeshId _id;

    // Offsets into the fat mesh buffer
    std::size_t _vertexOffset;
    std::size_t _indexOffset;

    std::size_t _numVertices;
    std::size_t _numIndices;
  };

  bool meshIdExists(MeshId meshId) const;
  std::vector<Mesh> _currentMeshes;

  struct Renderable {
    RenderableId _id;

    MeshId _meshId;
    MaterialID _materialId;

    bool _visible = true;

    glm::mat4 _transform;
    glm::vec3 _boundingSphereCenter;
    float _boundingSphereRadius;

    std::array<std::uint8_t, 128> _pushConstants;
    std::uint32_t _pushConstantsSize = 0;
  };

  std::vector<Renderable> _currentRenderables;
  void cleanupRenderable(const Renderable& renderable);

  std::unordered_map<MaterialID, Material> _materials;
  std::vector<std::pair<Material, std::size_t>> _ppMaterials; //size_t is index into pp render pass resources
  bool materialIdExists(MaterialID) const;

  // Fat mesh buffer that contains both vertex and index data of all currently registered meshes
  static const std::size_t GIGA_MESH_BUFFER_SIZE_MB = 256;
  struct GigaMeshBuffer {
    AllocatedBuffer _buffer;

    std::size_t _size = 1024 * 1024 * GIGA_MESH_BUFFER_SIZE_MB; // in bytes

    std::size_t _freeSpacePointer = 0; // points to where we currently can insert things into the buffer
  } _gigaMeshBuffer;

  void drawRenderables(
    VkCommandBuffer& commandBuffer,
    std::size_t materialIndex,
    bool shadowSupportRequired,
    VkViewport& viewport,
    VkRect2D& scissor,
    void* extraPushConstants = nullptr,
    std::size_t extraPushConstantsSize = 0);

  Shadowpass _shadowPass;
  Geometrypass _geometryPass;
  PostProcessingPass _ppPass;

  std::vector<Light> _lights;

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
  bool createDescriptorPool();
  bool createDepthResources();
  bool initShadowpass();
  bool initLights();
  bool initShadowDebug();
  bool initPostProcessingPass();
  bool initPostProcessingRenderable();
  bool initImgui();
  bool initGigaMeshBuffer();

  bool checkValidationLayerSupport();
  bool checkDeviceExtensionSupport(VkPhysicalDevice device);
  void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

  void cleanupSwapChain();
  void recreateSwapChain();

  bool createIndexBuffer(const std::vector<std::uint32_t>& indices, AllocatedBuffer& buffer);
  bool createVertexBuffer(std::uint8_t* data, std::size_t dataSize, AllocatedBuffer& buffer);
  void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

  void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

  VkCommandBuffer beginSingleTimeCommands();
  void endSingleTimeCommands(VkCommandBuffer buffer);

  std::vector<const char*> getRequiredExtensions();

  bool isDeviceSuitable(VkPhysicalDevice device);
  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

  VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
  VkFormat findDepthFormat();

  void recordCommandBuffer(VkCommandBuffer commandBuffer, std::uint32_t imageIndex, bool pp, bool debug);
  void recordCommandBufferShadow(VkCommandBuffer commandBuffer, bool debug);
  void recordCommandBufferGeometry(VkCommandBuffer commandBuffer, bool debug);
  void recordCommandBufferPP(VkCommandBuffer commandBuffer, std::uint32_t imageIndex, bool debug);

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