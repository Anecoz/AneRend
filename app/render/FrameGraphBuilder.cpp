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

std::optional<VkShaderModule> createShaderModule(const std::vector<char>& code, VkDevice device)
{
  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

  VkShaderModule shaderModule;
  if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
    printf("Could not create shader module!\n");
    return {};
  }

  return shaderModule;
}

bool isTypeBuffer(Type type)
{
  return type == Type::SSBO || type == Type::UBO;
}

bool isTypeImage(Type type)
{
  return type == Type::ColorAttachment || type == Type::DepthAttachment || type == Type::SampledDepthTexture ||
    type == Type::Present || type == Type::SampledTexture || type == Type::ImageStorage;
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
    type == Type::Present) {
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

      // Views and buffers
      int multiIdx = renderContext->getCurrentMultiBufferIdx();
      for (auto& us : node._resourceUsages) {
        if (us._type == Type::ColorAttachment) {
          auto view = ((ImageRenderResource*)_vault->getResource(us._resourceName))->_view;
          exeParams.colorAttachmentViews.emplace_back(view);
        }
        else if (us._type == Type::DepthAttachment) {
          auto view = ((ImageRenderResource*)_vault->getResource(us._resourceName))->_view;
          exeParams.depthAttachmentViews.emplace_back(view);
        }
        else if (us._type == Type::SSBO) {
          auto buf = ((BufferRenderResource*)_vault->getResource(us._resourceName, multiIdx))->_buffer._buffer;
          exeParams.buffers.emplace_back(buf);
        }
        else if (us._type == Type::Present) {
          exeParams.presentImage = ((ImageRenderResource*)_vault->getResource(us._resourceName))->_image._image;
        }
      }
      node._rpExe.value()(exeParams);
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
                im->_image);

              im->_view = imageutil::createImageView(
                renderContext->device(), 
                im->_image._image,
                im->_format,
                usage._type == Type::DepthAttachment ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT);

              std::string name = usage._resourceName + "_" + std::to_string(i);
              std::string viewName = usage._resourceName + "View" + std::to_string(i);
              renderContext->setDebugName(VK_OBJECT_TYPE_IMAGE, (uint64_t)im->_image._image, name.c_str());
              renderContext->setDebugName(VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)im->_view, viewName.c_str());

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

  if (createdResources.size() != foundResources.size()) {
    return false;
  }

  for (std::size_t i = 0; i < createdResources.size(); ++i) {
    if (createdResources[i] != foundResources[i]) return false;
  }

  return true;
}

bool FrameGraphBuilder::buildDescriptorSetLayout(DescriptorSetLayoutCreateParams params, VkDescriptorSetLayout& outDescriptorSetLayout, VkPipelineLayout& outPipelineLayout)
{
  std::vector<VkDescriptorSetLayoutBinding> bindings;

  for (auto& bindInfo : params.bindInfos) {
    VkDescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding = bindInfo.binding;
    layoutBinding.descriptorType = bindInfo.type;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = bindInfo.stages;
    bindings.emplace_back(std::move(layoutBinding));
  }

  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = (uint32_t)bindings.size();
  layoutInfo.pBindings = bindings.data();

  if (vkCreateDescriptorSetLayout(params.renderContext->device(), &layoutInfo, nullptr, &outDescriptorSetLayout) != VK_SUCCESS) {
    printf("Could not create descriptor set layout!\n");
    return false;
  }

  // Pipeline layout
  PipelineLayoutCreateParams pipeLayoutParam{};
  pipeLayoutParam.device = params.renderContext->device();
  pipeLayoutParam.descriptorSetLayouts.emplace_back(params.renderContext->bindlessDescriptorSetLayout()); // set 0
  pipeLayoutParam.descriptorSetLayouts.emplace_back(outDescriptorSetLayout); // set 1
  pipeLayoutParam.pushConstantsSize = 256;
  pipeLayoutParam.pushConstantStages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

  if (!buildPipelineLayout(pipeLayoutParam, outPipelineLayout)) {
    return false;
  }

  return true;
}

bool FrameGraphBuilder::buildPipelineLayout(PipelineLayoutCreateParams params, VkPipelineLayout& outPipelineLayout)
{
  // Push constants
  VkPushConstantRange pushConstant;
  if (params.pushConstantsSize > 0) {
    pushConstant.offset = 0;
    pushConstant.size = params.pushConstantsSize;
    pushConstant.stageFlags = params.pushConstantStages;
  }

  // Pipeline layout
  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = (uint32_t)params.descriptorSetLayouts.size();
  pipelineLayoutInfo.pSetLayouts = &params.descriptorSetLayouts[0];
  pipelineLayoutInfo.pushConstantRangeCount = params.pushConstantsSize > 0 ? 1 : 0;
  pipelineLayoutInfo.pPushConstantRanges = params.pushConstantsSize > 0 ? &pushConstant : nullptr;

  if (vkCreatePipelineLayout(params.device, &pipelineLayoutInfo, nullptr, &outPipelineLayout) != VK_SUCCESS) {
    printf("Could not create pipeline layout!\n");
    return false;
  }

  return true;
}

std::vector<VkDescriptorSet> FrameGraphBuilder::buildDescriptorSets(DescriptorSetsCreateParams params)
{
  int numMultiBuffer = params.multiBuffered ? params.renderContext->getMultiBufferSize() : 1;

  std::vector<VkDescriptorSet> sets;
  sets.resize(numMultiBuffer);

  std::vector<VkDescriptorSetLayout> layouts(numMultiBuffer, params.descLayout);
  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = params.renderContext->descriptorPool();
  allocInfo.descriptorSetCount = static_cast<uint32_t>(numMultiBuffer);
  allocInfo.pSetLayouts = layouts.data();

  if (vkAllocateDescriptorSets(params.renderContext->device(), &allocInfo, sets.data()) != VK_SUCCESS) {
    printf("failed to allocate compute descriptor sets!\n");
    return {};
  }

  int numDescriptors = params.bindInfos.size() / numMultiBuffer;
  int currIdx = 0;
  for (size_t j = 0; j < params.bindInfos.size(); j++) {
    if (currIdx >= numMultiBuffer) {
      currIdx = 0;
    }
    /*if (j - currIdx * numDescriptors >= numDescriptors) {
      currIdx++;
    }*/
    std::vector<VkWriteDescriptorSet> descriptorWrites;

    if (params.bindInfos[j].type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) {
      VkDescriptorBufferInfo bufferInfo{};
      VkWriteDescriptorSet bufWrite{};
      bufferInfo.buffer = params.bindInfos[j].buffer;
      bufferInfo.offset = 0;
      bufferInfo.range = VK_WHOLE_SIZE;

      bufWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      bufWrite.dstSet = sets[currIdx];
      bufWrite.dstBinding = params.bindInfos[j].binding;
      bufWrite.dstArrayElement = 0;
      bufWrite.descriptorType = params.bindInfos[j].type;
      bufWrite.descriptorCount = 1;
      bufWrite.pBufferInfo = &bufferInfo;

      descriptorWrites.emplace_back(std::move(bufWrite));
    }
    else if (params.bindInfos[j].type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
      VkDescriptorBufferInfo bufferInfo{};
      VkWriteDescriptorSet bufWrite{};
      bufferInfo.buffer = params.bindInfos[j].buffer;
      bufferInfo.offset = 0;
      bufferInfo.range = VK_WHOLE_SIZE;

      bufWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      bufWrite.dstSet = sets[currIdx];
      bufWrite.dstBinding = params.bindInfos[j].binding;
      bufWrite.dstArrayElement = 0;
      bufWrite.descriptorType = params.bindInfos[j].type;
      bufWrite.descriptorCount = 1;
      bufWrite.pBufferInfo = &bufferInfo;

      descriptorWrites.emplace_back(std::move(bufWrite));
    }
    else if (params.bindInfos[j].type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
      VkWriteDescriptorSet imWrite{};
      VkDescriptorImageInfo imageInfo{};
      imageInfo.imageLayout = params.bindInfos[j].imageLayout;
      imageInfo.imageView = params.bindInfos[j].view;
      imageInfo.sampler = params.bindInfos[j].sampler;

      imWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      imWrite.dstSet = sets[currIdx];
      imWrite.dstBinding = params.bindInfos[j].binding;
      imWrite.dstArrayElement = 0;
      imWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      imWrite.descriptorCount = 1;
      imWrite.pImageInfo = &imageInfo;

      descriptorWrites.emplace_back(std::move(imWrite));
    }
    else if (params.bindInfos[j].type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE) {
      VkWriteDescriptorSet imWrite{};
      VkDescriptorImageInfo imageInfo{};
      imageInfo.imageView = params.bindInfos[j].view;
      imageInfo.imageLayout = params.bindInfos[j].imageLayout;

      imWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      imWrite.dstSet = sets[currIdx];
      imWrite.dstBinding = params.bindInfos[j].binding;
      imWrite.dstArrayElement = 0;
      imWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
      imWrite.descriptorCount = 1;
      imWrite.pImageInfo = &imageInfo;

      descriptorWrites.emplace_back(std::move(imWrite));
    }

    vkUpdateDescriptorSets(params.renderContext->device(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    currIdx++;
  }

  return sets;
}

VkSampler FrameGraphBuilder::createSampler(SamplerCreateParams params)
{
  VkSampler samplerOut;

  VkSamplerCreateInfo sampler{};
  sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  sampler.magFilter = params.magFilter;
  sampler.minFilter = params.minFilter;
  sampler.mipmapMode = params.mipMapMode;
  sampler.addressModeU = params.addressMode;
  sampler.addressModeV = sampler.addressModeU;
  sampler.addressModeW = sampler.addressModeU;
  sampler.mipLodBias = params.mipLodBias;
  sampler.maxAnisotropy = params.maxAnisotropy;
  sampler.minLod = params.minLod;
  sampler.maxLod = params.maxLod;
  sampler.borderColor = params.borderColor;

  if (vkCreateSampler(params.renderContext->device(), &sampler, nullptr, &samplerOut) != VK_SUCCESS) {
    printf("Could not create shadow map sampler!\n");
  }

  return samplerOut;
}

bool FrameGraphBuilder::buildGraphicsPipeline(GraphicsPipelineCreateParams param, VkPipelineLayout& pipelineLayout, VkPipeline& outPipeline)
{
  // Shaders
  VkShaderModule vertShaderModule;
  VkShaderModule fragShaderModule;
  if (!param.vertShader.empty()) {
    auto vertShaderCode = util::readFile(std::string(ASSET_PATH) + param.vertShader);
    auto opt = createShaderModule(vertShaderCode, param.device);
    if (!opt) {
      printf("Could not create vertex shader!\n");
      return false;
    }
    vertShaderModule = opt.value();
  }

  if (!param.fragShader.empty()) {
    auto fragShaderCode = util::readFile(std::string(ASSET_PATH) + param.fragShader);
    auto opt = createShaderModule(fragShaderCode, param.device);
    if (!opt) {
      printf("Could not create frag shader!\n");
      return false;
    }
    fragShaderModule = opt.value();
  }

  DEFER([&]() {
    if (!param.fragShader.empty()) {
      vkDestroyShaderModule(param.device, fragShaderModule, nullptr);
    }
  if (!param.vertShader.empty()) {
    vkDestroyShaderModule(param.device, vertShaderModule, nullptr);
  }
    });

  std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
  if (!param.vertShader.empty()) {
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";
    shaderStages.emplace_back(std::move(vertShaderStageInfo));
  }

  if (!param.fragShader.empty()) {
    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";
    shaderStages.emplace_back(std::move(fragShaderStageInfo));
  }

  // Dynamic setup, viewport for instance
  std::vector<VkDynamicState> dynamicStates = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR
  };

  VkPipelineDynamicStateCreateInfo dynamicState{};
  dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
  dynamicState.pDynamicStates = dynamicStates.data();

  // Vertex input
  std::vector<VkVertexInputBindingDescription> bindingDescriptions;
  VkVertexInputBindingDescription bindingDescription{};
  bindingDescription.binding = 0;
  bindingDescription.stride = sizeof(render::Vertex);
  bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  bindingDescriptions.emplace_back(std::move(bindingDescription));

  std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
  if (param.posLoc != -1) {
    VkVertexInputAttributeDescription desc{};
    desc.binding = 0;
    desc.location = param.posLoc;
    desc.format = VK_FORMAT_R32G32B32_SFLOAT;
    desc.offset = offsetof(render::Vertex, pos);
    attributeDescriptions.emplace_back(std::move(desc));
  }

  if (param.colorLoc != -1) {
    VkVertexInputAttributeDescription desc{};
    desc.binding = 0;
    desc.location = param.colorLoc;
    desc.format = VK_FORMAT_R32G32B32_SFLOAT;
    desc.offset = offsetof(render::Vertex, color);
    attributeDescriptions.emplace_back(std::move(desc));
  }

  if (param.normalLoc != -1) {
    VkVertexInputAttributeDescription desc{};
    desc.binding = 0;
    desc.location = param.normalLoc;
    desc.format = VK_FORMAT_R32G32B32_SFLOAT;
    desc.offset = offsetof(render::Vertex, normal);
    attributeDescriptions.emplace_back(std::move(desc));
  }

  if (param.uvLoc != -1) {
    VkVertexInputAttributeDescription desc{};
    desc.binding = 0;
    desc.location = param.uvLoc;
    desc.format = VK_FORMAT_R32G32_SFLOAT;
    desc.offset = offsetof(render::Vertex, uv);
    attributeDescriptions.emplace_back(std::move(desc));
  }

  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  if (param.vertexLess) {
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr;
  }
  else {
    vertexInputInfo.vertexBindingDescriptionCount = (uint32_t)bindingDescriptions.size();
    vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
  }

  // Input assembly
  VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = param.topology;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  // Viewport and scissors
  // Only specify count since they are dynamic
  // Will be setup later at drawing time
  VkPipelineViewportStateCreateInfo viewportState{};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.scissorCount = 1;

  // Rasterizer
  VkPipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE; // This to true means geometry never passes through rasterizer stage (?)
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL; // Anything else here requires enabling a GPU feature (wireframe etc)
  rasterizer.lineWidth = 1.0f; // Thickness of lines in terms of fragments
  rasterizer.cullMode = param.cullMode;
  rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizer.depthBiasEnable = param.depthBiasEnable ? VK_TRUE : VK_FALSE;
  if (param.depthBiasEnable) {
    rasterizer.depthBiasConstantFactor = param.depthBiasConstant;
    rasterizer.depthBiasSlopeFactor = param.depthBiasSlope;
    //rasterizer.depthBiasClamp = param.depthBiasConstant * 2.0f;
  }

  // Multisampling
  // Needs enabling of a GPU feature to actually use MS
  VkPipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisampling.minSampleShading = 1.0f; // Optional
  multisampling.pSampleMask = nullptr; // Optional
  multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
  multisampling.alphaToOneEnable = VK_FALSE; // Optional

  // Depth and stencil testing
  VkPipelineDepthStencilStateCreateInfo depthStencil{};
  depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencil.depthTestEnable = param.depthTest ? VK_TRUE : VK_FALSE;
  depthStencil.depthWriteEnable = param.depthTest ? VK_TRUE : VK_FALSE;
  depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
  depthStencil.depthBoundsTestEnable = VK_FALSE;
  depthStencil.minDepthBounds = 0.0f; // Optional
  depthStencil.maxDepthBounds = 1.0f; // Optional
  depthStencil.stencilTestEnable = VK_FALSE;
  depthStencil.front = {}; // Optional
  depthStencil.back = {}; // Optional

  // Color blending
  VkPipelineColorBlendStateCreateInfo colorBlending{};
  std::vector<VkPipelineColorBlendAttachmentState> colBlendAttachments;
  if (param.colorAttachment) {

    for (int i = 0; i < param.colorAttachmentCount; ++i) {
      VkPipelineColorBlendAttachmentState colorBlendAttachment{};
      colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
      colorBlendAttachment.blendEnable = VK_FALSE;
      colBlendAttachments.emplace_back(colorBlendAttachment);
    }

    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = param.colorAttachmentCount;
    colorBlending.pAttachments = colBlendAttachments.data();
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;
  }
  else {
    colorBlending.attachmentCount = 0;
  }

  // Dynamic rendering info
  //std::vector<VkFormat> formats(param.colorAttachmentCount, param.colorFormat);
  VkPipelineRenderingCreateInfoKHR renderingCreateInfo{};
  renderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
  renderingCreateInfo.colorAttachmentCount = param.colorAttachment ? param.colorAttachmentCount : 0;
  renderingCreateInfo.pColorAttachmentFormats = param.colorAttachment ? param.colorFormats.data() : nullptr;
  renderingCreateInfo.depthAttachmentFormat = param.depthFormat;

  // Creating the pipeline
  VkGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.pNext = &renderingCreateInfo;
  pipelineInfo.stageCount = (uint32_t)shaderStages.size();
  pipelineInfo.pStages = shaderStages.data();
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pDepthStencilState = nullptr; // Optional
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = &dynamicState;
  pipelineInfo.layout = pipelineLayout;
  pipelineInfo.renderPass = nullptr;
  pipelineInfo.subpass = 0;
  pipelineInfo.pDepthStencilState = &depthStencil;

  if (vkCreateGraphicsPipelines(param.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &outPipeline) != VK_SUCCESS) {
    printf("failed to create graphics pipeline!\n");
    return false;
  }

  return true;
}

bool FrameGraphBuilder::buildComputePipeline(ComputePipelineCreateParams params, VkPipelineLayout& pipelineLayout, VkPipeline& outPipeline)
{
  // Shaders
  VkShaderModule compShaderModule;
  auto compShaderCode = util::readFile(std::string(ASSET_PATH) + params.shader);

  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = compShaderCode.size();
  createInfo.pCode = reinterpret_cast<const uint32_t*>(compShaderCode.data());

  if (vkCreateShaderModule(params.device, &createInfo, nullptr, &compShaderModule) != VK_SUCCESS) {
    printf("Could not create shader module!\n");
    return false;
  }

  DEFER([&]() {
    vkDestroyShaderModule(params.device, compShaderModule, nullptr);
    });

  VkPipelineShaderStageCreateInfo compShaderStageInfo{};
  compShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  compShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  compShaderStageInfo.module = compShaderModule;
  compShaderStageInfo.pName = "main";

  // Pipeline
  VkComputePipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  pipelineInfo.stage = compShaderStageInfo;
  pipelineInfo.layout = pipelineLayout;

  if (vkCreateComputePipelines(params.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &outPipeline) != VK_SUCCESS) {
    printf("Could not create compute pipeline!\n");
    return false;
  }

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

    // Start with descriptor set layout and pipeline layout
    VkDescriptorSetLayout descLayout{};
    VkPipelineLayout pipeLayout{};

    DescriptorSetLayoutCreateParams descLayoutParam{};
    descLayoutParam.renderContext = renderContext;

    int currBinding = 0;
    for (int i = 0; i < node._resourceUsages.size(); ++i) {
      auto& usage = node._resourceUsages[i];

      if ((isTypeBuffer(usage._type) && shouldBeDescriptor(usage._stage)) ||
        (isTypeImage(usage._type) && shouldBeDescriptor(usage._type))) {
          DescriptorBindInfo bindInfo{};
          bindInfo.binding = currBinding;
          bindInfo.stages = findStages(usage._resourceName);
          bindInfo.type = translateDescriptorType(usage._type);
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
      bindInfo.binding = currBinding;
      bindInfo.stages = findStages(usage._resourceName);
      bindInfo.type = translateDescriptorType(usage._type);

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
        samplerParam.renderContext = renderContext;
        auto sampler = createSampler(samplerParam);

        for (int i = 0; i < num; ++i) {
          auto view = ((ImageRenderResource*)vault->getResource(usage._resourceName, i))->_view;

          bindInfo.view = view;
          bindInfo.imageLayout = translateImageLayout(usage._type);
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

    // Compute or graphics pipeline
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
          // If we now only read, also ok to skip since we should have done an _after write_ barrier already (previous iteration)
          if (usage._access.test((std::size_t)Access::Read)) {
            skipPreBarrier = true;
          }
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
          beforeWriteBarrier._barrierFcn = [baseLayer, initialLayout](IRenderResource* resource, VkCommandBuffer& cmdBuffer) {
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
          afterWriteBarrier._barrierFcn = [transitionPair, baseLayer](IRenderResource* resource, VkCommandBuffer& cmdBuffer) {
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