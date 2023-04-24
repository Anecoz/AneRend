#include "PipelineUtil.h"

#include "../util/Utils.h"
#include "Vertex.h"
#include "RenderContext.h"

#include <cstddef>
#include <optional>

namespace
{

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

}

namespace render
{

bool buildDescriptorSetLayout(DescriptorSetLayoutCreateParams params, VkDescriptorSetLayout& outDescriptorSetLayout, VkPipelineLayout& outPipelineLayout)
{
  std::vector<VkDescriptorSetLayoutBinding> bindings;

  bool containsBindless = false;

  for (auto& bindInfo : params.bindInfos) {

    if (bindInfo.bindless) {
      containsBindless = true;
    }

    VkDescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding = bindInfo.binding;
    layoutBinding.descriptorType = bindInfo.type;
    layoutBinding.descriptorCount = bindInfo.bindless? params.renderContext->getMaxBindlessResources() : 1;
    layoutBinding.stageFlags = bindInfo.bindless? VK_SHADER_STAGE_ALL : bindInfo.stages;
    bindings.emplace_back(std::move(layoutBinding));
  }

  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = (uint32_t)bindings.size();
  layoutInfo.pBindings = bindings.data();

  if (containsBindless) {
    VkDescriptorBindingFlags bindlessFlags = 
      VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT | 
      VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT | 
      VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT;

    layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;

    VkDescriptorSetLayoutBindingFlagsCreateInfo extendedInfo{};
    extendedInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    extendedInfo.bindingCount = 1;
    extendedInfo.pBindingFlags = &bindlessFlags;

    layoutInfo.pNext = &extendedInfo;
  }

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

bool buildPipelineLayout(PipelineLayoutCreateParams params, VkPipelineLayout& outPipelineLayout)
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

std::vector<VkDescriptorSet> buildDescriptorSets(DescriptorSetsCreateParams params)
{
  int numMultiBuffer = params.multiBuffered ? params.renderContext->getMultiBufferSize() : 1;

  bool containsBindless = false;
  for (auto& binding : params.bindInfos) {
    if (binding.bindless) {
      containsBindless = true;
      break;
    }
  }

  std::vector<VkDescriptorSet> sets;
  sets.resize(numMultiBuffer);

  std::vector<VkDescriptorSetLayout> layouts(numMultiBuffer, params.descLayout);
  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = params.renderContext->descriptorPool();
  allocInfo.descriptorSetCount = static_cast<uint32_t>(numMultiBuffer);
  allocInfo.pSetLayouts = layouts.data();

  VkDescriptorSetVariableDescriptorCountAllocateInfo countInfo{};
  uint32_t maxBinding = params.renderContext->getMaxBindlessResources() - 1;
  std::vector<uint32_t> counts(numMultiBuffer, maxBinding);

  if (containsBindless) {
    countInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
    countInfo.descriptorSetCount = static_cast<uint32_t>(numMultiBuffer);
    countInfo.pDescriptorCounts = counts.data();

    allocInfo.pNext = &countInfo;
  }

  if (vkAllocateDescriptorSets(params.renderContext->device(), &allocInfo, sets.data()) != VK_SUCCESS) {
    printf("failed to allocate compute descriptor sets!\n");
    return {};
  }

  int numDescriptors = params.bindInfos.size() / numMultiBuffer;
  int currIdx = 0;
  int currBindlessIdx = 0;
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
      imWrite.dstArrayElement = params.bindInfos[j].bindless ? currBindlessIdx++ : 0;
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

VkSampler createSampler(SamplerCreateParams params)
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

bool buildGraphicsPipeline(GraphicsPipelineCreateParams param, VkPipelineLayout& pipelineLayout, VkPipeline& outPipeline)
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

bool buildComputePipeline(ComputePipelineCreateParams params, VkPipelineLayout& pipelineLayout, VkPipeline& outPipeline)
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

}