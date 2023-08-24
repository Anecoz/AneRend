#pragma once

#include <bitset>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "AllocatedBuffer.h"
#include "PipelineUtil.h"
#include "ShaderBindingTable.h"

#include <vulkan/vulkan.h>

namespace render {

class RenderResourceVault;
class RenderContext;
struct IRenderResource;

struct RenderExeParams
{
  RenderResourceVault* vault;
  RenderContext* rc;
  VkCommandBuffer* cmdBuffer;

  VkPipeline* pipeline;
  VkPipelineLayout* pipelineLayout;
  std::vector<VkDescriptorSet>* descriptorSets;
  ShaderBindingTable* sbt; // For ray trace pipelines

  std::vector<VkImageView> colorAttachmentViews;
  std::vector<VkImageView> depthAttachmentViews;
  VkImage presentImage;
  std::vector<VkBuffer> buffers;
  std::vector<VkSampler> samplers;
  std::vector<VkImage> images;
};

typedef std::function<void(RenderExeParams)> RenderPassExeFcn;

typedef std::function<void(IRenderResource* resource, VkCommandBuffer& cmdBuffer, RenderContext* renderContext)> ResourceInitFcn;

typedef std::function<void(IRenderResource* resource, VkCommandBuffer& cmdBuffer)> BarrierFcn;

enum class Access
{
  Read,
  Write,
  Count
};

enum class Stage : std::size_t
{
  Transfer,
  Compute,
  IndirectDraw,
  Vertex,
  Fragment,
  Irrelevant,
  RayTrace,
  Count
};

enum class Type
{
  ColorAttachment,
  DepthAttachment,
  Present,
  SampledTexture,
  SampledDepthTexture,
  UBO,
  SSBO,
  Irrelevant,
  ImageStorage,
  ImageTransferSrc,
  ImageTransferDst,
  Count
};

typedef std::bitset<(std::size_t)Access::Count> AccessBits;
typedef std::bitset<(std::size_t)Stage::Count> StageBits;

struct BufferInitialCreateInfo
{
  std::size_t _initialSize;
  std::function<void(RenderContext*, AllocatedBuffer&)> _initialDataCb = nullptr;
  bool _hostWritable = false;
  bool _multiBuffered = false;
};

struct ImageInitialCreateInfo
{
  std::size_t _initialWidth;
  std::size_t _initialHeight;
  VkFormat _intialFormat;
  std::uint32_t _mipLevels = 1;
  VkImageLayout _initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  std::function<void(RenderContext*, VkImage&)> _initialDataCb = nullptr;
  bool _multiBuffered = false;
};

struct ResourceUsage
{  
  AccessBits _access;
  StageBits _stage;
  Type _type;

  std::uint32_t _imageBaseLayer;

  bool _invalidateAfterRead = false;
  bool _ownedByEngine = false; // If true, indicates that the renderer owns it, so no create info is needed
  std::string _resourceName;

  std::optional<BufferInitialCreateInfo> _bufferCreateInfo;
  std::optional<ImageInitialCreateInfo> _imageCreateInfo;

  bool _imageAlwaysGeneral = false; // Override logic about image layout and force to be general.

  bool _samplerClampToBorder = false; // Use clamping for samplers.

  bool _samplerClampToEdge = false; // Use clamping for samplers.

  bool _noSamplerFiltering = false;

  bool _allMips = true;
  uint32_t _mip = 0; // if _allMips is false, this specifies which mip level to use.

  float _minLod = 0.0f;
  float _maxLod = 10.0f;

  bool _useMaxSampler = false; // Special case for Hi-Z generation...

  bool _bindless = false; // Currently only supported by combined image/samplers.

  bool _multiBuffered = false; // Filled in by frame graph builder
};

struct RenderPassRegisterInfo
{
  std::string _name;
  std::string _group = "";
  std::vector<ResourceUsage> _resourceUsages;
  bool _present = false;

  std::optional<ComputePipelineCreateParams> _computeParams;
  std::optional<GraphicsPipelineCreateParams> _graphicsParams;
  std::optional<RayTracingPipelineCreateParams> _rtParams;
};

class FrameGraphBuilder
{
public:
  FrameGraphBuilder();
  FrameGraphBuilder(RenderResourceVault* resourceVault);
  ~FrameGraphBuilder() = default;

  FrameGraphBuilder(const FrameGraphBuilder&) = delete;
  FrameGraphBuilder(FrameGraphBuilder&&) = delete;
  FrameGraphBuilder& operator=(const FrameGraphBuilder&) = delete;
  FrameGraphBuilder& operator=(FrameGraphBuilder&&) = delete;

  void reset(RenderContext* rc);

  void registerResourceInitExe(const std::string& resource, ResourceUsage&& initUsage, ResourceInitFcn initFcn);

  void registerRenderPass(RenderPassRegisterInfo&& registerInfo);
  void registerRenderPassExe(const std::string& renderPass, RenderPassExeFcn exeFcn);

  void executeGraph(VkCommandBuffer& cmdBuffer, RenderContext* renderContext);

  bool build(RenderContext* renderContext, RenderResourceVault* vault);

  void printBuiltGraphDebug();

private:
  RenderResourceVault* _vault;

  struct Submission
  {
    RenderPassRegisterInfo _regInfo;
    RenderPassExeFcn _exe;
  };

  struct ResourceInit
  {
    std::string _resource;
    ResourceUsage _initUsage;
    ResourceInitFcn _initFcn;
  };
 
  struct BarrierContext
  {
    std::string _resourceName;
    BarrierFcn _barrierFcn;
  };

  struct GraphNode
  {
    std::optional<ResourceInit*> _resourceInit;
    std::optional<RenderPassExeFcn> _rpExe;
    std::optional<BarrierContext> _barrier;

    std::vector<std::string> _producedResources;
    std::vector<ResourceUsage> _resourceUsages;

    std::string _debugName;
    std::string _group;

    std::optional<ComputePipelineCreateParams> _computeParams;
    std::optional<GraphicsPipelineCreateParams> _graphicsParams;
    std::optional<RayTracingPipelineCreateParams> _rtParams;

    // Things that are bound in the case this is a render pass exe
    VkPipeline _pipeline;
    VkPipelineLayout _pipelineLayout;
    VkDescriptorSetLayout _descriptorSetLayout;
    std::vector<VkDescriptorSet> _descriptorSets;
    std::vector<VkSampler> _samplers;
    ShaderBindingTable _sbt;
  };

  struct ResourceGraphInfo
  {
    std::string _resource;
    std::vector<Submission*> _rSubs;
    std::vector<Submission*> _wSubs;
  };

  bool stackContainsProducer(std::vector<GraphNode>& stack, const std::string& resource, GraphNode** nodeOut);
  void ensureOrder(std::vector<GraphNode>& stack, const std::string& nameBefore, const std::string& nameAfter);
  Submission* findSubmission(const std::string& name);
  ResourceInit* findResourceInit(const std::string& resource);
  std::vector<Submission*> findResourceProducers(const std::string& resource, const std::string& excludePass);

  std::vector<VkBufferUsageFlagBits> findBufferCreateFlags(const std::string& bufferResource);
  std::vector<VkImageUsageFlagBits> findImageCreateFlags(const std::string& resource);
  bool createResources(RenderContext* renderContext, RenderResourceVault* vault);

  VkShaderStageFlags findStages(const std::string& resource);
  bool createPipelines(RenderContext* renderContext, RenderResourceVault* vault);

  void findDependenciesRecurse(std::vector<GraphNode>& stack, Submission* submission);

  void internalBuild2(Submission* presentSub);
  void internalBuild3();

  std::pair<VkImageLayout, VkImageLayout> findImageLayoutUsage(AccessBits prevAccess, Type prevType, AccessBits newAccess, Type newType);
  VkImageLayout findInitialImageLayout(AccessBits access, Type type);
  std::string debugConstructImageBarrierName(VkImageLayout old, VkImageLayout newLayout);
  std::vector<ResourceUsage*> findPrevMipUsages(const ResourceUsage& usage, std::vector<GraphNode>& stack, int stackIdx);

  std::pair<VkAccessFlagBits, VkAccessFlagBits> findBufferAccessFlags(AccessBits prevAccess, StageBits prevStage, AccessBits newAccess, StageBits newStage);
  std::pair<VkPipelineStageFlagBits, VkPipelineStageFlagBits> translateStageBits(Type prevType, Type newType, StageBits prevStage, StageBits newStage);
  std::string debugConstructBufferBarrierName(VkAccessFlagBits oldAccess, VkPipelineStageFlagBits oldStage, VkAccessFlagBits newAccess, VkPipelineStageFlagBits newStage);

  void insertBarriers(std::vector<GraphNode>& stack);

  std::vector<ResourceInit> _resourceInits;
  std::vector<Submission> _submissions;
  std::vector<GraphNode> _builtGraph;
};

}