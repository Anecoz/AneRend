#include "ComputePipeline.h"

#include "GpuBuffers.h"

#include "../util/Utils.h"

namespace render {
namespace compute {

Material createComputeMaterial(VkDevice& device, const std::string& shader)
{
  Material output;

  // Descriptor set layout
  std::vector<VkDescriptorSetLayoutBinding> bindings;
  VkDescriptorSetLayoutBinding drawCmdBinding{};
  drawCmdBinding.binding = 0;
  drawCmdBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  drawCmdBinding.descriptorCount = 1;
  drawCmdBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  bindings.emplace_back(std::move(drawCmdBinding));

  VkDescriptorSetLayoutBinding renderableBinding{};
  renderableBinding.binding = 1;
  renderableBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  renderableBinding.descriptorCount = 1;
  renderableBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  bindings.emplace_back(std::move(renderableBinding));

  VkDescriptorSetLayoutBinding translationBinding{};
  translationBinding.binding = 2;
  translationBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  translationBinding.descriptorCount = 1;
  translationBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  bindings.emplace_back(std::move(translationBinding));

  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = (uint32_t)bindings.size();
  layoutInfo.pBindings = bindings.data();

  if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &output._descriptorSetLayouts[Material::STANDARD_INDEX]) != VK_SUCCESS) {
    printf("Could not create compute descriptor set layout!\n");
    return output;
  }

  // Push constants
  VkPushConstantRange pushConstant;
  pushConstant.offset = 0;
  pushConstant.size = (uint32_t)sizeof(gpu::GPUCullPushConstants);
  pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  // Pipeline layout
  VkPipelineLayoutCreateInfo pipelineLayoutInfo {};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &output._descriptorSetLayouts[Material::STANDARD_INDEX];
  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges = &pushConstant;
  if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &output._pipelineLayouts[Material::STANDARD_INDEX]) != VK_SUCCESS) {
    printf("Could not create compute pipeline layout!\n");
    return output;
  }

  // Shaders
  VkShaderModule compShaderModule;
  auto compShaderCode = util::readFile(std::string(ASSET_PATH) + shader);

  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = compShaderCode.size();
  createInfo.pCode = reinterpret_cast<const uint32_t*>(compShaderCode.data());

  if (vkCreateShaderModule(device, &createInfo, nullptr, &compShaderModule) != VK_SUCCESS) {
    printf("Could not create shader module!\n");
    return output;
  }

  DEFER([&](){
    vkDestroyShaderModule(device, compShaderModule, nullptr);
  });

  VkPipelineShaderStageCreateInfo compShaderStageInfo{};
  compShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  compShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  compShaderStageInfo.module = compShaderModule;
  compShaderStageInfo.pName = "main";

  // Pipeline
  VkComputePipelineCreateInfo pipelineInfo {};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  pipelineInfo.stage = compShaderStageInfo;
  pipelineInfo.layout = output._pipelineLayouts[Material::STANDARD_INDEX];

  if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &output._pipelines[Material::STANDARD_INDEX]) != VK_SUCCESS) {
    printf("Could not create compute pipeline!\n");
    return output;
  }

  return output;
}

}}