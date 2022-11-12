#include "FrameGraphBuilder.h"

#include "RenderResourceVault.h"
#include "ImageHelpers.h"

#include <algorithm>
#include <cstdio>

namespace render {

namespace {

bool isTypeBuffer(Type type)
{
  return type == Type::SSBO || type == Type::UBO;
}

bool isTypeImage(Type type)
{
  return type == Type::ColorAttachment || type == Type::DepthAttachment ||
    type == Type::Present || type == Type::SampledTexture;
}

}

FrameGraphBuilder::FrameGraphBuilder()
  : _vault(nullptr)
{}

FrameGraphBuilder::FrameGraphBuilder(RenderResourceVault* resourceVault)
  : _vault(resourceVault)
{}

void FrameGraphBuilder::reset()
{
  _submissions.clear();
  _builtGraph.clear();
  _resourceInits.clear();
}

void FrameGraphBuilder::registerResourceInitExe(const std::string& resource, ResourceUsage&& initUsage, ResourceInitFcn initFcn)
{
  ResourceInit resInit;
  resInit._resource = resource;
  resInit._initFcn = initFcn;
  resInit._initUsage = std::move(initUsage);
  _resourceInits.emplace_back(std::move(resInit));
}

void FrameGraphBuilder::registerRenderPass(RenderPassRegisterInfo&& registerInfo)
{
  // Make sure this doesn't already exist
  if (findSubmission(registerInfo._name)) {
    printf("Trying to submit render pass %s, but it's already submitted!\n", registerInfo._name.c_str());
    return;
  }

  Submission sub;
  sub._regInfo = std::move(registerInfo);
  _submissions.emplace_back(std::move(sub));
}

void FrameGraphBuilder::registerRenderPassExe(const std::string& renderPass, RenderPassExeFcn exeFcn)
{
  if (!findSubmission(renderPass)) {
    printf("Trying to register a render pass (%s) exe fcn, but it doesn't exist!\n", renderPass.c_str());
    return;
  }

  auto sub = findSubmission(renderPass);
  sub->_exe = exeFcn;
}

void FrameGraphBuilder::executeGraph(VkCommandBuffer& cmdBuffer)
{
  for (auto& node : _builtGraph) {
    if (node._resourceInit) {
      auto resource = _vault->getResource(node._resourceInit.value()->_resource);
      node._resourceInit.value()->_initFcn(resource);
    }
    else if (node._barrier) {
      //node._barrier.value()();
    }
    else if (node._rpExe) {
      node._rpExe.value()(_vault);
    }
  }
}

bool FrameGraphBuilder::stackContainsProducer(const std::vector<GraphNode>& stack, const std::string& resource)
{
  for (auto& node : stack) {
    for (auto& produced : node._producedResources) {
      if (produced == resource) {
        return true;
      }
    }
  }
  return false;
}

FrameGraphBuilder::Submission* FrameGraphBuilder::findSubmission(const std::string& name)
{
  for (auto& sub : _submissions) {
    if (sub._regInfo._name == name) {
      return &sub;
    }
  }
  return nullptr;
}

FrameGraphBuilder::ResourceInit* FrameGraphBuilder::findResourceInit(const std::string& resource)
{
  for (auto& resInit : _resourceInits) {
    if (resInit._resource == resource) {
      return &resInit;
    }
  }
  return nullptr;
}

std::vector<FrameGraphBuilder::Submission*> FrameGraphBuilder::findResourceProducers(const std::string& resource, const std::string& excludePass)
{
  std::vector<Submission*> out;
  for (auto& sub : _submissions) {
    if (sub._regInfo._name == excludePass) {
      break;
    }

    for (auto& resourceUsage : sub._regInfo._resourceUsages) {
      if (resourceUsage._resourceName == resource && resourceUsage._access.test((std::size_t)Access::Write)) {
        if (excludePass != sub._regInfo._name) {
          out.emplace_back(&sub);
        }
      }
    }
  }
  return out;
}

void FrameGraphBuilder::build()
{
  /* 
  Try to figure out internal dependencies.
  
  A couple of rules:
    - If a pass _reads_ from a resource, something needs to have _written_ to it before.
    - If a resource has been invalidated after reading, we need to re-validate it by rerunning the pass that writes to it.
    - Before running a pass that writes to a resource, if it has a registered init fcn, we have to run that first.
  */

  _builtGraph.clear();

  // Start with present and work ourselves backwards
  Submission* presentSubmission = nullptr;
  for (auto& submission : _submissions) {
    if (submission._regInfo._present) {
      presentSubmission = &submission;
      break;
    }
  }

  if (!presentSubmission) {
    printf("Could not build frame graph, there is no one presenting!\n");
    return;
  }

  // Now we need to find all requirements for this submission, and recursively do the same for the children
  GraphNode presentNode{};
  presentNode._rpExe = presentSubmission->_exe;
  presentNode._debugName = presentSubmission->_regInfo._name;
  presentNode._resourceUsages = presentSubmission->_regInfo._resourceUsages;
  _builtGraph.push_back(std::move(presentNode));

  findDependenciesRecurse(_builtGraph, presentSubmission);

  std::reverse(_builtGraph.begin(), _builtGraph.end());

  // Frame graph is built, now we need to insert barriers at the appropriate positions
  insertBarriers(_builtGraph);
}

void FrameGraphBuilder::findDependenciesRecurse(std::vector<GraphNode>& stack, Submission* submission)
{
  std::vector<GraphNode> localStack;

  // Check all read resources and find who writes to them
  for (auto& resourceUsage : submission->_regInfo._resourceUsages) {
    if (resourceUsage._access.test((std::size_t)Access::Read)) {

      // If it's a persistent resource, move on, assume it will be fine
      if (_vault->isPersistentResource(resourceUsage._resourceName)) {
        continue;
      }

      // Does our local stack already contain a producer for this resource?
      if (stackContainsProducer(localStack, resourceUsage._resourceName)) {
        continue;
      }

      // Find the producers, skip ourselves and any already present producers in the stack
      auto producers = findResourceProducers(resourceUsage._resourceName, submission->_regInfo._name);
      if (producers.empty()) {
        printf("FATAL: Frame graph could not find a producer for resource %s\n", resourceUsage._resourceName.c_str());
        return;
      }

      // Choose the last producer in the chain (submission order sensitive)
      auto producer = producers.back();

      // See what else this producer produces, in case we need more resources that turn out are produced by this producer
      GraphNode node{};
      for (auto& producerUsage : producer->_regInfo._resourceUsages) {
        if (producerUsage._access.test((std::size_t)Access::Write)) {
          node._producedResources.emplace_back(producerUsage._resourceName);
        }
      }
      node._rpExe = producer->_exe;
      node._debugName = std::string(producer->_regInfo._name);
      node._resourceUsages = producer->_regInfo._resourceUsages;

      auto producersCopy = node._producedResources;

      localStack.emplace_back(std::move(node));

      // If there is a registered init (non-render pass) for the resource, run that first
      for (auto& res : producersCopy) {
        if (auto resInit = findResourceInit(res)) {

          GraphNode node{};
          node._resourceInit = resInit;
          node._resourceUsages = { resInit->_initUsage };
          node._debugName = std::string(resInit->_resource);

          localStack.emplace_back(std::move(node));
        }
      }

      findDependenciesRecurse(localStack, producer);
    }
  }

  stack.insert(stack.end(), localStack.begin(), localStack.end());
}

std::pair<VkImageLayout, VkImageLayout> FrameGraphBuilder::findImageLayoutUsage(AccessBits prevAccess, Type prevType, AccessBits newAccess, Type newType)
{
  if (prevAccess.test(std::size_t(Access::Write)) &&
      newAccess.test(std::size_t(Access::Read))) {

    if (prevType == Type::DepthAttachment && newType == Type::SampledTexture) {
      return { VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL };
    }
    else if (prevType == Type::ColorAttachment && newType == Type::SampledTexture) {
      return { VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    }
    else if (prevType == Type::ColorAttachment && newType == Type::ColorAttachment) {
      return { VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    }
    else if (prevType == Type::ColorAttachment && newType == Type::Present) {
      return { VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR };
    }
  }

  return {};
}

VkImageLayout FrameGraphBuilder::findInitialImageLayout(AccessBits access, Type type)
{
  if (access.test(std::size_t(Access::Write))) {
    if (type == Type::DepthAttachment) {
      return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }
    else if (type == Type::ColorAttachment) {
      return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
  }

  return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
}

std::string FrameGraphBuilder::debugConstructImageBarrierName(VkImageLayout old, VkImageLayout newLayout)
{
  std::string output;
  std::string from, to;

  if (old == VK_IMAGE_LAYOUT_UNDEFINED) {
    from = "UNDEFINED";
  }
  else if (old == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
    from = "COLOR_ATTACHMENT_OPTIMAL";
  }
  else if (old == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) {
    from = "DEPTH_ATTACHMENT_OPTIMAL";
  }
  else if (old == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    from = "DEPTH_STENCIL_ATTACHMENT_OPTIMAL";
  }

  if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL) {
    to = "DEPTH_STENCIL_READ_ONLY_OPTIMAL";
  }
  else if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    to = "DEPTH_STENCIL_ATTACHMENT_OPTIMAL";
  }
  else if (newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
    to = "COLOR_ATTACHMENT_OPTIMAL";
  }
  else if (newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    to = "SHADER_READ_ONLY_OPTIMAL";
  }
  else if (newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
    to = "PRESENT_SRC_KHR";
  }

  output.append("Barrier: FROM ");
  output.append(from);
  output.append(" TO ");
  output.append(to);
  return output;
}

std::pair<VkAccessFlagBits, VkAccessFlagBits> FrameGraphBuilder::findBufferAccessFlags(AccessBits prevAccess, StageBits prevStage, AccessBits newAccess, StageBits newStage)
{
  if (prevAccess.test((std::size_t)Access::Write)) {
    if (prevStage.test((std::size_t)Stage::Transfer)) {

      if (newAccess.test((std::size_t)Access::Write)) {
        // Going from transfer (probably from staging buffer) to rw in a shader
        if (newStage.test((std::size_t)Stage::Compute)) {
          return { VK_ACCESS_TRANSFER_WRITE_BIT, (VkAccessFlagBits)(VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT) };
        }
      }
      else if (newAccess.test((std::size_t)Access::Read)) {
        if (newStage.test((std::size_t)Stage::Compute)) {
          return { VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT };
        }
      }
    }
    else if (prevStage.test((std::size_t)Stage::Compute)) {

      if (newAccess.test((std::size_t)Access::Read)) {
        if (newStage.test((std::size_t)Stage::IndirectDraw)) {
          // Going from compute output to indirect drawing
          return { VK_ACCESS_SHADER_WRITE_BIT, (VkAccessFlagBits)(VK_ACCESS_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_SHADER_READ_BIT) };
        }
        else if (newStage.test((std::size_t)Stage::Vertex)) {
          return { VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT };
        }
        else if (newStage.test((std::size_t)Stage::Compute)) {
          return { VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT };
        }
      }
    }
    else if (prevStage.test((std::size_t)Stage::Fragment)) {

      if (newAccess.test((std::size_t)Access::Read)) {
        if (newStage.test((std::size_t)Stage::Compute)) {
          return { VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT };
        }
      }
    }
  }

  return std::pair<VkAccessFlagBits, VkAccessFlagBits>();
}

std::pair<VkPipelineStageFlagBits, VkPipelineStageFlagBits> FrameGraphBuilder::translateStageBits(StageBits prevStage, StageBits newStage)
{
  VkPipelineStageFlagBits prev;
  VkPipelineStageFlagBits next;

  if (prevStage.test((std::size_t)Stage::Transfer)) {
    prev = VK_PIPELINE_STAGE_TRANSFER_BIT;
  }
  else if (prevStage.test((std::size_t)Stage::Compute)) {
    prev = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
  }
  else if (prevStage.test((std::size_t)Stage::IndirectDraw)) {
    prev = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
  }
  else if (prevStage.test((std::size_t)Stage::Vertex)) {
    prev = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
  }
  else if (prevStage.test((std::size_t)Stage::Fragment)) {
    prev = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  }

  if (newStage.test((std::size_t)Stage::Transfer)) {
    next = VK_PIPELINE_STAGE_TRANSFER_BIT;
  }
  else if (newStage.test((std::size_t)Stage::Compute)) {
    next = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
  }
  else if (newStage.test((std::size_t)Stage::IndirectDraw)) {
    next = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
  }
  else if (newStage.test((std::size_t)Stage::Vertex)) {
    next = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
  }
  else if (newStage.test((std::size_t)Stage::Fragment)) {
    next = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  }

  return std::pair<VkPipelineStageFlagBits, VkPipelineStageFlagBits>(prev, next);
}

std::string FrameGraphBuilder::debugConstructBufferBarrierName(VkAccessFlagBits oldAccess, VkPipelineStageFlagBits oldStage, VkAccessFlagBits newAccess, VkPipelineStageFlagBits newStage)
{
  std::string output;
  std::string dstAccess, srcAccess, dstStage, srcStage;

  if (oldAccess & VK_ACCESS_TRANSFER_WRITE_BIT) {
    srcAccess.append(" | TRANSFER_WRITE");
  }
  if (oldAccess & VK_ACCESS_SHADER_WRITE_BIT) {
    srcAccess.append(" | SHADER_WRITE");
  }
  if (oldAccess & VK_ACCESS_SHADER_READ_BIT) {
    srcAccess.append(" | SHADER_READ");
  }
  if (oldAccess & VK_ACCESS_INDIRECT_COMMAND_READ_BIT) {
    srcAccess.append(" | INDIRECT_COMMAND_READ");
  }
  if (newAccess & VK_ACCESS_TRANSFER_WRITE_BIT) {
    dstAccess.append(" | TRANSFER_WRITE");
  }
  if (newAccess & VK_ACCESS_SHADER_WRITE_BIT) {
    dstAccess.append(" | SHADER_WRITE");
  }
  if (newAccess & VK_ACCESS_SHADER_READ_BIT) {
    dstAccess.append(" | SHADER_READ");
  }
  if (newAccess & VK_ACCESS_INDIRECT_COMMAND_READ_BIT) {
    dstAccess.append(" | INDIRECT_COMMAND_READ");
  }

  if (oldStage & VK_PIPELINE_STAGE_TRANSFER_BIT) {
    srcStage.append(" | TRANSFER");
  }
  if (oldStage & VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT) {
    srcStage.append(" | COMPUTE");
  }
  if (oldStage & VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT) {
    srcStage.append(" | DRAW_INDIRECT");
  }
  if (oldStage & VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT) {
    srcStage.append(" | FRAGMENT");
  }
  if (oldStage & VK_PIPELINE_STAGE_VERTEX_SHADER_BIT) {
    srcStage.append(" | VERTEX");
  }
  if (oldStage & VK_PIPELINE_STAGE_VERTEX_SHADER_BIT) {
    srcStage.append(" | VERTEX");
  }
  if (newStage & VK_PIPELINE_STAGE_TRANSFER_BIT) {
    dstStage.append(" | TRANSFER");
  }
  if (newStage & VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT) {
    dstStage.append(" | COMPUTE");
  }
  if (newStage & VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT) {
    dstStage.append(" | DRAW_INDIRECT");
  }
  if (newStage & VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT) {
    dstStage.append(" | FRAGMENT");
  }
  if (newStage & VK_PIPELINE_STAGE_VERTEX_SHADER_BIT) {
    dstStage.append(" | VERTEX");
  }
  if (newStage & VK_PIPELINE_STAGE_VERTEX_SHADER_BIT) {
    dstStage.append(" | VERTEX");
  }

  output.append("Buffer barrier: ");
  output.append("SRCACC: ");
  output.append(srcAccess);
  output.append(", DSTACC: ");
  output.append(dstAccess); 
  output.append(", SRCSTAGE: ");
  output.append(srcStage); 
  output.append(", DSTSTAGE: ");
  output.append(dstStage);

  return output;
}

void FrameGraphBuilder::insertBarriers(std::vector<GraphNode>& stack)
{
  std::vector<GraphNode> updatedStack;

  // The supplied stack is already in correct order, go through each node and check how
  // the next node uses the produced resources.
  for (std::size_t i = 0; i < stack.size() - 1; ++i) {
    auto& node = stack[i];

    updatedStack.emplace_back(node);

    // Keep track of where the current node is placed in the updatedStack
    std::size_t currentNodeIdx = updatedStack.size() - 1;

    for (auto& usage : node._resourceUsages) {

      // Find (maybe) a previous usage
      ResourceUsage* prevUsage = nullptr;
      for (std::size_t j = 0; j < i; ++j) {
        auto& prevNode = stack[j];
        for (auto& prev : prevNode._resourceUsages) {
          if (prev._resourceName == usage._resourceName) {
            prevUsage = &prev;
            break;
          }
        }
        // If we found something (or called break) in inner loop, break outer loop
        if (prevUsage) {
          break;
        }
      }

      // Find next nodes equivalent of the resource
      ResourceUsage* nextUsage = nullptr;
      for (std::size_t j = i + 1; j < stack.size(); ++j) {
        auto& nextNode = stack[j];
        for (auto& next : nextNode._resourceUsages) {
          if (next._resourceName == usage._resourceName) {
            nextUsage = &next;
            break;
          }
        }
        // If we found something (or called break) in inner loop, break outer loop
        if (nextUsage) {
          break;
        }
      }

      if (!nextUsage) {
        // Doesn't use this resource, continue
        continue;
      }

      // Do either a execution barrier, buffer memory barrier or image transition
      if (isTypeImage(usage._type)) {
        // If the _previous_ usage wrote to this image, and we now read from it, and the type is the same, skip the pre-barrier
        bool skipPreBarrier = false;
        if (prevUsage) {
          if (prevUsage->_access.test((std::size_t)Access::Write) && prevUsage->_type == usage._type) {
            skipPreBarrier = true;
          }
        }

        std::uint32_t baseLayer = usage._imageBaseLayer;

        // Initial layout transition (before writing)
        if (!skipPreBarrier) {
          auto initialLayout = findInitialImageLayout(usage._access, usage._type);

          BarrierContext beforeWriteBarrier{};
          beforeWriteBarrier._resourceName = usage._resourceName;
          beforeWriteBarrier._barrierFcn = [baseLayer, initialLayout](IRenderResource* resource, VkCommandBuffer& cmdBuffer, uint32_t) {
            auto imageResource = dynamic_cast<ImageRenderResource*>(resource);

            imageutil::transitionImageLayout(
              cmdBuffer,
              imageResource->_image._image,
              imageResource->_format,
              VK_IMAGE_LAYOUT_UNDEFINED,
              initialLayout,
              baseLayer);
          };

          // Add the pre-write barrier _before_ writing
          GraphNode beforeWriteNode{};
          auto beforeName = debugConstructImageBarrierName(VK_IMAGE_LAYOUT_UNDEFINED, initialLayout);
          beforeWriteNode._debugName = "Before write " + usage._resourceName + " (" + beforeName + ")";
          beforeWriteNode._barrier = std::move(beforeWriteBarrier);

          updatedStack.insert(updatedStack.begin() + currentNodeIdx, std::move(beforeWriteNode));
          // The currentNodeIdx has now been updated (since we added this node _before_)
          currentNodeIdx++;
        }

        // Add the post-write transition here
        BarrierContext afterWriteBarrier{};
        afterWriteBarrier._resourceName = usage._resourceName;

        // Gather some metadata about the image resource usage
        auto transitionPair = findImageLayoutUsage(usage._access, usage._type, nextUsage->_access, nextUsage->_type);

        // If the next usage is the same as the current, no need to have a barrier...?
        // Probably an execution barrier is still needed?
        if (transitionPair.first != transitionPair.second) {
          afterWriteBarrier._barrierFcn = [transitionPair, baseLayer](IRenderResource* resource, VkCommandBuffer& cmdBuffer, uint32_t) {
            auto imageResource = dynamic_cast<ImageRenderResource*>(resource);

            imageutil::transitionImageLayout(
              cmdBuffer,
              imageResource->_image._image,
              imageResource->_format,
              transitionPair.first,
              transitionPair.second,
              baseLayer);
          };

          GraphNode afterWriteNode{};
          auto afterName = debugConstructImageBarrierName(transitionPair.first, transitionPair.second);
          afterWriteNode._debugName = "After write " + usage._resourceName + " (" + afterName + ")";
          afterWriteNode._barrier = std::move(afterWriteBarrier);

          // Add this after the current idx
          updatedStack.insert(updatedStack.begin() + currentNodeIdx + 1, std::move(afterWriteNode));
        }
      }
      else if (isTypeBuffer(usage._type)) {
        auto accessFlagPair = findBufferAccessFlags(usage._access, usage._stage, nextUsage->_access, nextUsage->_stage);
        auto transStagePair = translateStageBits(usage._stage, nextUsage->_stage);

        BarrierContext bufBarrier{};
        bufBarrier._resourceName = usage._resourceName;
        bufBarrier._barrierFcn = [accessFlagPair, transStagePair](IRenderResource* resource, VkCommandBuffer& cmdBuffer, uint32_t graphicsFamilyQ) {
          BufferRenderResource* buf = dynamic_cast<BufferRenderResource*>(resource);

          VkBufferMemoryBarrier memBarr{};
          memBarr.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
          memBarr.srcAccessMask = accessFlagPair.first;
          memBarr.dstAccessMask = accessFlagPair.second;
          memBarr.srcQueueFamilyIndex = graphicsFamilyQ;
          memBarr.dstQueueFamilyIndex = graphicsFamilyQ;
          memBarr.buffer = buf->_buffer._buffer;
          memBarr.offset = 0;
          memBarr.size = VK_WHOLE_SIZE;

          vkCmdPipelineBarrier(
            cmdBuffer,
            transStagePair.first,
            transStagePair.second,
            0, 0, nullptr,
            1, &memBarr,
            0, nullptr);
        };

        GraphNode bufBarrNode{};
        bufBarrNode._debugName = std::string("(") + std::string(usage._resourceName) + std::string(") ") + debugConstructBufferBarrierName(accessFlagPair.first, transStagePair.first, accessFlagPair.second, transStagePair.second);
        bufBarrNode._barrier = std::move(bufBarrier);

        // This needs to be added after the current node (because it protects the next operation)
        updatedStack.insert(updatedStack.begin() + currentNodeIdx + 1, std::move(bufBarrNode));
      }
      // TODO: Execution barrier when doing init of resources (?)
      //       Basically write-after-read needs execution barrier
    }
  }

  updatedStack.emplace_back(stack.back());
  std::swap(updatedStack, stack);
}

void FrameGraphBuilder::printBuiltGraphDebug()
{
  printf("\n---Frame graph debug print start---\n");
  if (_builtGraph.empty()) {
    printf("Frame graph is empty!\n");
    return;
  }

  for (auto& node : _builtGraph) {
    if (node._barrier) {
      printf("\t%s\n\n", node._debugName.c_str());
    }
    else if (node._resourceInit) {
      printf("\tResource init: %s\n\n", node._debugName.c_str());
    }
    else if (node._rpExe) {
      printf("\tRender pass: %s\n", node._debugName.c_str());
      if (!node._producedResources.empty()) {
        printf("\t\tThese resources were produced: \n");
        for (auto& res : node._producedResources) {
          printf("\t\t\tResource: %s\n", res.c_str());
        }
      }
      printf("\n");
    }
  }

  printf("\n---Frame graph debug print end---\n");
}

}