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
#include "MeshId.h"
#include "Camera.h"
#include "AllocatedBuffer.h"
#include "AllocatedImage.h"
#include "Swapchain.h"
#include "Light.h"
#include "RenderContext.h"
#include "RenderPass.h"
#include "RenderResourceVault.h"
#include "FrameGraphBuilder.h"
#include "Mesh.h"
#include "RenderableId.h"

#include "../logic/WindSystem.h"

#include <array>
#include <unordered_map>
#include <optional>
#include <vector>
#include <tuple>

namespace render {

class VulkanRenderer : public RenderContext
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
    const glm::mat4& transform,
    const glm::vec3& sphereBoundCenter,
    float sphereBoundRadius);

  // Completely removes all data related to this id and will stop rendering it.
  void unregisterRenderable(RenderableId id);

  // Hide or show a renderable.
  void setRenderableVisible(RenderableId id, bool visible);

  void update(
    const Camera& camera,
    const Camera& shadowCamera,
    const glm::vec4& lightDir,
    double delta,
    double time,
    bool lockCulling,
    RenderDebugOptions options,
    logic::WindMap windMap);

  // Prepares some things for drawing (imgui stuff as of now).
  // Has to be called before drawFrame()!
  void prepare();

  // Goes through all registered renderables, updates any resource descriptor sets (like UBOs),
  // updates push constants, renders and queues for presentation.
  void drawFrame(bool applyPostProcessing, bool debug);

  // Will recreate swapchain to accomodate the new window size.
  void notifyFramebufferResized();

  // Render Context interface
  VkDevice& device() override final;
  VkDescriptorPool& descriptorPool() override final;
  VmaAllocator vmaAllocator() override final;
  
  VkPipelineLayout& bindlessPipelineLayout() override final;
  VkDescriptorSetLayout& bindlessDescriptorSetLayout() override final;
  VkExtent2D swapChainExtent() override final;

  void drawGigaBuffer(VkCommandBuffer* commandBuffer) override final;
  void drawGigaBufferIndirect(VkCommandBuffer*, VkBuffer drawCalls) override final;
  void drawNonIndexIndirect(VkCommandBuffer*, VkBuffer drawCalls, uint32_t drawCount, uint32_t stride) override final;
  void drawMeshId(VkCommandBuffer*, MeshId, uint32_t vertCount, uint32_t instanceCount) override final;

  VkImage& getCurrentSwapImage() override final;
  int getCurrentMultiBufferIdx() override final;
  int getMultiBufferSize() override final;

  size_t getMaxNumMeshes() override final;
  size_t getMaxNumRenderables() override final;

  std::vector<Mesh>& getCurrentMeshes() override final;
  std::unordered_map<MeshId, std::size_t>& getCurrentMeshUsage() override final;
  size_t getCurrentNumRenderables() override final;

  gpu::GPUCullPushConstants getCullParams() override final;

  RenderDebugOptions& getDebugOptions() override final;

  void setDebugName(VkObjectType objectType, uint64_t objectHandle, const char* name) override final;

  virtual glm::vec2 getWindDir() override final;

private:
  static const std::size_t MAX_FRAMES_IN_FLIGHT = 2;
  static const std::size_t MAX_PUSH_CONSTANT_SIZE = 128;
  static const std::size_t GIGA_MESH_BUFFER_SIZE_MB = 256;
  static const std::size_t MAX_NUM_RENDERABLES = std::size_t(1e5);
  static const std::size_t MAX_NUM_MESHES = std::size_t(1e3);

  RenderDebugOptions _debugOptions;
  logic::WindMap _currentWindMap;

  RenderableId _nextRenderableId = 1;
  MeshId _nextMeshId = 0;

  Camera _latestCamera;

  // This is dangerous, but points to "current" image index in the swap chain
  uint32_t _currentSwapChainIndex;

  bool meshIdExists(MeshId meshId) const;
  std::vector<Mesh> _currentMeshes;
  std::unordered_map<MeshId, std::size_t> _currentMeshUsage;

  struct Renderable {
    RenderableId _id;

    MeshId _meshId;

    bool _visible = true;

    glm::mat4 _transform;
    glm::vec3 _boundingSphereCenter;
    float _boundingSphereRadius;
  };

  std::vector<Renderable> _currentRenderables;
  void cleanupRenderable(const Renderable& renderable);

  /* 
  * "Bindless" descriptor set layout and pipeline layout, used for every render pass.
  * The render passes create their own pipelines, specifying these on creation (accessed via RenderContext).
  * Only one descriptor set gets bound before all rendering.
  */
  VkPipelineLayout _bindlessPipelineLayout;
  VkDescriptorSetLayout _bindlessDescSetLayout;
  std::vector<VkDescriptorSet> _bindlessDescriptorSets;

  RenderResourceVault _vault;
  FrameGraphBuilder _fgb;
  std::vector<RenderPass*> _renderPasses;

  void executeFrameGraph(VkCommandBuffer commandBuffer, int imageIndex);

  // Giga mesh buffer that contains both vertex and index data of all currently registered meshes
  struct GigaMeshBuffer {
    AllocatedBuffer _buffer;

    std::size_t _size = 1024 * 1024 * GIGA_MESH_BUFFER_SIZE_MB; // in bytes

    std::size_t _freeSpacePointer = 0; // points to where we currently can insert things into the buffer
  } _gigaMeshBuffer;

  // Staging buffer for copying data to the gpu buffers.
  std::vector<AllocatedBuffer> _gpuStagingBuffer;
  std::size_t _currentStagingOffset = 0;

  // Contains renderable info for compute culling shader.
  std::vector<AllocatedBuffer> _gpuRenderableBuffer;

  // Contains scene data needed in shaders (view and proj matrices etc.)
  std::vector<AllocatedBuffer> _gpuSceneDataBuffer;

  // Contains wind system data, accessed by various render passes.
  std::vector<AllocatedImage> _gpuWindForceImage;

  // View for the wind force images.
  std::vector<VkImageView> _gpuWindForceView;

  // Samplers for the wind force images.
  std::vector<VkSampler> _gpuWindForceSampler;

  // Fills gpu renderable buffer with current renderable information (could be done async)
  void prefillGPURenderableBuffer(VkCommandBuffer& commandBuffer);

  // Update wind force image
  void updateWindForceImage(VkCommandBuffer& commandBuffer);

  std::vector<Light> _lights;

  VmaAllocator _vmaAllocator;

  bool createInstance();
  bool setupDebugMessenger();
  bool pickPhysicalDevice();
  bool createLogicalDevice();
  bool createVmaAllocator();
  bool createSurface();
  bool createSwapChain();
  bool createCommandPool();
  bool createCommandBuffers();
  bool createSyncObjects();
  bool createDescriptorPool();
  bool initLights();
  bool initImgui();
  bool initGigaMeshBuffer();
  bool initGpuBuffers();
  bool initBindless();
  bool initFrameGraphBuilder();

  bool checkValidationLayerSupport();
  bool checkDeviceExtensionSupport(VkPhysicalDevice device);
  void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

  void cleanupSwapChain();
  void recreateSwapChain();

  VkCommandBuffer beginSingleTimeCommands();
  void endSingleTimeCommands(VkCommandBuffer buffer);

  std::vector<const char*> getRequiredExtensions();

  bool isDeviceSuitable(VkPhysicalDevice device);
  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

  VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
  VkFormat findDepthFormat();

  GLFWwindow* _window;
  bool _enableValidationLayers;

  VkInstance _instance;
  VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
  VkDevice _device;

  QueueFamilyIndices _queueIndices;
  VkQueue _computeQ;
  VkQueue _graphicsQ;
  VkQueue _presentQ;

  VkDebugUtilsMessengerEXT _debugMessenger;

  VkSurfaceKHR _surface;
  Swapchain _swapChain;

  bool _framebufferResized = false;

  VkDescriptorPool _descriptorPool;
  VkDescriptorPool _imguiDescriptorPool;

  std::vector<VkCommandBuffer> _commandBuffers;
  VkCommandPool _commandPool;

  std::vector<VkSemaphore> _imageAvailableSemaphores;
  std::vector<VkSemaphore> _renderFinishedSemaphores;
  std::vector<VkFence> _inFlightFences;

  std::vector<const char*> _validationLayers = {
    "VK_LAYER_KHRONOS_validation",
    "VK_LAYER_KHRONOS_synchronization2"
  };

  const std::vector<const char*> _deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME
  };

  std::uint32_t _currentFrame = 0;
};
}