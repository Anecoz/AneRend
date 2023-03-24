#include "RenderPass.h"

#include "Vertex.h"
#include "../util/Utils.h"

#include <optional>
#include <vector>

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

}

namespace render {

bool RenderPass::buildGraphicsPipeline(GraphicsPipelineCreateParams param)
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
  vertexInputInfo.vertexBindingDescriptionCount = (uint32_t)bindingDescriptions.size();
  vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
  vertexInputInfo.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(attributeDescriptions.size());
  vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

  // Input assembly
  VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
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
  rasterizer.depthBiasEnable = VK_FALSE;
  if (param.depthBiasEnable) {
    rasterizer.depthBiasEnable = VK_TRUE;
    rasterizer.depthBiasConstantFactor = param.depthBiasConstant;
    rasterizer.depthBiasSlopeFactor = param.depthBiasSlope;
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
  VkPipelineColorBlendAttachmentState colorBlendAttachment{};
  if (param.colorAttachment) {
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;
  }
  else {
    colorBlending.attachmentCount = 0;
  }

  // Dynamic rendering info
  VkPipelineRenderingCreateInfoKHR renderingCreateInfo{};
  renderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
  renderingCreateInfo.colorAttachmentCount = param.colorAttachment ? 1 : 0;
  renderingCreateInfo.pColorAttachmentFormats = param.colorAttachment ? &param.colorFormat : nullptr;
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
  pipelineInfo.layout = param.pipelineLayout;
  pipelineInfo.renderPass = nullptr;
  pipelineInfo.subpass = 0;
  pipelineInfo.pDepthStencilState = &depthStencil;

  if (vkCreateGraphicsPipelines(param.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pipeline) != VK_SUCCESS) {
    printf("failed to create graphics pipeline!\n");
    return false;
  }

  return true;
}

bool RenderPass::buildComputePipeline(ComputePipelineCreateParams params)
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
  pipelineInfo.layout = params.pipelineLayout;

  if (vkCreateComputePipelines(params.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pipeline) != VK_SUCCESS) {
    printf("Could not create compute pipeline!\n");
    return false;
  }

  return true;
}

bool RenderPass::buildDescriptorSetLayout(DescriptorSetLayoutCreateParams params)
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

  if (vkCreateDescriptorSetLayout(params.renderContext->device(), &layoutInfo, nullptr, &_descriptorSetLayout) != VK_SUCCESS) {
    printf("Could not create descriptor set layout!\n");
    return false;
  }

  // Pipeline layout
  PipelineLayoutCreateParams pipeLayoutParam{};
  pipeLayoutParam.device = params.renderContext->device();
  pipeLayoutParam.descriptorSetLayouts.emplace_back(params.renderContext->bindlessDescriptorSetLayout()); // set 0
  pipeLayoutParam.descriptorSetLayouts.emplace_back(_descriptorSetLayout); // set 1
  pipeLayoutParam.pushConstantsSize = 256;
  pipeLayoutParam.pushConstantStages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

  if (!buildPipelineLayout(pipeLayoutParam)) {
    return false;
  }

  return true;
}

bool RenderPass::buildPipelineLayout(PipelineLayoutCreateParams params)
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

  if (vkCreatePipelineLayout(params.device, &pipelineLayoutInfo, nullptr, &_pipelineLayout) != VK_SUCCESS) {
    printf("Could not create pipeline layout!\n");
    return false;
  }

  return true;
}

std::vector<VkDescriptorSet> RenderPass::buildDescriptorSets(DescriptorSetsCreateParams params)
{
  int numMultiBuffer = params.renderContext->getMultiBufferSize();

  std::vector<VkDescriptorSet> sets;
  sets.resize(numMultiBuffer);

  std::vector<VkDescriptorSetLayout> layouts(numMultiBuffer, _descriptorSetLayout);
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
    if (j - currIdx*numDescriptors >= numDescriptors) {
      currIdx++;
    }
    std::vector<VkWriteDescriptorSet> descriptorWrites;

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

    vkUpdateDescriptorSets(params.renderContext->device(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
  }

  return sets;
}

}
