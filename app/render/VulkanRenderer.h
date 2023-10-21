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
#include "Camera.h"
#include "AllocatedBuffer.h"
#include "AllocatedImage.h"
#include "Swapchain.h"
#include "Light.h"
#include "RenderContext.h"
#include "passes/RenderPass.h"
#include "RenderResourceVault.h"
#include "FrameGraphBuilder.h"
#include "Particle.h"
#include "internal/InternalMesh.h"
#include "internal/InternalModel.h"
#include "internal/InternalMaterial.h"
#include "internal/InternalRenderable.h"
#include "internal/GigaBuffer.h"
#include "internal/AnimationThread.h"
#include "AccelerationStructure.h"
#include "animation/Animator.h"

#include "../logic/WindSystem.h"

#include <array>
#include <any>
#include <unordered_map>
#include <optional>
#include <vector>
#include <tuple>

namespace render {

struct PerFrameTimer
{
  std::string _name;
  std::string _group;

  double _durationMs = 0.0f;
  double _avg10 = 0.0f;
  double _avg100 = 0.0f;

  double _cumulative10 = 0.0f;
  int _currNum10 = 0;

  double _cumulative100 = 0.0f;
  int _currNum100 = 0;

  std::vector<float> _buf;
};

class VulkanRenderer : public RenderContext
{
public:
  VulkanRenderer(GLFWwindow* window, const Camera& initialCamera);
  ~VulkanRenderer();

  VulkanRenderer(const VulkanRenderer&) = delete;
  VulkanRenderer(VulkanRenderer&&) = delete;
  VulkanRenderer& operator=(const VulkanRenderer&) = delete;
  VulkanRenderer& operator=(VulkanRenderer&&) = delete;

  // Initializes surface, device, all material pipelines etc.
  bool init();

  void assetUpdate(AssetUpdate&& update) override final;

  void update(
    const Camera& camera,
    const Camera& shadowCamera,
    const glm::vec4& lightDir,
    double delta,
    double time,
    bool lockCulling,
    RenderOptions renderOptions,
    RenderDebugOptions debugOptions,
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

  //void drawGigaBuffer(VkCommandBuffer* commandBuffer) override final;
  void drawGigaBufferIndirect(VkCommandBuffer*, VkBuffer drawCalls, uint32_t drawCount) override final;
  void drawNonIndexIndirect(VkCommandBuffer*, VkBuffer drawCalls, uint32_t drawCount, uint32_t stride) override final;
  void drawMeshId(VkCommandBuffer*, MeshId, uint32_t vertCount, uint32_t instanceCount) override final;

  VkImage& getCurrentSwapImage() override final;
  int getCurrentMultiBufferIdx() override final;
  int getMultiBufferSize() override final;

  size_t getMaxNumMeshes() override final;
  size_t getMaxNumRenderables() override final;

  size_t getMaxBindlessResources() override final;

  std::vector<internal::InternalMesh>& getCurrentMeshes() override final;
  std::unordered_map<MeshId, std::size_t>& getCurrentMeshUsage() override final;
  size_t getCurrentNumRenderables() override final;

  gpu::GPUCullPushConstants getCullParams() override final;

  RenderOptions& getRenderOptions() override final;
  RenderDebugOptions& getDebugOptions() override final;

  void setDebugName(VkObjectType objectType, uint64_t objectHandle, const char* name) override final;

  virtual glm::vec2 getWindDir() override final;

  VkCommandBuffer beginSingleTimeCommands() override final;
  void endSingleTimeCommands(VkCommandBuffer buffer) override final;

  VkPhysicalDeviceRayTracingPipelinePropertiesKHR getRtPipeProps() override final;

  void registerPerFrameTimer(const std::string& name, const std::string& group) override final;
  void startTimer(const std::string& name, VkCommandBuffer cmdBuffer) override final;
  void stopTimer(const std::string& name, VkCommandBuffer cmdBuffer) override final;

  internal::InternalMesh& getSphereMesh() override final;

  size_t getNumIrradianceProbesXZ() override final;
  size_t getNumIrradianceProbesY() override final;

  double getElapsedTime() override final;

  std::vector<PerFrameTimer> getPerFrameTimers();

  const Camera& getCamera() override final;

  AccelerationStructure& getTLAS() override final;

  VkFormat getHDRFormat() override final;

  // Hack for testing
  std::vector<Particle>& getParticles() override final;

  bool blackboardValueBool(const std::string& key) override final;
  int blackboardValueInt(const std::string& key) override final;
  void setBlackboardValueBool(const std::string& key, bool val) override final;
  void setBlackboardValueInt(const std::string& key, int val) override final;

private:
  static const std::size_t MAX_FRAMES_IN_FLIGHT = 2;
  static const std::size_t MAX_PUSH_CONSTANT_SIZE = 128;
  static const std::size_t GIGA_MESH_BUFFER_SIZE_MB = 128;
  static const std::size_t STAGING_BUFFER_SIZE_MB = 512;
  static const std::size_t MAX_NUM_RENDERABLES = std::size_t(1e5);
  static const std::size_t MAX_NUM_MESHES = std::size_t(1e3);
  static const std::size_t MAX_NUM_MATERIALS = 10000;
  static const std::size_t NUM_PIXELS_CLUSTER_X = 16;
  static const std::size_t NUM_PIXELS_CLUSTER_Y = 9;
  static const std::size_t NUM_CLUSTER_DEPTH_SLIZES = 7;
  static const std::size_t MAX_NUM_LIGHTS = 32*32;
  static const std::size_t MAX_BINDLESS_RESOURCES = 16536;
  static const std::size_t MAX_TIMESTAMP_QUERIES = 100;
  static const std::size_t NUM_IRRADIANCE_PROBES_XZ = 64;
  static const std::size_t NUM_IRRADIANCE_PROBES_Y = 8;
  static const std::size_t MAX_NUM_JOINTS = 50;
  static const std::size_t MAX_NUM_SKINNED_MODELS = 1000;

  std::unordered_map<std::string, std::any> _blackboard;

  MeshId _debugSphereMeshId;

  RenderDebugOptions _debugOptions;
  RenderOptions _renderOptions;
  logic::WindMap _currentWindMap;

  Camera _latestCamera;

  // Query pool for timestamps, one per frame
  std::vector<VkQueryPool> _queryPools;

  // How to interpret timestamp results
  float _timestampPeriod;

  // Timers that run per frame and then get reset
  std::vector<PerFrameTimer> _perFrameTimers;

  int findTimerIndex(const std::string& timer);
  void resetPerFrameQueries(VkCommandBuffer cmdBuffer);
  void computePerFrameQueries();

  // This is dangerous, but points to "current" image index in the swap chain
  uint32_t _currentSwapChainIndex;

  // Animation multithread runtime.
  internal::AnimationThread _animThread;

  // Single mem interface to place skeletons into GPU buffers.
  internal::BufferMemoryInterface _skeletonMemIf;

  // Keeps track of where skeletons should go in the related GPU buffer. Does not own skeleton data.
  // NOTE: The offset given by each Handle is in terms of _NUMBER OF_ matrices, not bytes!
  std::unordered_map<render::SkeletonId, internal::BufferMemoryInterface::Handle> _skeletonOffsets;

  // Assets
  std::vector<internal::InternalModel> _currentModels;
  std::vector<internal::InternalMesh> _currentMeshes;
  std::vector<internal::InternalMaterial> _currentMaterials;
  std::vector<internal::InternalRenderable> _currentRenderables;

  // Maps engine-wide IDs to internal buffer ids.
  std::unordered_map<ModelId, std::size_t> _modelIdMap;
  std::unordered_map<MeshId, std::size_t> _meshIdMap;
  std::unordered_map<RenderableId, std::size_t> _renderableIdMap;
  std::unordered_map<MaterialId, std::size_t> _materialIdMap;

  // This is needed for generating draw calls, it records how many renderables use each mesh.
  std::unordered_map<MeshId, std::size_t> _currentMeshUsage;

  std::vector<bool> _modelsChanged;
  std::vector<bool> _renderablesChanged;
  std::vector<bool> _materialsChanged;

  // Pending uploads
  std::vector<asset::Model> _modelsToUpload;
  std::vector<asset::Material> _materialsToUpload;

  void uploadPendingModels(VkCommandBuffer cmdBuffer);
  void uploadPendingMaterials(VkCommandBuffer cmdBuffer);

  // Ray tracing related
  void registerBottomLevelAS(VkCommandBuffer cmdBuffer, MeshId meshId);
  void buildTopLevelAS();
  void writeTLASDescriptor();
  bool _topLevelBuilt = false;

  std::unordered_map<MeshId, AccelerationStructure> _blases;
  AccelerationStructure _tlas;

  VkPhysicalDeviceRayTracingPipelinePropertiesKHR _rtPipeProps;

  /* 
  * "Bindless" descriptor set layout and pipeline layout, used for every render pass.
  * The render passes create their own pipelines, specifying these on creation (accessed via RenderContext).
  * Only one descriptor set gets bound before all rendering.
  */
  VkPipelineLayout _bindlessPipelineLayout;
  VkDescriptorSetLayout _bindlessDescSetLayout;
  std::vector<VkDescriptorSet> _bindlessDescriptorSets;

  uint32_t _renderableMatIndexBinding = 6;
  uint32_t _gigaIdxBinding = _renderableMatIndexBinding + 1;
  uint32_t _gigaVtxBinding = _gigaIdxBinding + 1;
  uint32_t _meshBinding = _gigaVtxBinding + 1;
  uint32_t _tlasBinding = _meshBinding + 1;
  uint32_t _skeletonBinding = _tlasBinding + 1;
  uint32_t _idMapBinding = _skeletonBinding + 1;
  uint32_t _bindlessTextureBinding = _idMapBinding + 1;

  // Use a mem interface to select empty bindless indices.
  internal::BufferMemoryInterface _bindlessTextureMemIf;
  //uint32_t _currentBindlessTextureIndex = 0;

  void createTexture(
    VkCommandBuffer cmdBuffer,
    VkFormat format, 
    int width, 
    int height, 
    const std::vector<uint8_t>& data,
    VkSampler& samplerOut,
    AllocatedImage& imageOut,
    VkImageView& viewOut);

  void generateMipmaps(
    VkCommandBuffer cmdBuffer,
    VkImage image,
    VkFormat imageFormat,
    int32_t texWidth,
    int32_t texHeight,
    uint32_t mipLevels);

  internal::BufferMemoryInterface::Handle addTextureToBindless(VkImageLayout layout, VkImageView view, VkSampler sampler);

  RenderResourceVault _vault;
  FrameGraphBuilder _fgb;
  std::vector<RenderPass*> _renderPasses;

  void executeFrameGraph(VkCommandBuffer commandBuffer, int imageIndex);

  // Testing
  void createParticles();
  std::vector<Particle> _particles;

  // These buffers contain vertex and index data for all current meshes
  internal::GigaBuffer _gigaVtxBuffer;
  internal::GigaBuffer _gigaIdxBuffer;

  // Staging buffer for copying data to the gpu buffers.
  std::vector<AllocatedBuffer> _gpuStagingBuffer;
  std::size_t _currentStagingOffset = 0;

  // Contains renderable info for compute culling shader.
  std::vector<AllocatedBuffer> _gpuRenderableBuffer;

  // Contains mappings from *Id to internal indices.
  std::vector<AllocatedBuffer> _gpuIdMapBuffer;

  // Contains material info.
  std::vector<AllocatedBuffer> _gpuMaterialBuffer;

  // Maps renderable material indices into the actual material buffer.
  std::vector<AllocatedBuffer> _gpuRenderableMaterialIndexBuffer;

  // Contains mesh info
  std::vector<AllocatedBuffer> _gpuMeshInfoBuffer;

  // Contains all joints for all skinned animated models
  std::vector<AllocatedBuffer> _gpuSkeletonBuffer;

  // SSBO for light information.
  std::vector<AllocatedBuffer> _gpuLightBuffer;

  // SSBO for the view clusters.
  std::vector<AllocatedBuffer> _gpuViewClusterBuffer;

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

  // Fill info about which material index each renderable references.
  void prefillGPURendMatIdxBuffer(VkCommandBuffer& commandBuffer);

  // Fills gpu id map buffer with the current renderable and mesh ids.
  void prefillGPUIdMapBuffer(VkCommandBuffer& commandBuffer);

  // Fill gpu mesh info.
  void prefillGPUMeshBuffer(VkCommandBuffer& commandBuffer);

  // Fill gpu skeleton info.
  void prefillGPUSkeletonBuffer(VkCommandBuffer& commandBuffer, std::vector<anim::Skeleton>& skeletons);

  // Fills GPU light buffer with current light information.
  void prefillGPULightBuffer(VkCommandBuffer& commandBuffer);

  // Update wind force image
  void updateWindForceImage(VkCommandBuffer& commandBuffer);

  // View space clusters for the light assignment
  struct ViewCluster
  {
    // AABB
    glm::vec3 _minBounds;
    glm::vec3 _maxBounds;
  };

  std::vector<ViewCluster> _viewClusters;

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
  bool createQueryPool();
  bool createCommandBuffers();
  bool createSyncObjects();
  bool createDescriptorPool();
  bool initLights();
  bool initImgui();
  bool initGigaMeshBuffer();
  bool initGpuBuffers();
  bool initBindless();
  bool initFrameGraphBuilder();
  bool initRenderPasses();

  bool checkValidationLayerSupport();
  bool checkDeviceExtensionSupport(VkPhysicalDevice device);
  void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

  void cleanupSwapChain();
  void recreateSwapChain();

  std::vector<const char*> getRequiredExtensions();

  bool isDeviceSuitable(VkPhysicalDevice device);
  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

  VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
  VkFormat findDepthFormat();

  GLFWwindow* _window;
  bool _enableValidationLayers;
  bool _enableRayTracing;

  VkInstance _instance;
  VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
  VkDevice _device;

  QueueFamilyIndices _queueIndices;
  VkQueue _computeQ;
  VkQueue _transferQ;
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

  const std::vector<const char*> _deviceExtensionsRT = {
  VK_KHR_SWAPCHAIN_EXTENSION_NAME,
  VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
  VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
  VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
  VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
  VK_EXT_SHADER_ATOMIC_FLOAT_EXTENSION_NAME
  };

  std::uint32_t _currentFrame = 0;
};
}