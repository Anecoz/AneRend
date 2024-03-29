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
#include "internal/InternalTerrain.h"
#include "internal/InternalLight.h"
#include "internal/GigaBuffer.h"
#include "internal/BufferMemoryInterface.h"
#include "internal/DeletionQueue.h"
#include "internal/InternalTexture.h"
#include "internal/UploadContext.h"
#include "internal/UploadQueue.h"
#include "internal/StagingBuffer.h"
#include "AccelerationStructure.h"
#include "scene/TileIndex.h"
#include "../component/Registry.h"
#include "asset/AssetFetcher.h"

//#include "../logic/WindSystem.h"

#include <array>
#include <any>
#include <functional>
#include <unordered_map>
#include <vector>

namespace render {

typedef std::function<void(glm::vec3)> WorldPosCallback;
typedef std::function<void(asset::Texture)> BakeTextureCallback;

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

class VulkanRenderer : public RenderContext, public internal::UploadContext
{
public:
  VulkanRenderer(GLFWwindow* window, const Camera& initialCamera, component::Registry* registry);
  ~VulkanRenderer();

  VulkanRenderer(const VulkanRenderer&) = delete;
  VulkanRenderer(VulkanRenderer&&) = delete;
  VulkanRenderer& operator=(const VulkanRenderer&) = delete;
  VulkanRenderer& operator=(VulkanRenderer&&) = delete;

  // Initializes surface, device, all material pipelines etc.
  bool init();

  void assetUpdate(AssetUpdate&& update) override final;

  void update(
    Camera& camera,
    const Camera& shadowCamera,
    const glm::vec4& lightDir,
    double delta,
    double time,
    bool lockCulling,
    RenderOptions renderOptions,
    RenderDebugOptions debugOptions);
    //logic::WindMap windMap);

  // Prepares some things for drawing (imgui stuff as of now).
  // Has to be called before drawFrame()!
  void prepare();

  // Goes through all registered renderables, updates any resource descriptor sets (like UBOs),
  // updates push constants, renders and queues for presentation.
  void drawFrame();

  // Will recreate swapchain to accomodate the new window size.
  void notifyFramebufferResized();

  // Request a world position for a given viewport 2D position.
  void requestWorldPosition(glm::ivec2 viewportPos, WorldPosCallback callback);

  // Sets the renderer in "baking" mode, baking diffuse GI at the given tile index.
  void startBakeDDGI(scene::TileIndex tileIdx);

  // Immediately returns the original camera position. Callback will be called after next update() has finished.
  glm::vec3 stopBake(BakeTextureCallback callback);

  // Update which registry to use for observing components.
  void setRegistry(component::Registry* registry);

  // Update which asset collection to use.
  void setAssetCollection(asset::AssetCollection* assetCollection);

  // Warning! This return value may not be valid the entire time. It is up to caller to make sure that update() and drawFrame() are called
  // in such a way that this remains valid.
  void* getImGuiTexId(util::Uuid& texId);

  // This is needed to support the editor, will force a ref of asset with id.
  int ref(const util::Uuid& id);

  // This is needed to support the editor, will check a ref of asset with id.
  int checkRef(const util::Uuid& id);

  // This is needed to support the editor, will force a deref of asset with id.
  int deref(const util::Uuid& id);

  // Upload context interface
  internal::StagingBuffer& getStagingBuffer() override final;
  internal::GigaBuffer& getVtxBuffer() override final;
  internal::GigaBuffer& getIdxBuffer() override final;
  RenderContext* getRC() override final;

  void textureUploadedCB(internal::InternalTexture tex) override final;
  void modelMeshUploadedCB(internal::InternalMesh mesh, VkCommandBuffer cmdBuf) override final;

  // Render Context interface
  bool isBaking() override final;

  void generateMipMaps(asset::Texture& tex) override final;

  VkDevice& device() override final;
  VkDescriptorPool& descriptorPool() override final;
  VmaAllocator vmaAllocator() override final;
  
  VkPipelineLayout& bindlessPipelineLayout() override final;
  VkDescriptorSetLayout& bindlessDescriptorSetLayout() override final;
  VkExtent2D swapChainExtent() override final;

  std::size_t getGigaBufferSizeMB() override final;

  VkDeviceAddress getGigaVtxBufferAddr() override final;
  VkDeviceAddress getGigaIdxBufferAddr() override final;

  void drawGigaBufferIndirect(VkCommandBuffer*, VkBuffer drawCalls, uint32_t drawCount) override final;
  void drawGigaBufferIndirectCount(VkCommandBuffer*, VkBuffer drawCalls, VkBuffer count, uint32_t maxDrawCount) override final;
  void drawNonIndexIndirect(VkCommandBuffer*, VkBuffer drawCalls, uint32_t drawCount, uint32_t stride) override final;
  void drawMeshId(VkCommandBuffer*, util::Uuid, uint32_t instanceCount) override final;

  VkImage& getCurrentSwapImage() override final;
  int getCurrentMultiBufferIdx() override final;
  int getMultiBufferSize() override final;

  size_t getMaxNumMeshes() override final;
  size_t getMaxNumRenderables() override final;
  size_t getMaxBindlessResources() override final;
  size_t getMaxNumPointLightShadows() override final;

  const std::vector<internal::InternalLight>& getLights() override final;
  std::vector<int> getShadowCasterLightIndices() override final;

  virtual std::vector<std::size_t> getTerrainIndices() override final;

  std::vector<internal::InternalMesh>& getCurrentMeshes() override final;
  std::vector<internal::InternalRenderable>& getCurrentRenderables() override final;
  bool getRenderableById(util::Uuid id, internal::InternalRenderable** out) override final;
  bool getMeshById(util::Uuid id, internal::InternalMesh** out) override final;
  std::unordered_map<util::Uuid, std::size_t>& getCurrentMeshUsage() override final;
  size_t getCurrentNumRenderables() override final;

  std::unordered_map<util::Uuid, std::vector<AccelerationStructure>>& getDynamicBlases() override final;

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
  util::Uuid getSphereMeshId() const { return _debugSphereMeshId; }

  size_t getNumIrradianceProbesXZ() override final;
  size_t getNumIrradianceProbesY() override final;

  double getElapsedTime() override final;

  std::vector<PerFrameTimer> getPerFrameTimers();

  const Camera& getCamera() override final;

  AccelerationStructure& getTLAS() override final;

  VkFormat getHDRFormat() override final;

  // Immediate mode debug draw functions.
  void debugDrawLine(debug::Line line) override final;
  void debugDrawTriangle(debug::Triangle triangle) override final;
  void debugDrawGeometry(debug::Geometry geometry) override final;

  std::vector<debug::Line> takeCurrentDebugLines() override final { return std::move(_currentDebugLines); }
  std::vector<debug::Triangle> takeCurrentDebugTriangles() override final { return std::move(_currentDebugTriangles); }
  std::vector<debug::Geometry> takeCurrentDebugGeometry() override final { return std::move(_currentDebugGeometries); }
  std::vector<debug::Geometry> takeCurrentDebugGeometryWireframe() override final { return std::move(_currentDebugGeometriesWireframe); }

  // Hack for testing
  std::vector<Particle>& getParticles() override final;

  bool blackboardValueBool(const std::string& key) override final;
  int blackboardValueInt(const std::string& key) override final;
  void setBlackboardValueBool(const std::string& key, bool val) override final;
  void setBlackboardValueInt(const std::string& key, int val) override final;

private:
  static const std::size_t MAX_FRAMES_IN_FLIGHT = 2;
  static const std::size_t MAX_PUSH_CONSTANT_SIZE = 128;
  static const std::size_t GIGA_MESH_BUFFER_SIZE_MB = 512;
  static const std::size_t STAGING_BUFFER_SIZE_MB = 128;
  static const std::size_t MAX_NUM_RENDERABLES = std::size_t(1e5);
  static const std::size_t MAX_NUM_MESHES = std::size_t(2500);
  static const std::size_t MAX_NUM_MATERIALS = 500;
  static const std::size_t NUM_PIXELS_CLUSTER_X = 16;
  static const std::size_t NUM_PIXELS_CLUSTER_Y = 9;
  static const std::size_t NUM_CLUSTER_DEPTH_SLIZES = 7;
  static const std::size_t MAX_NUM_LIGHTS = 32*32;
  static const std::size_t MAX_BINDLESS_RESOURCES = 16536;
  static const std::size_t MAX_TIMESTAMP_QUERIES = 150;
  static const std::size_t NUM_IRRADIANCE_PROBES_XZ = 64;
  static const std::size_t NUM_IRRADIANCE_PROBES_Y = 8;
  static const std::size_t MAX_NUM_JOINTS = 50;
  static const std::size_t MAX_NUM_SKINNED_MODELS = 1000;
  static const std::size_t MAX_NUM_POINT_LIGHT_SHADOWS = 4;
  static const std::size_t MAX_PAGE_TILE_RADIUS = 2;

  asset::AssetFetcher _assetFetcher;

  component::Registry* _registry = nullptr;
  entt::observer _nodeObserver;
  entt::observer _terrainObserver;

  void updateNodes();
  void updateSkeletons();
  void updateAssetFetches();

  bool arePrerequisitesUploaded(internal::InternalModel& model);
  bool arePrerequisitesUploaded(internal::InternalRenderable& rend);
  bool arePrerequisitesUploaded(internal::InternalMaterial& mat);
  bool arePrerequisitesUploaded(internal::InternalTerrain& terrain);

  void derefAssets(internal::InternalRenderable& rend);

  std::unordered_map<std::string, std::any> _blackboard;

  // Baking stuff
  struct DDGIBakeInfo
  {
    bool _stopNextFrame = false;
    scene::TileIndex _bakingIndex;
    glm::vec3 _originalCamPos;
    BakeTextureCallback _callback;
  } _bakeInfo;

  asset::Texture downloadDDGIAtlas();

  // Pending world position request
  struct WorldPosRequest
  {
    glm::ivec2 _viewportPos;
    AllocatedBuffer _hostBuffer;
    WorldPosCallback _callback;
    std::uint32_t _frameWhenRecordedCopy;
    bool _recorded = false;
  } _worldPosRequest;
  
  void checkWorldPosCallback();
  void worldPosCopy(VkCommandBuffer cmdBuf);

  util::Uuid _debugSphereMeshId;

  RenderDebugOptions _debugOptions;
  RenderOptions _renderOptions;
  //logic::WindMap _currentWindMap;

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
  //internal::AnimationThread _animThread;

  // Single mem interface to place skeletons into GPU buffers.
  internal::BufferMemoryInterface _skeletonMemIf;

  // Keeps track of where skeletons should go in the related GPU buffer. Does not own skeleton data.
  // NOTE: The offset given by each Handle is in terms of _NUMBER OF_ matrices, not bytes!
  std::unordered_map<util::Uuid, internal::BufferMemoryInterface::Handle> _skeletonOffsets;

  uint32_t getOrCreateSkeleOffset(util::Uuid& node, std::size_t numMatrices);

  // This is a mirror of the GPU buffer, for simplicity re-created on CPU here.
  std::array<glm::mat4, MAX_NUM_SKINNED_MODELS * MAX_NUM_JOINTS>* _cachedSkeletons = new std::array<glm::mat4, MAX_NUM_SKINNED_MODELS* MAX_NUM_JOINTS>;

  // Keep track of imgui tex ids (Descriptor sets as of now)
  std::unordered_map<util::Uuid, void*> _imguiTexIds;

  // Assets
  std::vector<internal::InternalModel> _currentModels;
  std::vector<internal::InternalMesh> _currentMeshes;
  std::vector<internal::InternalMaterial> _currentMaterials;
  std::vector<internal::InternalTexture> _currentTextures;
  std::vector<internal::InternalTerrain> _currentTerrains;
  std::vector<internal::InternalRenderable> _currentRenderables;
  std::vector<internal::InternalRenderable> _pendingFirstUploadRenderables;

  // Maps engine-wide IDs to internal buffer ids.
  std::unordered_map<util::Uuid, std::size_t> _modelIdMap;
  std::unordered_map<util::Uuid, std::size_t> _meshIdMap;
  std::unordered_map<util::Uuid, std::size_t> _renderableIdMap;
  std::unordered_map<util::Uuid, std::size_t> _materialIdMap;
  std::unordered_map<util::Uuid, std::size_t> _textureIdMap;
  std::unordered_map<util::Uuid, std::size_t> _terrainIdMap;

  // This is needed for generating draw calls, it records how many renderables use each mesh.
  std::unordered_map<util::Uuid, std::size_t> _currentMeshUsage;

  std::vector<bool> _modelsChanged;
  std::vector<bool> _renderablesChanged;
  std::vector<bool> _lightsChanged;
  std::vector<bool> _materialsChanged;
  std::vector<bool> _texturesChanged;
  std::vector<bool> _terrainsChanged;
  std::vector<bool> _tileInfosChanged;

  std::vector<asset::Material> _materialsToUpload;

  void uploadPendingMaterials(VkCommandBuffer cmdBuffer);

  // Ray tracing related
  AccelerationStructure registerBottomLevelAS(VkCommandBuffer cmdBuffer, util::Uuid meshId, bool dynamic = false, bool doBarrier = true);
  void buildTopLevelAS();
  void writeTLASDescriptor();
  bool _topLevelBuilt = false;

  // Only for ray-tracing: These renderables need to have their models (all meshes)
  // copied, aswell as their blases, since they are dynamic (per-renderable).
  // Typically animated.
  struct DynamicModelCopyInfo
  {
    util::Uuid _renderableId;
    std::vector<util::Uuid> _generatedDynamicIds;
    std::vector<AccelerationStructure> _currentBlases;
    std::size_t _currentMeshIdx = 0;
  };
  std::vector<DynamicModelCopyInfo> _dynamicModelsToCopy;

  void copyDynamicModels(VkCommandBuffer cmdBuffer);

  // Static BLAS for each mesh
  std::unordered_map<util::Uuid, AccelerationStructure> _blases;

  // Dynamic BLASes for dynamically updated (animated) renderables
  // Vector contains one BLAS per mesh in the renderable model
  std::unordered_map<util::Uuid, std::vector<AccelerationStructure>> _dynamicBlases;

  // The single TLAS written to each frame.
  AccelerationStructure _tlas;

  VkPhysicalDeviceRayTracingPipelinePropertiesKHR _rtPipeProps;
  std::size_t _rtScratchAlignment = 0;

  /* 
  * "Bindless" descriptor set layout and pipeline layout, used for every render pass.
  * The render passes create their own pipelines, specifying these on creation (accessed via RenderContext).
  * Only one descriptor set gets bound before all rendering.
  */
  VkPipelineLayout _bindlessPipelineLayout;
  VkDescriptorSetLayout _bindlessDescSetLayout;
  std::vector<VkDescriptorSet> _bindlessDescriptorSets;

  uint32_t _renderableMatIndexBinding = 7;
  uint32_t _modelBinding = _renderableMatIndexBinding + 1;
  uint32_t _gigaIdxBinding = _modelBinding + 1;
  uint32_t _gigaVtxBinding = _gigaIdxBinding + 1;
  uint32_t _meshBinding = _gigaVtxBinding + 1;
  uint32_t _tlasBinding = _meshBinding + 1;
  uint32_t _skeletonBinding = _tlasBinding + 1;
  uint32_t _tileBinding = _skeletonBinding + 1;
  uint32_t _terrainBinding = _tileBinding + 1;
  uint32_t _bindlessTextureBinding = _terrainBinding + 1;

  // Use a mem interface to select empty bindless indices.
  internal::BufferMemoryInterface _bindlessTextureMemIf;

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

  VkDeviceAddress _gigaVtxAddr;
  VkDeviceAddress _gigaIdxAddr;

  // Staging buffer for copying data to the gpu buffers.
  std::vector<internal::StagingBuffer> _gpuStagingBuffer;

  // Contains renderable info for compute culling shader.
  std::vector<AllocatedBuffer> _gpuRenderableBuffer;

  // Contains material info.
  std::vector<AllocatedBuffer> _gpuMaterialBuffer;

  // Maps renderable material indices into the actual material buffer.
  std::vector<AllocatedBuffer> _gpuRenderableMaterialIndexBuffer;

  // Maps models to mesh indices
  std::vector<AllocatedBuffer> _gpuModelBuffer;

  // Contains mesh info
  std::vector<AllocatedBuffer> _gpuMeshInfoBuffer;

  // Contains all joints for all skinned animated models
  std::vector<AllocatedBuffer> _gpuSkeletonBuffer;

  // SSBO for light information.
  std::vector<AllocatedBuffer> _gpuLightBuffer;

  // SSBO for per-tile information.
  std::vector<AllocatedBuffer> _gpuTileBuffer;

  // UBO for point light shadow cube views, used in multiview rendering.
  std::vector<AllocatedBuffer> _gpuPointLightShadowBuffer;

  // SSBO for the view clusters.
  std::vector<AllocatedBuffer> _gpuViewClusterBuffer;

  // Contains scene data needed in shaders (view and proj matrices etc.)
  std::vector<AllocatedBuffer> _gpuSceneDataBuffer;

  // SSBO with terrain info used in terrain pass.
  std::vector<AllocatedBuffer> _gpuTerrainBuffer;

  // Contains wind system data, accessed by various render passes.
  std::vector<AllocatedImage> _gpuWindForceImage;

  // View for the wind force images.
  std::vector<VkImageView> _gpuWindForceView;

  // Samplers for the wind force images.
  std::vector<VkSampler> _gpuWindForceSampler;

  // Fills gpu renderable buffer with current renderable information (could be done async)
  bool prefillGPURenderableBuffer(VkCommandBuffer& commandBuffer);

  // Fill info about which material index each renderable references.
  bool prefillGPURendMatIdxBuffer(VkCommandBuffer& commandBuffer);

  // Fill buffer with mesh indices of currently used models.
  bool prefillGPUModelBuffer(VkCommandBuffer& commandBuffer);

  // Fill gpu mesh info.
  bool prefillGPUMeshBuffer(VkCommandBuffer& commandBuffer);

  // Fill gpu skeleton info from cached CPU array.
  void prefillGPUSkeletonBuffer(VkCommandBuffer& commandBuffer);

  // Fills GPU light buffer with current light information.
  void prefillGPULightBuffer(VkCommandBuffer& commandBuffer);

  // Fills GPU buffer containing current point light shadow cube views.
  void prefillGPUPointLightShadowCubeBuffer(VkCommandBuffer& commandBuffer);

  // Fills GPU tile info buffer with the current tile infos (camera relative).
  void prefillGPUTileInfoBuffer(VkCommandBuffer& commandBuffer);

  // Fills GPU terrain info buffer with currently paged terrain infos.
  bool prefillGPUTerrainInfoBuffer(VkCommandBuffer& commandBuffer);

  // Update wind force image
  void updateWindForceImage(VkCommandBuffer& commandBuffer);

  // Current tile infos
  std::vector<asset::TileInfo> _currentTileInfos;

  // View space clusters for the light assignment
  struct ViewCluster
  {
    // AABB
    glm::vec3 _minBounds;
    glm::vec3 _maxBounds;
  };

  std::vector<ViewCluster> _viewClusters;

  std::vector<internal::InternalLight> _lights;
  std::array<util::Uuid, MAX_NUM_POINT_LIGHT_SHADOWS> _shadowCasters;

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
  internal::DeletionQueue _delQ;
  internal::UploadQueue _uploadQ;

  std::vector<debug::Line> _currentDebugLines;
  std::vector<debug::Triangle> _currentDebugTriangles;
  std::vector<debug::Geometry> _currentDebugGeometriesWireframe;
  std::vector<debug::Geometry> _currentDebugGeometries;
};
}