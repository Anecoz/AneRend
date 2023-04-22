#pragma once

#include <bitset>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "AllocatedBuffer.h"

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

  std::vector<VkImageView> colorAttachmentViews;
  std::vector<VkImageView> depthAttachmentViews;
  VkImage presentImage;
  std::vector<VkBuffer> buffers;
  std::vector<VkSampler> samplers;
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
  std::function<void(RenderContext*, VkImage&)> _initialDataCb = nullptr;
  bool _multiBuffered = false;
};

struct GraphicsPipelineCreateParams
{
  VkDevice device = nullptr;
  VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  std::vector<VkFormat> colorFormats = { VK_FORMAT_B8G8R8A8_SRGB };
  uint32_t colorAttachmentCount = 1;
  VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
  std::string vertShader;
  std::string fragShader;
  int posLoc = 0;
  int colorLoc = 1;
  int normalLoc = 2;
  int uvLoc = -1;
  VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
  bool depthBiasEnable = false;
  float depthBiasConstant = 0.0f;
  float depthBiasSlope = 0.0f;
  bool depthTest = true;
  bool colorAttachment = true;
  bool vertexLess = false;
};

struct ComputePipelineCreateParams
{
  VkDevice device = nullptr;
  std::string shader;
};

struct ResourceUsage
{  
  AccessBits _access;
  StageBits _stage;
  Type _type;

  std::uint32_t _imageBaseLayer;

  bool _invalidateAfterRead = false;
  std::string _resourceName;

  std::optional<BufferInitialCreateInfo> _bufferCreateInfo;
  std::optional<ImageInitialCreateInfo> _imageCreateInfo;

  bool _multiBuffered = false; // Filled in by frame graph builder
};

struct RenderPassRegisterInfo
{
  std::string _name;
  std::vector<ResourceUsage> _resourceUsages;
  bool _present = false;

  std::optional<ComputePipelineCreateParams> _computeParams;
  std::optional<GraphicsPipelineCreateParams> _graphicsParams;
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

  struct DescriptorBindInfo
  {
    uint32_t binding;
    VkShaderStageFlags stages;
    VkDescriptorType type;
    VkImageLayout imageLayout;
    VkImageView view;
    VkSampler sampler;
    VkBuffer buffer;
  };

  struct DescriptorSetLayoutCreateParams
  {
    RenderContext* renderContext;

    std::vector<DescriptorBindInfo> bindInfos;
  };

  struct PipelineLayoutCreateParams
  {
    VkDevice device;
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
    uint32_t pushConstantsSize = 0;
    VkShaderStageFlags pushConstantStages;
  };

  struct DescriptorSetsCreateParams
  {
    RenderContext* renderContext;

    bool multiBuffered = true;
    std::vector<DescriptorBindInfo> bindInfos;
    VkDescriptorSetLayout descLayout;
  };

  struct SamplerCreateParams
  {
    RenderContext* renderContext;

    VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    VkFilter magFilter = VK_FILTER_NEAREST;
    VkFilter minFilter = VK_FILTER_NEAREST;
    VkSamplerMipmapMode mipMapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    float mipLodBias = 0.0f;
    float maxAnisotropy = 1.0f;
    float minLod = 0.0f;
    float maxLod = 1.0f;
    VkBorderColor borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
  };

  struct GraphNode
  {
    std::optional<ResourceInit*> _resourceInit;
    std::optional<RenderPassExeFcn> _rpExe;
    std::optional<BarrierContext> _barrier;

    std::vector<std::string> _producedResources;
    std::vector<ResourceUsage> _resourceUsages;

    std::string _debugName;

    std::optional<ComputePipelineCreateParams> _computeParams;
    std::optional<GraphicsPipelineCreateParams> _graphicsParams;

    // Things that are bound in the case this is a render pass exe
    VkPipeline _pipeline;
    VkPipelineLayout _pipelineLayout;
    VkDescriptorSetLayout _descriptorSetLayout;
    std::vector<VkDescriptorSet> _descriptorSets;
    std::vector<VkSampler> _samplers;
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
  bool buildPipelineLayout(PipelineLayoutCreateParams params, VkPipelineLayout& outPipelineLayout);
  bool buildDescriptorSetLayout(DescriptorSetLayoutCreateParams params, VkDescriptorSetLayout& outDescriptorSetLayout, VkPipelineLayout& outPipelineLayout);
  std::vector<VkDescriptorSet> buildDescriptorSets(DescriptorSetsCreateParams params);
  VkSampler createSampler(SamplerCreateParams params);
  bool buildComputePipeline(ComputePipelineCreateParams params, VkPipelineLayout& pipelineLayout, VkPipeline& outPipeline);
  bool buildGraphicsPipeline(GraphicsPipelineCreateParams param, VkPipelineLayout& pipelineLayout, VkPipeline& outPipeline);
  bool createPipelines(RenderContext* renderContext, RenderResourceVault* vault);

  void findDependenciesRecurse(std::vector<GraphNode>& stack, Submission* submission);

  void internalBuild2(Submission* presentSub);
  void internalBuild3();

  std::pair<VkImageLayout, VkImageLayout> findImageLayoutUsage(AccessBits prevAccess, Type prevType, AccessBits newAccess, Type newType);
  VkImageLayout findInitialImageLayout(AccessBits access, Type type);
  std::string debugConstructImageBarrierName(VkImageLayout old, VkImageLayout newLayout);

  std::pair<VkAccessFlagBits, VkAccessFlagBits> findBufferAccessFlags(AccessBits prevAccess, StageBits prevStage, AccessBits newAccess, StageBits newStage);
  std::pair<VkPipelineStageFlagBits, VkPipelineStageFlagBits> translateStageBits(StageBits prevStage, StageBits newStage);
  std::string debugConstructBufferBarrierName(VkAccessFlagBits oldAccess, VkPipelineStageFlagBits oldStage, VkAccessFlagBits newAccess, VkPipelineStageFlagBits newStage);

  void insertBarriers(std::vector<GraphNode>& stack);

  std::vector<ResourceInit> _resourceInits;
  std::vector<Submission> _submissions;
  std::vector<GraphNode> _builtGraph;
};

}