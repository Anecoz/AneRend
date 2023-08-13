#include "FrameGraphBuilder.h"

#include "RenderResourceVault.h"
#include "ImageHelpers.h"
#include "RenderContext.h"
#include "BufferHelpers.h"
#include "ImageHelpers.h"
#include "../util/Utils.h"

#include <algorithm>
#include <cstdio>
#include <iterator>

namespace render {

namespace {

bool isTypeBuffer(Type type)
{
  return type == Type::SSBO || type == Type::UBO;
}

bool isTypeImage(Type type)
{
  return type == Type::ColorAttachment || type == Type::DepthAttachment || type == Type::SampledDepthTexture ||
    type == Type::Present || type == Type::SampledTexture || type == Type::ImageStorage || type == Type::ImageTransferDst || type == Type::ImageTransferSrc;
}

bool isDepth(Type type)
{
  return type == Type::DepthAttachment || type == Type::SampledDepthTexture;
}

bool usesSameMips(const ResourceUsage& a, const ResourceUsage& b)
{
  if (a._allMips && b._allMips) {
    return true;
  }

  if (!a._allMips && b._allMips) {
    return true;
  }

  if (a._allMips && !b._allMips) {
    return true;
  }

  // if we get here, there is no allMips set in either a or b
  return a._mip == b._mip;
}

VkAccessFlags findWriteAccessMask(Type type)
{
  if (type == Type::ImageStorage) {
    return VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
  }
  else if (type == Type::ColorAttachment) {
    return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  }
  else if (type == Type::DepthAttachment) {
    return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;;
  }
  else if (type == Type::SampledTexture || type == Type::SampledDepthTexture) {
    return VK_ACCESS_SHADER_READ_BIT;
  }
  else if (type == Type::Present) {
    return VK_ACCESS_TRANSFER_READ_BIT;
  }
  else if (type == Type::ImageTransferSrc) {
    return VK_ACCESS_TRANSFER_READ_BIT;
  }
  else if (type == Type::ImageTransferDst) {
    return VK_ACCESS_TRANSFER_WRITE_BIT;
  }

  return VK_ACCESS_SHADER_READ_BIT;
}

VkDescriptorType translateDescriptorType(Type type)
{
  if (type == Type::SSBO) {
    return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  }
  if (type == Type::UBO) {
    return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  }
  if (type == Type::SampledTexture) {
    return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  }
  if (type == Type::SampledDepthTexture) {
    return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  }
  if (type == Type::ImageStorage) {
    return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  }

  return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
}

VkImageLayout translateImageLayout(Type type)
{
  if (type == Type::SampledTexture) {
    return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  }
  if (type == Type::SampledDepthTexture) {
    return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
  }
  if (type == Type::ImageStorage) {
    return VK_IMAGE_LAYOUT_GENERAL;
  }

  return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

bool shouldBeDescriptor(Type type)
{
  if (type == Type::DepthAttachment ||
    type == Type::ColorAttachment ||
    type == Type::Present ||
    type == Type::ImageTransferDst ||
    type == Type::ImageTransferSrc) {
    return false;
  }

  return true;
}

bool shouldBeDescriptor(StageBits stageBits)
{
  if (stageBits.test((std::size_t)Stage::IndirectDraw)) {
    return false;
  }

  return true;
}

}

FrameGraphBuilder::FrameGraphBuilder()
  : _vault(nullptr)
{}

FrameGraphBuilder::FrameGraphBuilder(RenderResourceVault* resourceVault)
  : _vault(resourceVault)
{}

void FrameGraphBuilder::reset(RenderContext* rc)
{
  // Delete all objects associated with the nodes
  for (auto& node : _builtGraph) {
    if (!node._rpExe.has_value()) continue;

    vkDestroyDescriptorSetLayout(rc->device(), node._descriptorSetLayout, nullptr);
    vkDestroyPipelineLayout(rc->device(), node._pipelineLayout, nullptr);
    vkDestroyPipeline(rc->device(), node._pipeline, nullptr);

    for (auto& sampler : node._samplers) {
      vkDestroySampler(rc->device(), sampler, nullptr);
    }

    if (node._sbt._buffer._buffer != VK_NULL_HANDLE) {
      vmaDestroyBuffer(rc->vmaAllocator(), node._sbt._buffer._buffer, node._sbt._buffer._allocation);
    }
  }

  _submissions.clear();
  _builtGraph.clear();
  _resourceInits.clear();
}

void FrameGraphBuilder::registerResourceInitExe(const std::string& resource, ResourceUsage&& initUsage, ResourceInitFcn initFcn)
{
  ResourceInit resInit;
  resInit._resource = resource;
  initUsage._resourceName = resource;
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

void FrameGraphBuilder::executeGraph(VkCommandBuffer& cmdBuffer, RenderContext* renderContext)
{
  for (auto& node : _builtGraph) {
    if (node._resourceInit) {
      auto resource = _vault->getResource(node._resourceInit.value()->_resource, renderContext->getCurrentMultiBufferIdx());
      node._resourceInit.value()->_initFcn(resource, cmdBuffer, renderContext);
    }
    else if (node._barrier) {
      auto resource = _vault->getResource(node._barrier->_resourceName, renderContext->getCurrentMultiBufferIdx());
      node._barrier.value()._barrierFcn(resource, cmdBuffer);
    }
    else if (node._rpExe) {
      // Find parameters for this exe
      RenderExeParams exeParams{};
      exeParams.cmdBuffer = &cmdBuffer;
      exeParams.rc = renderContext;
      exeParams.vault = _vault;
      exeParams.pipeline = &node._pipeline;
      exeParams.pipelineLayout = &node._pipelineLayout;
      exeParams.descriptorSets = &node._descriptorSets;
      exeParams.samplers = node._samplers;
      exeParams.sbt = &node._sbt;

      // Views and buffers
      int multiIdx = renderContext->getCurrentMultiBufferIdx();
      for (auto& us : node._resourceUsages) {
        if (us._type == Type::ColorAttachment) {
          auto view = ((ImageRenderResource*)_vault->getResource(us._resourceName))->_views[0];
          exeParams.colorAttachmentViews.emplace_back(view);
        }
        else if (us._type == Type::DepthAttachment) {
          auto view = ((ImageRenderResource*)_vault->getResource(us._resourceName))->_views[0];
          exeParams.depthAttachmentViews.emplace_back(view);
        }
        else if (us._type == Type::SSBO) {
          auto buf = ((BufferRenderResource*)_vault->getResource(us._resourceName, multiIdx))->_buffer._buffer;
          exeParams.buffers.emplace_back(buf);
        }
        else if (us._type == Type::Present) {
          exeParams.presentImage = ((ImageRenderResource*)_vault->getResource(us._resourceName))->_image._image;
        }
        else if (us._type == Type::ImageStorage) {
          auto image = ((ImageRenderResource*)_vault->getResource(us._resourceName))->_image._image;
          exeParams.images.emplace_back(image);
        }
        else if (us._type == Type::SampledTexture) {
          auto image = ((ImageRenderResource*)_vault->getResource(us._resourceName))->_image._image;
          exeParams.images.emplace_back(image);
        }
        else if (us._type == Type::SampledDepthTexture) {
          auto image = ((ImageRenderResource*)_vault->getResource(us._resourceName))->_image._image;
          exeParams.images.emplace_back(image);
        }
        else if (us._type == Type::ImageTransferSrc) {
          auto image = ((ImageRenderResource*)_vault->getResource(us._resourceName))->_image._image;
          exeParams.images.emplace_back(image);
        }
        else if (us._type == Type::ImageTransferDst) {
          auto image = ((ImageRenderResource*)_vault->getResource(us._resourceName))->_image._image;
          exeParams.images.emplace_back(image);
        }
      }

      renderContext->startTimer(node._debugName, cmdBuffer);
      node._rpExe.value()(exeParams);
      renderContext->stopTimer(node._debugName, cmdBuffer);
    }
  }
}

bool FrameGraphBuilder::stackContainsProducer(std::vector<GraphNode>& stack, const std::string& resource, GraphNode** nodeOut)
{
  for (auto& node : stack) {
    for (auto& produced : node._producedResources) {
      if (produced == resource) {
        *nodeOut = &node;
        return true;
      }
    }
  }
  return false;
}

void FrameGraphBuilder::ensureOrder(std::vector<GraphNode>& stack, const std::string& nameBefore, const std::string& nameAfter)
{
  if (stack.size() <= 1) return;

  // Make sure nameBefore comes before nameAfter
  int afterIdx = -1;
  int beforeIdx = -1;
  bool doSwap = false;
  for (int i = 0; i < stack.size(); ++i) {
    auto& node = stack[i];

    if (node._debugName == nameBefore) {
      beforeIdx = i;
    }
    if (node._debugName == nameAfter) {
      afterIdx = i;
    }

    if (afterIdx != -1) {
      if (beforeIdx != -1) {
        if (doSwap) {
          // We just found before, do swap
          std::iter_swap(stack.begin() + beforeIdx, stack.begin() + afterIdx);
          return;
        }
        else {
          // All is good
          return;
        }
      }
      else {
        doSwap = true;
      }
    }
  }
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

bool FrameGraphBuilder::build(RenderContext* renderContext, RenderResourceVault* vault)
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
    return false;
  }

  // Do a pass of all submissions and figure out what resources need to be created
  if (!createResources(renderContext, vault)) {
    printf("Could not create resources for frame graph!\n");
    return false;
  }

  //internalBuild2(presentSubmission);
  internalBuild3();

  // Insert timers
  for (auto& node : _builtGraph) {
    if (node._rpExe) {
      renderContext->registerPerFrameTimer(node._debugName);
    }
  }

  // Do another pass and create all pipelines, pipelinelayouts, descriptor set layouts and descriptor sets
  // that each render pass needs
  if (!createPipelines(renderContext, vault)) {
    printf("Could not create pipelines for frame graph!\n");
    return false;
  }

  // Now we need to find all requirements for this submission, and recursively do the same for the children
  /*GraphNode presentNode{};
  presentNode._rpExe = presentSubmission->_exe;
  presentNode._debugName = presentSubmission->_regInfo._name;
  presentNode._resourceUsages = presentSubmission->_regInfo._resourceUsages;
  _builtGraph.push_back(std::move(presentNode));

  findDependenciesRecurse(_builtGraph, presentSubmission);

  std::reverse(_builtGraph.begin(), _builtGraph.end());*/

  // Frame graph is built, now we need to insert barriers at the appropriate positions
  insertBarriers(_builtGraph);

  // TODO: Barrier insertion particularly for buffers is overly ambitious... 
  //       Should be able to go through after insertions and tidy it up. Execution
  //       barriers for instance can be treacherous. 

  return true;
}

std::vector<VkBufferUsageFlagBits> FrameGraphBuilder::findBufferCreateFlags(const std::string& bufferResource)
{
  std::vector<VkBufferUsageFlagBits> output;

  for (auto& sub : _submissions) {
    for (auto& us : sub._regInfo._resourceUsages) {
      if (us._resourceName == bufferResource) {
        if (us._type == Type::SSBO) {
          output.emplace_back(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        }
        if (us._type == Type::UBO) {
          output.emplace_back(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
        }
        if (us._stage.test((std::size_t)Stage::IndirectDraw)) {
          output.emplace_back(VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);
        }
        if (us._stage.test((std::size_t)Stage::Transfer)) {
          output.emplace_back(VK_BUFFER_USAGE_TRANSFER_DST_BIT);
        }
      }
    }
  }
  for (auto& init : _resourceInits) {
    if (init._resource == bufferResource) {
      if (init._initUsage._type == Type::SSBO) {
        output.emplace_back(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
      }
      if (init._initUsage._stage.test((std::size_t)Stage::IndirectDraw)) {
        output.emplace_back(VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);
      }
      if (init._initUsage._stage.test((std::size_t)Stage::Transfer)) {
        output.emplace_back(VK_BUFFER_USAGE_TRANSFER_DST_BIT);
      }
    }
  }

  // Uniqueify
  std::sort(output.begin(), output.end());
  output.erase(std::unique(output.begin(), output.end()), output.end());

  return output;
}

std::vector<VkImageUsageFlagBits> FrameGraphBuilder::findImageCreateFlags(const std::string& resource)
{
  std::vector<VkImageUsageFlagBits> output;

  for (auto& sub : _submissions) {
    for (auto& us : sub._regInfo._resourceUsages) {
      if (us._resourceName == resource) {
        if (us._type == Type::DepthAttachment) {
          output.emplace_back(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
        }
        if (us._type == Type::ColorAttachment) {
          output.emplace_back(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
        }
        if (us._type == Type::SampledTexture || us._type == Type::SampledDepthTexture) {
          output.emplace_back(VK_IMAGE_USAGE_SAMPLED_BIT);
        }
        if (us._type == Type::ImageStorage) {
          output.emplace_back(VK_IMAGE_USAGE_STORAGE_BIT);
        }
        if (us._type == Type::Present) {
          output.emplace_back(VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
        }
        if (us._type == Type::ImageTransferDst || us._type == Type::ImageTransferSrc) {
          output.emplace_back(VK_IMAGE_USAGE_TRANSFER_DST_BIT);
          output.emplace_back(VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
        }
      }
    }
  }
  for (auto& init : _resourceInits) {
    if (init._resource == resource) {
      if (init._initUsage._type == Type::DepthAttachment) {
        output.emplace_back(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
      }
      if (init._initUsage._type == Type::ColorAttachment) {
        output.emplace_back(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
      }
      if (init._initUsage._type == Type::SampledTexture || init._initUsage._type == Type::SampledDepthTexture) {
        output.emplace_back(VK_IMAGE_USAGE_SAMPLED_BIT);
      }
      if (init._initUsage._type == Type::ImageStorage) {
        output.emplace_back(VK_IMAGE_USAGE_STORAGE_BIT);
      }
      if (init._initUsage._type == Type::Present) {
        output.emplace_back(VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
      }
    }
  }

  // Uniqueify
  std::sort(output.begin(), output.end());
  output.erase(std::unique(output.begin(), output.end()), output.end());

  return output;
}

bool FrameGraphBuilder::createResources(RenderContext* renderContext, RenderResourceVault* vault)
{
  auto allocator = renderContext->vmaAllocator();

  std::vector<std::string> createdResources;
  std::vector<std::string> foundResources;

  for (auto& sub : _submissions) {
    for (auto& usage : sub._regInfo._resourceUsages) {
      foundResources.emplace_back(usage._resourceName);

      // If this resource is not created
      if (std::find(createdResources.begin(), createdResources.end(), usage._resourceName) == createdResources.end()) {

        if (isTypeBuffer(usage._type)) {
          if (usage._bufferCreateInfo.has_value()) {
            auto flags = findBufferCreateFlags(usage._resourceName);

            VkBufferUsageFlagBits flag{};
            for (auto f : flags) {
              flag = VkBufferUsageFlagBits(flag | f);
            }

            VmaAllocationCreateFlags createFlags = 0;
            if (usage._bufferCreateInfo->_hostWritable) {
              createFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
            }

            int num = usage._bufferCreateInfo->_multiBuffered ? renderContext->getMultiBufferSize() : 1;

            for (int i = 0; i < num; ++i) {
              auto buf = new BufferRenderResource();
              bufferutil::createBuffer(
                allocator,
                usage._bufferCreateInfo->_initialSize,
                flag,
                createFlags,
                buf->_buffer);

              std::string name = usage._resourceName + "_" + std::to_string(i);
              renderContext->setDebugName(VK_OBJECT_TYPE_BUFFER, (uint64_t)buf->_buffer._buffer, name.c_str());

              if (usage._bufferCreateInfo->_initialDataCb) {
                usage._bufferCreateInfo->_initialDataCb(renderContext, buf->_buffer);
              }

              vault->addResource(usage._resourceName, std::unique_ptr<IRenderResource>(buf), usage._bufferCreateInfo->_multiBuffered, i);
              createdResources.emplace_back(usage._resourceName);

              usage._multiBuffered = usage._bufferCreateInfo->_multiBuffered;
            }
          }
        }
        else if (isTypeImage(usage._type)) {
          if (usage._imageCreateInfo.has_value()) {
            auto flags = findImageCreateFlags(usage._resourceName);

            VkImageUsageFlagBits flag{};
            for (auto f : flags) {
              flag = VkImageUsageFlagBits(flag | f);
            }

            if (usage._imageCreateInfo->_initialDataCb) {
              flag = VkImageUsageFlagBits(flag | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
            }

            int num = usage._imageCreateInfo->_multiBuffered ? renderContext->getMultiBufferSize() : 1;

            for (int i = 0; i < num; ++i) {
              auto im = new ImageRenderResource();
              im->_format = usage._imageCreateInfo->_intialFormat;
              imageutil::createImage(
                usage._imageCreateInfo->_initialWidth,
                usage._imageCreateInfo->_initialHeight,
                usage._imageCreateInfo->_intialFormat,
                VK_IMAGE_TILING_OPTIMAL,
                renderContext->vmaAllocator(),
                flag,
                im->_image,
                usage._imageCreateInfo->_mipLevels
              );

              // Do a transition if we're not undefined from the start (this is hacky...)
              if (usage._imageCreateInfo->_initialLayout != VK_IMAGE_LAYOUT_UNDEFINED) {
                auto cmdBuffer = renderContext->beginSingleTimeCommands();
                imageutil::transitionImageLayout(
                  cmdBuffer,
                  im->_image._image,
                  im->_format,
                  VK_IMAGE_LAYOUT_UNDEFINED,
                  usage._imageCreateInfo->_initialLayout,
                  0,
                  usage._imageCreateInfo->_mipLevels
                );
                renderContext->endSingleTimeCommands(cmdBuffer);
              }

              im->_views.emplace_back(imageutil::createImageView(
                renderContext->device(), 
                im->_image._image,
                im->_format,
                usage._type == Type::DepthAttachment ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT,
                0,
                usage._imageCreateInfo->_mipLevels));

              // Create separate views for each mip (if there are any more)
              if (usage._imageCreateInfo->_mipLevels > 1) {
                for (int j = 0; j < usage._imageCreateInfo->_mipLevels; ++j) {
                  im->_views.emplace_back(imageutil::createImageView(
                    renderContext->device(),
                    im->_image._image,
                    im->_format,
                    usage._type == Type::DepthAttachment ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT,
                    j,
                    1));

                  std::string viewName = usage._resourceName + "View" + std::to_string(i) + "_mip_" + std::to_string(j);
                  renderContext->setDebugName(VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)im->_views[j+1], viewName.c_str());
                }
              }

              std::string name = usage._resourceName + "_" + std::to_string(i);
              std::string viewName = usage._resourceName + "View" + std::to_string(i);
              renderContext->setDebugName(VK_OBJECT_TYPE_IMAGE, (uint64_t)im->_image._image, name.c_str());
              renderContext->setDebugName(VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)im->_views[0], viewName.c_str());

              if (usage._imageCreateInfo->_initialDataCb) {
                usage._imageCreateInfo->_initialDataCb(renderContext, im->_image._image);
              }

              vault->addResource(usage._resourceName, std::unique_ptr<IRenderResource>(im), usage._imageCreateInfo->_multiBuffered, i);
              createdResources.emplace_back(usage._resourceName);

              usage._multiBuffered = usage._imageCreateInfo->_multiBuffered;
            }
          }
        }

      }
    }
  }

  // If we didn't create all resources that we found, means we missed something, which is fatal
  std::sort(createdResources.begin(), createdResources.end());
  createdResources.erase(std::unique(createdResources.begin(), createdResources.end()), createdResources.end());

  std::sort(foundResources.begin(), foundResources.end());
  foundResources.erase(std::unique(foundResources.begin(), foundResources.end()), foundResources.end());

  // Should probably check that they are actually equal...
  if (createdResources.empty()) {
    return false;
  }

  /*if (createdResources.size() != foundResources.size()) {
    return false;
  }

  for (std::size_t i = 0; i < createdResources.size(); ++i) {
    if (createdResources[i] != foundResources[i]) return false;
  }*/

  return true;
}

VkShaderStageFlags FrameGraphBuilder::findStages(const std::string& resource)
{
  std::vector<VkShaderStageFlagBits> output;

  for (auto& sub : _submissions) {
    for (auto& us : sub._regInfo._resourceUsages) {
      if (us._resourceName == resource) {
        if (us._stage.test((std::size_t)Stage::Vertex)) {
          output.emplace_back(VK_SHADER_STAGE_VERTEX_BIT);
        }
        if (us._stage.test((std::size_t)Stage::Compute)) {
          output.emplace_back(VK_SHADER_STAGE_COMPUTE_BIT);
        }
        if (us._stage.test((std::size_t)Stage::Fragment)) {
          output.emplace_back(VK_SHADER_STAGE_FRAGMENT_BIT);
        }
        if (us._stage.test((std::size_t)Stage::RayTrace)) {
          output.emplace_back(VK_SHADER_STAGE_RAYGEN_BIT_KHR);
          output.emplace_back(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
        }
      }
    }
  }

  // Uniqueify
  std::sort(output.begin(), output.end());
  output.erase(std::unique(output.begin(), output.end()), output.end());

  VkShaderStageFlags flags{};
  for (auto f : output) {
    flags = VkShaderStageFlagBits(flags | f);
  }

  return flags;
}

bool FrameGraphBuilder::createPipelines(RenderContext* renderContext, RenderResourceVault* vault)
{
  // Go through each node in the built graph and build all needed
  for (auto& node : _builtGraph) {
    // Only do this if we are a render pass
    if (!node._rpExe.has_value()) {
      continue;
    }

    // Do a pre-run to find the highest binding (will be used as bindless binding)
    bool containsBindless = false;
    int highestBinding = 0;
    for (auto& us : node._resourceUsages) {
      if ((isTypeBuffer(us._type) && shouldBeDescriptor(us._stage)) ||
          (isTypeImage(us._type) && shouldBeDescriptor(us._type)) && !us._bindless) {
        highestBinding++;
      }

      if (us._bindless) containsBindless = true;
    }

    // Start with descriptor set layout and pipeline layout
    VkDescriptorSetLayout descLayout{};
    VkPipelineLayout pipeLayout{};

    DescriptorSetLayoutCreateParams descLayoutParam{};
    descLayoutParam.renderContext = renderContext;

    int currBinding = 0;
    bool bindlessAdded = false;
    for (int i = 0; i < node._resourceUsages.size(); ++i) {
      auto& usage = node._resourceUsages[i];

      if ((isTypeBuffer(usage._type) && shouldBeDescriptor(usage._stage)) ||
        (isTypeImage(usage._type) && shouldBeDescriptor(usage._type)) && !bindlessAdded) {
          DescriptorBindInfo bindInfo{};
          bindInfo.binding = usage._bindless ? highestBinding : currBinding;
          bindInfo.stages = findStages(usage._resourceName);
          bindInfo.type = translateDescriptorType(usage._type);

          if (usage._bindless) {
            bindlessAdded = true;
            bindInfo.bindless = true;
          }

          descLayoutParam.bindInfos.emplace_back(bindInfo);

          currBinding++;
      }
    }

    if (!buildDescriptorSetLayout(descLayoutParam, descLayout, pipeLayout)) {
      printf("Could not create descriptor set layout!\n");
      return false;
    }

    node._descriptorSetLayout = descLayout;
    node._pipelineLayout = pipeLayout;

    // Descriptor sets
    std::vector<VkSampler> samplers;
    DescriptorSetsCreateParams descParam{};
    descParam.renderContext = renderContext;
    descParam.descLayout = descLayout;

    // TODO: This shouldn't be needed...
    bool multiBuffered = false;
    currBinding = 0;
    for (int i = 0; i < node._resourceUsages.size(); ++i) {
      auto& usage = node._resourceUsages[i];
      bool advanceBinding = false;

      DescriptorBindInfo bindInfo{};
      bindInfo.binding = usage._bindless ? highestBinding : currBinding;
      bindInfo.stages = findStages(usage._resourceName);
      bindInfo.type = translateDescriptorType(usage._type);
      bindInfo.bindless = usage._bindless;

      multiBuffered = usage._multiBuffered;
      std::size_t num = usage._multiBuffered ? renderContext->getMultiBufferSize() : 1;

      if (isTypeBuffer(usage._type) && shouldBeDescriptor(usage._stage)) {
        for (int i = 0; i < num; ++i) {
          auto buf = (BufferRenderResource*)vault->getResource(usage._resourceName, i);
          bindInfo.buffer = buf->_buffer._buffer;
          descParam.bindInfos.emplace_back(bindInfo);

          advanceBinding = true;
        }
      }
      else if (isTypeImage(usage._type) && shouldBeDescriptor(usage._type)) {
        SamplerCreateParams samplerParam{};
        samplerParam.useMaxFilter = usage._useMaxSampler;
        samplerParam.renderContext = renderContext;
        if (usage._samplerClamp) {
          samplerParam.addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        }
        auto sampler = createSampler(samplerParam);

        for (int i = 0; i < num; ++i) {
          VkImageView view;

          if (usage._allMips) {
            view = ((ImageRenderResource*)vault->getResource(usage._resourceName, i))->_views[0];
          }
          else {
            view = ((ImageRenderResource*)vault->getResource(usage._resourceName, i))->_views[usage._mip + 1];
          }

          bindInfo.view = view;
          bindInfo.imageLayout = usage._imageAlwaysGeneral ? VK_IMAGE_LAYOUT_GENERAL : translateImageLayout(usage._type);
          bindInfo.sampler = sampler;
          descParam.bindInfos.emplace_back(bindInfo);

          advanceBinding = true;
        }

        std::string samplerName = usage._resourceName + "_sampler";
        renderContext->setDebugName(VK_OBJECT_TYPE_SAMPLER, (uint64_t)sampler, samplerName.c_str());
        samplers.emplace_back(sampler);
      }

      if (advanceBinding) currBinding++;
    }

    node._samplers = std::move(samplers);
    descParam.multiBuffered = multiBuffered;
    if (!descParam.bindInfos.empty()) {
      node._descriptorSets = buildDescriptorSets(descParam);
    }

    // Compute, graphics or raytracing pipeline
    if (node._computeParams.has_value()) {
      if (!buildComputePipeline(node._computeParams.value(), node._pipelineLayout, node._pipeline)) {
        printf("Could not build compute pipeline!\n");
        return false;
      }
    }
    else if (node._graphicsParams.has_value()) {
      if (!buildGraphicsPipeline(node._graphicsParams.value(), node._pipelineLayout, node._pipeline)) {
        printf("Could not build graphics pipeline!\n");
        return false;
      }
    }
    else if (renderContext->getRenderOptions().raytracingEnabled && node._rtParams.has_value()) {
      if (!buildRayTracingPipeline(node._rtParams.value(), node._pipelineLayout, node._pipeline, node._sbt)) {
        printf("Could not build ray tracing pipeline!\n");
        return false;
      }
    }
  }

  return true;
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
      GraphNode* nodeOut;
      //if (resourceUsage._invalidateAfterRead) {
        if (stackContainsProducer(localStack, resourceUsage._resourceName, &nodeOut)) {
          continue;
        }
      /* }
      else {
        // Remove ourselves from stack
        std::vector<GraphNode> tempStack = stack;
        tempStack.pop_back();
        tempStack.insert(tempStack.end(), localStack.begin(), localStack.end());
        if (stackContainsProducer(tempStack, resourceUsage._resourceName, &nodeOut)) {
          // Make sure that we are executed *after* the producer that is already present in the stack.
          ensureOrder(stack, submission->_regInfo._name, nodeOut->_debugName);
          continue;
        }
      }*/

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
      node._computeParams = producer->_regInfo._computeParams;
      node._graphicsParams= producer->_regInfo._graphicsParams;
      node._rtParams = producer->_regInfo._rtParams;
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

void FrameGraphBuilder::internalBuild2(Submission* presentSub)
{
  // Build an internal resource information structure first
  std::vector<ResourceGraphInfo> resInfos;
  for (auto& sub : _submissions) {
    for (auto& resUsage : sub._regInfo._resourceUsages) {

      auto it = std::find_if(resInfos.begin(), resInfos.end(), [&](ResourceGraphInfo& info) {
        return info._resource == resUsage._resourceName;
        });

      bool r = resUsage._access.test((std::size_t)Access::Read);
      bool w = resUsage._access.test((std::size_t)Access::Write);

      if (it != resInfos.end()) {
        if (r) {
          it->_rSubs.emplace_back(&sub);
        }
        if (w) {
          it->_wSubs.emplace_back(&sub);
        }
      }
      else {
        ResourceGraphInfo info{};
        info._resource = resUsage._resourceName;
        if (r) info._rSubs.emplace_back(&sub);
        if (w) info._wSubs.emplace_back(&sub);
        resInfos.emplace_back(std::move(info));
      }
    }
  }

  // DEBUG
  for (auto& inf : resInfos) {
    printf("\n");
    printf("Resource %s is written to by: \n", inf._resource.c_str());
    for (auto sub : inf._wSubs) {
      printf("\t%s", sub->_regInfo._name.c_str());
    }
    printf("\n");

    printf("Resource %s is read from by: \n", inf._resource.c_str());
    for (auto sub : inf._rSubs) {
      printf("\t%s", sub->_regInfo._name.c_str());
    }
    printf("\n");
  }

  // Go through each resource (start with the one used for presenting) and add all passes that also _read_ from this resource.
  std::string presentRes;
  for (auto& resUsage : presentSub->_regInfo._resourceUsages) {
    if (resUsage._type == Type::Present) {
      presentRes = resUsage._resourceName;
      break;
    }
  }

  GraphNode node{};
  node._rpExe = presentSub->_exe;
  node._debugName = std::string(presentSub->_regInfo._name);
  node._resourceUsages = presentSub->_regInfo._resourceUsages;
  _builtGraph.emplace_back(node);
   
  /*readSubs = addReadSubs();
  writeSubs = addWriteSubs
  res = */

  std::string currRes = presentRes;
  for (auto& info : resInfos) {
    if (info._resource == currRes) {

      for (auto rSub : info._rSubs) {
        GraphNode node{};
        for (auto& producerUsage : rSub->_regInfo._resourceUsages) {
          if (producerUsage._access.test((std::size_t)Access::Write)) {
            node._producedResources.emplace_back(producerUsage._resourceName);
          }
        }
        node._rpExe = rSub->_exe;
        node._debugName = std::string(rSub->_regInfo._name);
        node._resourceUsages = rSub->_regInfo._resourceUsages;
        _builtGraph.emplace_back(std::move(node));
      }

      // Now

    } 
  }
}

void FrameGraphBuilder::internalBuild3()
{
  // Simply use submission order, but insert resource inits if there are any
  for (auto& sub : _submissions) {
    GraphNode node{};

    for (auto& resUs : sub._regInfo._resourceUsages) {

      if (resUs._access.test((std::size_t)Access::Write)) {
        node._producedResources.emplace_back(resUs._resourceName);

        if (auto init = findResourceInit(resUs._resourceName)) {
          GraphNode node{};
          node._resourceInit = init;
          node._resourceUsages = { init->_initUsage };
          node._debugName = std::string(init->_resource);
          _builtGraph.emplace_back(node);
        }
      }
    }

    node._rpExe = sub._exe;
    node._computeParams = sub._regInfo._computeParams;
    node._graphicsParams = sub._regInfo._graphicsParams;
    node._rtParams = sub._regInfo._rtParams;
    node._debugName = std::string(sub._regInfo._name);
    node._resourceUsages = sub._regInfo._resourceUsages;

    _builtGraph.emplace_back(node);
  }
}

std::pair<VkImageLayout, VkImageLayout> FrameGraphBuilder::findImageLayoutUsage(AccessBits prevAccess, Type prevType, AccessBits newAccess, Type newType)
{
  if (prevAccess.test(std::size_t(Access::Write)) &&
      newAccess.test(std::size_t(Access::Read))) {

    if (prevType == Type::DepthAttachment && newType == Type::SampledTexture) {
      return { VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL };
    }
    else if (prevType == Type::DepthAttachment && newType == Type::SampledDepthTexture) {
      return { VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL };
    }
    else if (prevType == Type::ColorAttachment && newType == Type::SampledTexture) {
      return { VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    }
    else if (prevType == Type::ColorAttachment && newType == Type::ColorAttachment) {
      return { VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    }
    else if (prevType == Type::ColorAttachment && newType == Type::Present) {
      //return { VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR };
      return { VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL };
    }
    else if (prevType == Type::ImageStorage && newType == Type::ColorAttachment) {
      return { VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    }
    else if (prevType == Type::ImageStorage && newType == Type::SampledTexture) {
      return { VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    }
    else if (prevType == Type::ImageStorage && newType == Type::Present) {
      return { VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL };
    }
  }
  else if (prevAccess.test(std::size_t(Access::Read)) &&
    newAccess.test(std::size_t(Access::Read))) {
    if (prevType == Type::SampledTexture && newType == Type::Present) {
      return { VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL };
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
    else if (type == Type::ImageStorage) {
      return VK_IMAGE_LAYOUT_GENERAL;
    }
    else if (type == Type::Present) {
      return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    }
    else if (type == Type::ImageTransferDst) {
      return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    }
  }
  else if (access.test(std::size_t(Access::Read))) {
    if (type == Type::SampledDepthTexture) {
      return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    }
    else if (type == Type::SampledTexture) {
      return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    else if (type == Type::ImageStorage) {
      return VK_IMAGE_LAYOUT_GENERAL;
    }
    else if (type == Type::Present) {
      return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    }
    else if (type == Type::ImageTransferSrc) {
      return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
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
  else if (old == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    from = "SHADER_READ_ONLY_OPTIMAL";
  }
  else if (old == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) {
    from = "DEPTH_ATTACHMENT_OPTIMAL";
  }
  else if (old == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    from = "DEPTH_STENCIL_ATTACHMENT_OPTIMAL";
  }
  else if (old == VK_IMAGE_LAYOUT_GENERAL) {
    from = "GENERAL";
  }
  else if (old == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    from = "TRANSFER_DST_OPTIMAL";
  }
  else if (old == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
    from = "TRANSFER_SRC_OPTIMAL";
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
  else if (newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
    to = "TRANSFER_SRC_OPTIMAL";
  }
  else if (newLayout == VK_IMAGE_LAYOUT_GENERAL) {
    to = "GENERAL";
  }
  else if (newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    to = "TRANSFER_DST_OPTIMAL";
  }

  output.append("Barrier: FROM ");
  output.append(from);
  output.append(" TO ");
  output.append(to);
  return output;
}

std::vector<ResourceUsage*> FrameGraphBuilder::findPrevMipUsages(const ResourceUsage& usage, std::vector<GraphNode>& stack, int stackIdx)
{
  // Find how many mips there are for starters
  int numMips = 0;

  for (auto& sub : _submissions) {
    bool done = false;
    for (auto& resUs : sub._regInfo._resourceUsages) {
      if (resUs._resourceName == usage._resourceName && resUs._imageCreateInfo.has_value()) {
        numMips = resUs._imageCreateInfo->_mipLevels;
        done = true;
        break;
      }
    }
    if (done) break;
  }

  if (numMips == 0) {
    printf("Fatal: Could not find number of mip levels for resource %s\n", usage._resourceName.c_str());
    return {};
  }

  // Now go through backwards starting from stackIdx-1 and try to find each mip usage
  std::vector<ResourceUsage*> out;

  for (int mip = 0; mip < numMips; ++mip) {
    for (int j = stackIdx - 1; j >= 0; --j) {
      auto& prevNode = stack[j];
      bool done = false;
      for (auto& prev : prevNode._resourceUsages) {
        if (prev._resourceName == usage._resourceName) {
          if (!prev._allMips && prev._mip == mip) {
            out.emplace_back(&prev);
            done = true;
            break;
          }
        }
      }
      // If we found something (or called break) in inner loop, break outer loop
      if (done) {
        break;
      }
    }
  }

  return out;
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
          return { VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_INDIRECT_COMMAND_READ_BIT };
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
        if (newStage.test((std::size_t)Stage::RayTrace)) {
          return { VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT };
        }
      }
    }
  }

  return std::pair<VkAccessFlagBits, VkAccessFlagBits>();
}

std::pair<VkPipelineStageFlagBits, VkPipelineStageFlagBits> FrameGraphBuilder::translateStageBits(Type prevType, Type newType, StageBits prevStage, StageBits newStage)
{
  VkPipelineStageFlagBits prev = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  VkPipelineStageFlagBits next = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

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
    if (prevType == Type::DepthAttachment) {
      prev = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    }
    else if (prevType == Type::ColorAttachment) {
      prev = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    }
    else {
      prev = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
  }
  else if (prevStage.test((std::size_t)Stage::RayTrace)) {
    prev = VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
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
    if (newType == Type::DepthAttachment) {
      next = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else if (newType == Type::ColorAttachment) {
      next = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    }
    else {
      next = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
  }
  else if (newStage.test((std::size_t)Stage::RayTrace)) {
    next = VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
  }
  
  if (newType == Type::Present) {
    next = VK_PIPELINE_STAGE_TRANSFER_BIT;
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
  if (oldStage & VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR) {
    srcStage.append(" | RAYTRACE");
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
  if (newStage & VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR) {
    dstStage.append(" | RAYTRACE");
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
  for (std::size_t i = 0; i < stack.size(); ++i) {
    auto& node = stack[i];

    updatedStack.emplace_back(node);

    // Keep track of where the current node is placed in the updatedStack
    std::size_t currentNodeIdx = updatedStack.size() - 1;

    for (auto& usage : node._resourceUsages) {

      // Find (maybe) a previous usage
      ResourceUsage* prevUsage = nullptr;
      if (i > 0) {
        for (int j = i-1; j >= 0; --j) {
          auto& prevNode = stack[j];
          for (auto& prev : prevNode._resourceUsages) {
            if (prev._resourceName == usage._resourceName) {
              if (usesSameMips(prev, usage)) {
                prevUsage = &prev;
                break;
              }
            }
          }
          // If we found something (or called break) in inner loop, break outer loop
          if (prevUsage) {
            break;
          }
        }
      }

      // Find next nodes equivalent of the resource
      ResourceUsage* nextUsage = nullptr;
      for (std::size_t j = i + 1; j < stack.size(); ++j) {
        auto& nextNode = stack[j];
        for (auto& next : nextNode._resourceUsages) {
          if (next._resourceName == usage._resourceName) {
            if (usesSameMips(next, usage)) {
              nextUsage = &next;
              break;
            }
          }
        }
        // If we found something (or called break) in inner loop, break outer loop
        if (nextUsage) {
          break;
        }
      }

      if (!nextUsage) {
        // Are we ourselves using it? Check that first
        for (auto& us : node._resourceUsages) {
          if (us._resourceName == usage._resourceName &&
            us._mip == usage._mip &&
            us._access != usage._access &&
            us._type != usage._type) {
            nextUsage = &us;
          }
        }
      }

      // Do either a execution barrier, buffer memory barrier or image transition
      if (isTypeImage(usage._type)) {
        bool skipPreBarrier = false;

        // If we only read and previous only read, skip barrier
        std::vector<ResourceUsage*> prevMipUsages;
        if (prevUsage) {
          if (usage._access.test((std::size_t)Access::Read) && prevUsage->_access.test((std::size_t)Access::Read) &&
              usage._stage == prevUsage->_stage &&
              usage._allMips == prevUsage->_allMips &&
              !(usage._imageAlwaysGeneral || prevUsage->_imageAlwaysGeneral)) {
            skipPreBarrier = true;
          }

          // If prev usage of an image is a single mip, and we now use all mips, we have to account for the rest of the prev mip usages to get the 
          // correct transitions
          if (!prevUsage->_allMips && usage._allMips) {
            prevMipUsages = findPrevMipUsages(usage, stack, i);
          }
        }

        std::uint32_t baseLayer = usage._imageBaseLayer;
        std::uint32_t mip = usage._mip;
        bool allMips = usage._allMips;

        // Initial layout transition (before executing)
        if (!skipPreBarrier) {
          auto initialLayout = usage._imageAlwaysGeneral ? VK_IMAGE_LAYOUT_GENERAL : findInitialImageLayout(usage._access, usage._type);
          VkImageLayout oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
          auto stageBits = translateStageBits(usage._type, usage._type, usage._stage, usage._stage);
          bool depth = isDepth(usage._type);
          auto dstAccessMask = findWriteAccessMask(usage._type);
          auto srcAccessMask = 0;

          if (prevUsage) {
            srcAccessMask = findWriteAccessMask(prevUsage->_type);
            stageBits = translateStageBits(prevUsage->_type, usage._type, prevUsage->_stage, usage._stage);
            oldLayout = prevUsage->_imageAlwaysGeneral ? VK_IMAGE_LAYOUT_GENERAL : findInitialImageLayout(prevUsage->_access, prevUsage->_type);
          }

          // Do a loop for all prev mip usages (this is most of the time going to be empty)
          if (!prevMipUsages.empty()) {
            for (auto prevMipUsage : prevMipUsages) {
              srcAccessMask = findWriteAccessMask(prevMipUsage->_type);
              stageBits = translateStageBits(prevMipUsage->_type, usage._type, prevMipUsage->_stage, usage._stage);
              oldLayout = prevMipUsage->_imageAlwaysGeneral ? VK_IMAGE_LAYOUT_GENERAL : findInitialImageLayout(prevMipUsage->_access, prevMipUsage->_type);

              BarrierContext beforeWriteBarrier{};
              beforeWriteBarrier._resourceName = usage._resourceName;
              auto prevMip = prevMipUsage->_mip;
              beforeWriteBarrier._barrierFcn = [srcAccessMask, oldLayout, prevMip, baseLayer, initialLayout, stageBits, depth, dstAccessMask](IRenderResource* resource, VkCommandBuffer& cmdBuffer) {
                auto imageResource = dynamic_cast<ImageRenderResource*>(resource);

                VkImageMemoryBarrier barrier{};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.oldLayout = oldLayout;
                barrier.newLayout = initialLayout;
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.image = imageResource->_image._image;
                barrier.subresourceRange.aspectMask = depth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
                barrier.subresourceRange.baseMipLevel = prevMip;
                barrier.subresourceRange.levelCount = 1;
                barrier.subresourceRange.baseArrayLayer = baseLayer;
                barrier.subresourceRange.layerCount = 1;
                barrier.srcAccessMask = srcAccessMask;
                barrier.dstAccessMask = dstAccessMask;

                vkCmdPipelineBarrier(
                  cmdBuffer,
                  stageBits.first, stageBits.second,
                  0,
                  0, nullptr,
                  0, nullptr,
                  1, &barrier
                );
              };

              // Add the pre-write barrier _before_ writing
              GraphNode beforeWriteNode{};
              auto beforeName = debugConstructImageBarrierName(oldLayout, initialLayout);
              beforeWriteNode._debugName = "Before write " + usage._resourceName + " (" + beforeName + "), mip " + std::to_string(prevMip) + " (allMips = 0)";
              beforeWriteNode._barrier = std::move(beforeWriteBarrier);

              updatedStack.insert(updatedStack.begin() + currentNodeIdx, std::move(beforeWriteNode));
              // The currentNodeIdx has now been updated (since we added this node _before_)
              currentNodeIdx++;
            }
          }
          else {
            BarrierContext beforeWriteBarrier{};
            beforeWriteBarrier._resourceName = usage._resourceName;
            beforeWriteBarrier._barrierFcn = [srcAccessMask, oldLayout, allMips, mip, baseLayer, initialLayout, stageBits, depth, dstAccessMask](IRenderResource* resource, VkCommandBuffer& cmdBuffer) {
              auto imageResource = dynamic_cast<ImageRenderResource*>(resource);

              VkImageMemoryBarrier barrier{};
              barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
              barrier.oldLayout = oldLayout;
              barrier.newLayout = initialLayout;
              barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
              barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
              barrier.image = imageResource->_image._image;
              barrier.subresourceRange.aspectMask = depth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
              barrier.subresourceRange.baseMipLevel = allMips ? 0 : mip;
              barrier.subresourceRange.levelCount = allMips ? (imageResource->_views.size() == 1 ? 1 : imageResource->_views.size() - 1) : 1;
              barrier.subresourceRange.baseArrayLayer = baseLayer;
              barrier.subresourceRange.layerCount = 1;
              barrier.srcAccessMask = srcAccessMask;
              barrier.dstAccessMask = dstAccessMask;

              vkCmdPipelineBarrier(
                cmdBuffer,
                stageBits.first, stageBits.second,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier
              );
            };

            // Add the pre-write barrier _before_ writing
            GraphNode beforeWriteNode{};
            auto beforeName = debugConstructImageBarrierName(oldLayout, initialLayout);
            beforeWriteNode._debugName = "Before write " + usage._resourceName + " (" + beforeName + "), mip " + std::to_string(mip) + " (allMips = " + std::to_string(allMips) + ")";
            beforeWriteNode._barrier = std::move(beforeWriteBarrier);

            updatedStack.insert(updatedStack.begin() + currentNodeIdx, std::move(beforeWriteNode));
            // The currentNodeIdx has now been updated (since we added this node _before_)
            currentNodeIdx++;
          }
        }
      }
      else if (isTypeBuffer(usage._type) && nextUsage) {
        auto accessFlagPair = findBufferAccessFlags(usage._access, usage._stage, nextUsage->_access, nextUsage->_stage);
        auto transStagePair = translateStageBits(usage._type, nextUsage->_type, usage._stage, nextUsage->_stage);

        BarrierContext bufBarrier{};
        bufBarrier._resourceName = usage._resourceName;
        bufBarrier._barrierFcn = [accessFlagPair, transStagePair](IRenderResource* resource, VkCommandBuffer& cmdBuffer) {
          BufferRenderResource* buf = dynamic_cast<BufferRenderResource*>(resource);

          // If there is no access, an execution barrier is enough
          if (accessFlagPair.first == 0 && accessFlagPair.second == 0) {
            vkCmdPipelineBarrier(
              cmdBuffer,
              transStagePair.first,
              transStagePair.second,
              0,
              0, nullptr,
              0, nullptr,
              0, nullptr);
          }
          else {
            VkBufferMemoryBarrier memBarr{};
            memBarr.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            memBarr.srcAccessMask = accessFlagPair.first;
            memBarr.dstAccessMask = accessFlagPair.second;
            memBarr.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            memBarr.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
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
          }
        };

        GraphNode bufBarrNode{};
        bufBarrNode._debugName = std::string("(") + std::string(usage._resourceName) + std::string(") ") + debugConstructBufferBarrierName(accessFlagPair.first, transStagePair.first, accessFlagPair.second, transStagePair.second);
        bufBarrNode._barrier = std::move(bufBarrier);

        // This needs to be added after the current node (because it protects the next operation)
        updatedStack.insert(updatedStack.begin() + currentNodeIdx + 1, std::move(bufBarrNode));
      }
    }
  }

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