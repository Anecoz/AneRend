#include "MaterialFactory.h"

#include "../util/Utils.h"

#include "Vertex.h" // For binding description offsets

#include <vulkan/vulkan.h>

#include <array>
#include <cstdio>
#include <string>
#include <optional>
#include <tuple>

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

bool internalCreate(
  VkDescriptorSetLayout& descriptorSetLayout, VkPipelineLayout& pipelineLayout, VkPipeline& pipeline,
  VkDevice device, VkFormat colorFormat, VkFormat depthFormat,
  bool ubo, bool sampler,
  const std::string& vertShader, const std::string& fragShader,
  bool instanceBuffer,
  int posLoc, int colorLoc, int normalLoc, int uvLoc,
  VkCullModeFlags cullMode,
  bool depthBiasEnable, float depthBiasConstant, float depthBiasSlope,
  bool depthTest,
  bool colorAttachment,
  bool pushConstants
  )
{
  // Descriptor set layout
  std::vector<VkDescriptorSetLayoutBinding> bindings;
  if (ubo) {
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings.emplace_back(std::move(uboLayoutBinding));
  }

  if (sampler) {
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings.emplace_back(std::move(samplerLayoutBinding));
  }

  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = (uint32_t)bindings.size();
  layoutInfo.pBindings = bindings.data();

  if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
    printf("failed to create descriptor set layout!\n");
    return false;
  }

  // Shaders
  VkShaderModule vertShaderModule;
  VkShaderModule fragShaderModule;
  if (!vertShader.empty()) {
    auto vertShaderCode = util::readFile(std::string(ASSET_PATH) + vertShader);
    auto opt = createShaderModule(vertShaderCode, device);
    if (!opt) {
      printf("Could not create vertex shader!\n");
      return false;
    }
    vertShaderModule = opt.value();
  }
  
  if (!fragShader.empty()) {
    auto fragShaderCode = util::readFile(std::string(ASSET_PATH) + fragShader);
    auto opt = createShaderModule(fragShaderCode, device);
    if (!opt) {
      printf("Could not create frag shader!\n");
      return false;
    }
    fragShaderModule = opt.value();
  }

  DEFER([&](){
    if (!fragShader.empty()) {
      vkDestroyShaderModule(device, fragShaderModule, nullptr);
    }
    if (!vertShader.empty()) {
      vkDestroyShaderModule(device, vertShaderModule, nullptr);
    }
  });

  std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
  if (!vertShader.empty()) {
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";
    shaderStages.emplace_back(std::move(vertShaderStageInfo));
  }

  if (!fragShader.empty()) {
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

  if (instanceBuffer) {
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 1;
    bindingDescription.stride = 4 * 4 * 4; // TODO: Configurable?
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
    bindingDescriptions.emplace_back(std::move(bindingDescription));
  }

  std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
  if (posLoc != -1) {
    VkVertexInputAttributeDescription desc{};
    desc.binding = 0;
    desc.location = posLoc;
    desc.format = VK_FORMAT_R32G32B32_SFLOAT;
    desc.offset = offsetof(render::Vertex, pos);
    attributeDescriptions.emplace_back(std::move(desc));
  }

  if (colorLoc != -1) {
    VkVertexInputAttributeDescription desc{};
    desc.binding = 0;
    desc.location = colorLoc;
    desc.format = VK_FORMAT_R32G32B32_SFLOAT;
    desc.offset = offsetof(render::Vertex, color);
    attributeDescriptions.emplace_back(std::move(desc));
  }

  if (normalLoc != -1) {
    VkVertexInputAttributeDescription desc{};
    desc.binding = 0;
    desc.location = normalLoc;
    desc.format = VK_FORMAT_R32G32B32_SFLOAT;
    desc.offset = offsetof(render::Vertex, normal);
    attributeDescriptions.emplace_back(std::move(desc));
  }

  if (uvLoc != -1) {
    VkVertexInputAttributeDescription desc{};
    desc.binding = 0;
    desc.location = uvLoc;
    desc.format = VK_FORMAT_R32G32_SFLOAT;
    desc.offset = offsetof(render::Vertex, uv);
    attributeDescriptions.emplace_back(std::move(desc));
  }

  if (instanceBuffer) {
    // TODO: Configurable?
    // Mat4 out of "4 vec4s"
    {
      VkVertexInputAttributeDescription desc{};
      desc.binding = 1;
      desc.location = 3;
      desc.format = VK_FORMAT_R32G32B32A32_SFLOAT;
      desc.offset = 0 * 4 * 4;
      attributeDescriptions.emplace_back(std::move(desc));
    }

    {
      VkVertexInputAttributeDescription desc{};
      desc.binding = 1;
      desc.location = 4;
      desc.format = VK_FORMAT_R32G32B32A32_SFLOAT;
      desc.offset = 1 * 4 * 4;
      attributeDescriptions.emplace_back(std::move(desc));
    }

    {
      VkVertexInputAttributeDescription desc{};
      desc.binding = 1;
      desc.location = 5;
      desc.format = VK_FORMAT_R32G32B32A32_SFLOAT;
      desc.offset = 2 * 4 * 4;
      attributeDescriptions.emplace_back(std::move(desc));
    }

    {
      VkVertexInputAttributeDescription desc{};
      desc.binding = 1;
      desc.location = 6;
      desc.format = VK_FORMAT_R32G32B32A32_SFLOAT;
      desc.offset = 3 * 4 * 4;
      attributeDescriptions.emplace_back(std::move(desc));
    }
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
  rasterizer.cullMode = cullMode;
  rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;
  if (depthBiasEnable) {
    rasterizer.depthBiasEnable = VK_TRUE;
    rasterizer.depthBiasConstantFactor = depthBiasConstant;
    rasterizer.depthBiasSlopeFactor = depthBiasSlope;
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
  depthStencil.depthTestEnable = depthTest ? VK_TRUE : VK_FALSE;
  depthStencil.depthWriteEnable = depthTest ? VK_TRUE : VK_FALSE;
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
  if (colorAttachment) {    
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

  // Push constants
  VkPushConstantRange pushConstant;
  if (pushConstants) {
    // TODO: Configurable
    pushConstant.offset = 0;
    pushConstant.size = 4 * 16;
    pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  }

  // Pipeline layout
  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
  pipelineLayoutInfo.pushConstantRangeCount = pushConstants ? 1 : 0;
  pipelineLayoutInfo.pPushConstantRanges = pushConstants ? &pushConstant : nullptr;

  if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
    printf("failed to create pipeline layout!\n");
    return false;
  }

  // Dynamic rendering info
  VkPipelineRenderingCreateInfoKHR renderingCreateInfo{};
  renderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
  renderingCreateInfo.colorAttachmentCount = colorAttachment ? 1 : 0;
  renderingCreateInfo.pColorAttachmentFormats = colorAttachment ? &colorFormat : nullptr;
  renderingCreateInfo.depthAttachmentFormat = depthFormat;

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
  pipelineInfo.pDepthStencilState = nullptr; // Optional
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = &dynamicState;
  pipelineInfo.layout = pipelineLayout;
  pipelineInfo.renderPass = nullptr;
  pipelineInfo.subpass = 0;
  pipelineInfo.pDepthStencilState = &depthStencil;

  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
    printf("failed to create graphics pipeline!\n");
    return false;
  }

  return true;
}

}

namespace render {

Material MaterialFactory::createStandardMaterial(VkDevice device, VkFormat colorFormat, VkFormat depthFormat)
{
  Material output;

  output._supportsPushConstants = true;
  output._supportsShadowPass = true;

  internalCreate(
    output._descriptorSetLayout, output._pipelineLayout, output._pipeline,
    device, colorFormat, depthFormat,
    true, true,
    "standard_vert.spv", "standard_frag.spv",
    false,
    0, 1, 2, -1,
    VK_CULL_MODE_BACK_BIT,
    false, 0.0f, 0.0f, 
    true,
    true,
    true);

  internalCreate(
    output._descriptorSetLayoutShadow, output._pipelineLayoutShadow, output._pipelineShadow,
    device, colorFormat, depthFormat,
    true, false,
    "standard_shadow_vert.spv", "",
    false, 
    0, -1, -1, -1,
    VK_CULL_MODE_FRONT_BIT,
    true, 4.0f, 1.5f,
    true,
    false,
    true
  );

  return output;
}

Material MaterialFactory::createShadowDebugMaterial(VkDevice device, VkFormat colorFormat, VkFormat depthFormat)
{
  Material output;

  internalCreate(
    output._descriptorSetLayout, output._pipelineLayout, output._pipeline,
    device, colorFormat, depthFormat,
    true, true,
    "shadow_debug_vert.spv", "shadow_debug_frag.spv",
    false,
    0, -1, -1, 1,
    VK_CULL_MODE_BACK_BIT,
    false, 0.0f, 0.0f,
    false,
    true,
    false
  );

  return output;
}

Material MaterialFactory::createStandardInstancedMaterial(VkDevice device, VkFormat colorFormat, VkFormat depthFormat)
{
  Material output;

  output._hasInstanceBuffer = true;
  output._supportsShadowPass = true;

  internalCreate(
    output._descriptorSetLayout, output._pipelineLayout, output._pipeline,
    device, colorFormat, depthFormat,
    true, true,
    "standard_instanced_vert.spv", "standard_frag.spv",
    true,
    0, 1, 2, -1,
    VK_CULL_MODE_BACK_BIT,
    false, 0.0f, 0.0f, 
    true,
    true,
    false
  );

  internalCreate(
    output._descriptorSetLayoutShadow, output._pipelineLayoutShadow, output._pipelineShadow,
    device, colorFormat, depthFormat,
    true, false,
    "standard_instanced_shadow_vert.spv", "",
    true,
    0, -1, -1, -1,
    VK_CULL_MODE_FRONT_BIT,
    true, 4.0f, 1.5f, 
    true,
    false,
    false
  );

  return output;
}

}