#pragma once

#include <bitset>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>

namespace render {

class RenderResourceVault;
struct IRenderResource;

// TODO: Render pass needs a RenderContext to render stuff.
typedef std::function<void(RenderResourceVault* resourceVault)> RenderPassExeFcn;

typedef std::function<void(IRenderResource* resource)> ResourceInitFcn;

typedef std::function<void(IRenderResource* resource, VkCommandBuffer& cmdBuffer, uint32_t graphicsFamilyQ)> BarrierFcn;

enum class Access
{
  Read,
  Write,
  Count
};

enum class Stage
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
  UBO,
  SSBO,
  Irrelevant,
  Count
};

typedef std::bitset<(std::size_t)Access::Count> AccessBits;
typedef std::bitset<(std::size_t)Stage::Count> StageBits;

struct ResourceUsage
{  
  AccessBits _access;
  StageBits _stage;
  Type _type;

  std::uint32_t _imageBaseLayer;

  bool _invalidateAfterRead = false;
  std::string _resourceName;
};

struct RenderPassRegisterInfo
{
  std::string _name;
  std::vector<ResourceUsage> _resourceUsages;
  bool _present = false;
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

  void reset();

  void registerResourceInitExe(const std::string& resource, ResourceUsage&& initUsage, ResourceInitFcn initFcn);

  void registerRenderPass(RenderPassRegisterInfo&& registerInfo);
  void registerRenderPassExe(const std::string& renderPass, RenderPassExeFcn exeFcn);

  void executeGraph(VkCommandBuffer& cmdBuffer);

  void build();

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
  };

  bool stackContainsProducer(const std::vector<GraphNode>& stack, const std::string& resource);
  Submission* findSubmission(const std::string& name);
  ResourceInit* findResourceInit(const std::string& resource);
  std::vector<Submission*> findResourceProducers(const std::string& resource, const std::string& excludePass);

  void findDependenciesRecurse(std::vector<GraphNode>& stack, Submission* submission);

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