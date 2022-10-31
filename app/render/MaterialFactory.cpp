#include "MaterialFactory.h"

#include "../util/Utils.h"
#include "BufferHelpers.h"
#include "StandardUBO.h"
#include "ShadowUBO.h"

#include "Vertex.h" // For binding description offsets

#include <vulkan/vulkan.h>

#include <cstdint>
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

bool internalCreateDescriptorSets(
  VkDevice device,
  VkDescriptorSetLayout& descriptorSetLayout, std::vector<VkDescriptorSet>& descriptorSets,
  std::size_t numCopies,
  VkDescriptorPool& descriptorPool,
  const std::vector<render::AllocatedBuffer>& uboBuffers, std::size_t uboBufferRange,
  const std::vector<render::AllocatedBuffer>& gpuRenderableBuffers, const std::vector<render::AllocatedBuffer>& gpuTranslationBuffers,
  const std::vector<VkImageView*> imageViews, const std::vector<VkSampler*> samplers, VkImageLayout imageLayout,
  int uboBinding, int samplerBinding, int gpuRenderableBinding, int gpuTranslationBinding)
{
  std::vector<VkDescriptorSetLayout> layouts(numCopies, descriptorSetLayout);
  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = descriptorPool;
  allocInfo.descriptorSetCount = static_cast<uint32_t>(numCopies);
  allocInfo.pSetLayouts = layouts.data();

  descriptorSets.resize(numCopies);
  if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
    printf("failed to allocate descriptor sets!\n");
    return false;
  }

  for (size_t i = 0; i < numCopies; i++) {
    std::vector<VkWriteDescriptorSet> descriptorWrites;

    VkDescriptorBufferInfo bufferInfo{};
    VkWriteDescriptorSet bufWrite{};
    if (!uboBuffers.empty()) {
      bufferInfo.buffer = uboBuffers[i]._buffer;
      bufferInfo.offset = 0;
      bufferInfo.range = uboBufferRange;
      
      bufWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      bufWrite.dstSet = descriptorSets[i];
      bufWrite.dstBinding = uboBinding;
      bufWrite.dstArrayElement = 0;
      bufWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      bufWrite.descriptorCount = 1;
      bufWrite.pBufferInfo = &bufferInfo;

      descriptorWrites.emplace_back(std::move(bufWrite));
    }

    if (!gpuRenderableBuffers.empty()) {
      VkDescriptorBufferInfo bufferInfo{};
      VkWriteDescriptorSet bufWrite{};

      bufferInfo.buffer = gpuRenderableBuffers[i]._buffer;
      bufferInfo.offset = 0;
      bufferInfo.range = VK_WHOLE_SIZE;

      bufWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      bufWrite.dstSet = descriptorSets[i];
      bufWrite.dstBinding = gpuRenderableBinding;
      bufWrite.dstArrayElement = 0;
      bufWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
      bufWrite.descriptorCount = 1;
      bufWrite.pBufferInfo = &bufferInfo;

      descriptorWrites.emplace_back(std::move(bufWrite));
    }

    if (!gpuTranslationBuffers.empty()) {
      VkDescriptorBufferInfo bufferInfo{};
      VkWriteDescriptorSet bufWrite{};

      bufferInfo.buffer = gpuTranslationBuffers[i]._buffer;
      bufferInfo.offset = 0;
      bufferInfo.range = VK_WHOLE_SIZE;

      bufWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      bufWrite.dstSet = descriptorSets[i];
      bufWrite.dstBinding = gpuTranslationBinding;
      bufWrite.dstArrayElement = 0;
      bufWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
      bufWrite.descriptorCount = 1;
      bufWrite.pBufferInfo = &bufferInfo;

      descriptorWrites.emplace_back(std::move(bufWrite));
    }

    std::vector<VkDescriptorImageInfo> imageInfos;
    VkWriteDescriptorSet imWrite{};
    if (!imageViews.empty()) {
      for (std::size_t i = 0; i < imageViews.size(); ++i) {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = imageLayout;
        imageInfo.imageView = *(imageViews[i]);
        imageInfo.sampler = *(samplers[i]);
        imageInfos.emplace_back(std::move(imageInfo));
      }
    
      imWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      imWrite.dstSet = descriptorSets[i];
      imWrite.dstBinding = samplerBinding;
      imWrite.dstArrayElement = 0;
      imWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      imWrite.descriptorCount = (uint32_t)imageViews.size();
      imWrite.pImageInfo = imageInfos.data();

      descriptorWrites.emplace_back(std::move(imWrite));
    }

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
  }

  return true;
}

bool internalCreate(
  VkDescriptorSetLayout& descriptorSetLayout, VkPipelineLayout& pipelineLayout, VkPipeline& pipeline,
  VkDevice device, VkFormat colorFormat, VkFormat depthFormat,
  int uboBinding, int samplerBinding, int gpuRenderableBinding, int gpuTranslationBinding,
  std::size_t samplerCount,
  const std::string& vertShader, const std::string& fragShader,
  bool instanceBuffer,
  int posLoc, int colorLoc, int normalLoc, int uvLoc,
  VkCullModeFlags cullMode,
  bool depthBiasEnable, float depthBiasConstant, float depthBiasSlope,
  bool depthTest,
  bool colorAttachment,
  bool pushConstants,
  std::size_t pushConstantsSize
  )
{
  // Descriptor set layout
  std::vector<VkDescriptorSetLayoutBinding> bindings;
  VkDescriptorSetLayoutBinding uboLayoutBinding{};
  if (uboBinding != -1) {
    uboLayoutBinding.binding = uboBinding;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings.emplace_back(std::move(uboLayoutBinding));
  }

  if (gpuRenderableBinding != - 1) {
    VkDescriptorSetLayoutBinding renderableLayoutBinding{};
    renderableLayoutBinding.binding = gpuRenderableBinding;
    renderableLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    renderableLayoutBinding.descriptorCount = 1;
    renderableLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    bindings.emplace_back(std::move(renderableLayoutBinding));
  }

  if (gpuTranslationBinding != -1) {
    VkDescriptorSetLayoutBinding translationBinding{};
    translationBinding.binding = gpuTranslationBinding;
    translationBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    translationBinding.descriptorCount = 1;
    translationBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    bindings.emplace_back(std::move(translationBinding));
  }

  VkDescriptorSetLayoutBinding samplerLayoutBinding{};
  if (samplerBinding != -1) {
    samplerLayoutBinding.binding = samplerBinding;
    samplerLayoutBinding.descriptorCount = (uint32_t)samplerCount;
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
    pushConstant.size = (uint32_t)pushConstantsSize;
    pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
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

render::Material internalCreatePP(
  VkDevice device, VkFormat colorFormat, VkFormat depthFormat, std::size_t numCopies,
  VkDescriptorPool& descriptorPool, VmaAllocator& vmaAllocator,
  VkImageView* imageView, VkSampler* sampler, VkImageLayout samplerImageLayout,
  const std::string& vert, const std::string& frag)
{
  render::Material output;
  output._supportsPostProcessingPass = true;

  if (!internalCreate(
    output._descriptorSetLayouts[render::Material::POST_PROCESSING_INDEX], output._pipelineLayouts[render::Material::POST_PROCESSING_INDEX], output._pipelines[render::Material::POST_PROCESSING_INDEX],
    device, colorFormat, depthFormat,
    -1, 0, -1, -1, 1,
    vert, frag,
    false,
    0, -1, -1, 1,
    VK_CULL_MODE_BACK_BIT,
    false, 0.0f, 0.0f,
    false,
    true,
    false, 0)) {
    printf("Could not create pp flip mat!\n");
    return output;
  }

  if (!internalCreateDescriptorSets(
    device,
    output._descriptorSetLayouts[render::Material::POST_PROCESSING_INDEX], output._descriptorSets[render::Material::POST_PROCESSING_INDEX],
    numCopies, descriptorPool,
    {}, 0,
    {}, {},
    {imageView}, {sampler}, samplerImageLayout,
    -1, 0, -1, -1)) {
    printf("Could not create pp flip descriptor sets!\n");
    return output;
  }

  return output;
}

}

namespace render {

Material MaterialFactory::createStandardMaterial(
  VkDevice device, VkFormat colorFormat, VkFormat depthFormat, std::size_t numCopies,
  VkDescriptorPool& descriptorPool, VmaAllocator& vmaAllocator,
  const std::vector<AllocatedBuffer>& gpuRenderableBuffers, const std::vector<AllocatedBuffer>& gpuTranslationBuffers,
  const std::vector<VkImageView*>& textureImageViews, const std::vector<VkSampler*>& samplers, VkImageLayout samplerImageLayout)
{
  Material output;

  output._supportsPushConstants = true;
  output._supportsShadowPass = true;

  // Non-shadow
  internalCreate(
    output._descriptorSetLayouts[Material::STANDARD_INDEX], output._pipelineLayouts[Material::STANDARD_INDEX], output._pipelines[Material::STANDARD_INDEX],
    device, colorFormat, depthFormat,
    0, 1, 2, 3, textureImageViews.size(),
    "standard_vert.spv", "standard_frag.spv",
    false,
    0, 1, 2, -1,
    VK_CULL_MODE_BACK_BIT,
    false, 0.0f, 0.0f, 
    true,
    true,
    true, 4 * 4 * 4);

  internalCreate(
    output._descriptorSetLayouts[Material::SHADOW_INDEX], output._pipelineLayouts[Material::SHADOW_INDEX], output._pipelines[Material::SHADOW_INDEX],
    device, colorFormat, depthFormat,
    0, -1, 1, 2, 0,
    "standard_shadow_vert.spv", "standard_shadow_frag.spv",
    false, 
    0, -1, -1, -1,
    VK_CULL_MODE_BACK_BIT,
    true, 4.0f, 1.5f,
    true,
    false,
    true, 4 * 4 * 4 + 4 * 4); //mat4 and vec4

  // Non-shadow descriptor sets
  {
    VkDeviceSize bufferSize = sizeof(StandardUBO);

    output._ubos[Material::STANDARD_INDEX].resize(numCopies);

    for (size_t i = 0; i < numCopies; i++) {
      bufferutil::createBuffer(vmaAllocator, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, output._ubos[Material::STANDARD_INDEX][i]);
    }

    if (!internalCreateDescriptorSets(
          device, output._descriptorSetLayouts[Material::STANDARD_INDEX], output._descriptorSets[Material::STANDARD_INDEX],
          numCopies, descriptorPool, output._ubos[Material::STANDARD_INDEX], bufferSize,
          gpuRenderableBuffers, gpuTranslationBuffers,
          textureImageViews, samplers, samplerImageLayout, 0, 1, 2, 3)) {
      printf("Could not create descriptor set!\n");
      return output;
    }
  }

  // shadow descriptor sets
  {
    VkDeviceSize bufferSize = sizeof(ShadowUBO);

    output._ubos[Material::SHADOW_INDEX].resize(numCopies);

    for (size_t i = 0; i < numCopies; i++) {
      bufferutil::createBuffer(vmaAllocator, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, output._ubos[Material::SHADOW_INDEX][i]);
    }

    if (!internalCreateDescriptorSets(
          device, output._descriptorSetLayouts[Material::SHADOW_INDEX], output._descriptorSets[Material::SHADOW_INDEX],
          numCopies, descriptorPool, output._ubos[Material::SHADOW_INDEX], bufferSize,
          gpuRenderableBuffers, gpuTranslationBuffers,
          {}, {}, samplerImageLayout, 0, -1, 1, 2)) {
      printf("Could not create descriptor set!\n");
      return output;
    }
  }

  return output;
}

Material MaterialFactory::createShadowDebugMaterial(
  VkDevice device, VkFormat colorFormat, VkFormat depthFormat, std::size_t numCopies,
  VkDescriptorPool& descriptorPool, VmaAllocator& vmaAllocator,
  VkImageView* imageView, VkSampler* sampler, VkImageLayout samplerImageLayout)
{
  Material output;

  internalCreate(
    output._descriptorSetLayouts[Material::STANDARD_INDEX], output._pipelineLayouts[Material::STANDARD_INDEX], output._pipelines[Material::STANDARD_INDEX],
    device, colorFormat, depthFormat,
    -1, 0, -1, -1,  1,
    "shadow_debug_vert.spv", "shadow_debug_frag.spv",
    false,
    0, -1, -1, 1,
    VK_CULL_MODE_BACK_BIT,
    false, 0.0f, 0.0f,
    false,
    true,
    false, 0
  );

  if (!internalCreateDescriptorSets(
        device, output._descriptorSetLayouts[Material::STANDARD_INDEX], output._descriptorSets[Material::STANDARD_INDEX],
        numCopies, descriptorPool, {}, 0, {}, {},
        {imageView}, {sampler}, samplerImageLayout, -1, 0, -1, -1)) {
    printf("Could not create descriptor set!\n");
    return output;
  }

  return output;
}

Material MaterialFactory::createStandardInstancedMaterial(
  VkDevice device, VkFormat colorFormat, VkFormat depthFormat, std::size_t numCopies,
  VkDescriptorPool& descriptorPool, VmaAllocator& vmaAllocator,
  const std::vector<VkImageView*>& textureImageViews, const std::vector<VkSampler*>& samplers, VkImageLayout samplerImageLayout)
{
  Material output;

  output._hasInstanceBuffer = true;
  output._supportsShadowPass = true;

  internalCreate(
    output._descriptorSetLayouts[Material::STANDARD_INDEX], output._pipelineLayouts[Material::STANDARD_INDEX], output._pipelines[Material::STANDARD_INDEX],
    device, colorFormat, depthFormat,
    0, 1, -1, -1, (uint32_t)samplers.size(),
    "standard_instanced_vert.spv", "standard_frag.spv",
    true,
    0, 1, 2, -1,
    VK_CULL_MODE_BACK_BIT,
    false, 0.0f, 0.0f, 
    true,
    true,
    false, 0
  );

  internalCreate(
    output._descriptorSetLayouts[Material::SHADOW_INDEX], output._pipelineLayouts[Material::SHADOW_INDEX], output._pipelines[Material::SHADOW_INDEX],
    device, colorFormat, depthFormat,
    0, -1, -1, -1, 0,
    "standard_instanced_shadow_vert.spv", "",
    true,
    0, -1, -1, -1,
    VK_CULL_MODE_BACK_BIT,
    true, 4.0f, 1.5f, 
    true,
    false,
    true, 4 * 4 * 4
  );

  // Non-shadow descriptor sets
  {
    VkDeviceSize bufferSize = sizeof(StandardUBO);

    output._ubos[Material::STANDARD_INDEX].resize(numCopies);

    for (size_t i = 0; i < numCopies; i++) {
      bufferutil::createBuffer(vmaAllocator, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, output._ubos[Material::STANDARD_INDEX][i]);
    }

    if (!internalCreateDescriptorSets(
          device, output._descriptorSetLayouts[Material::STANDARD_INDEX], output._descriptorSets[Material::STANDARD_INDEX],
          numCopies, descriptorPool, output._ubos[Material::STANDARD_INDEX], bufferSize,
          {}, {},
          textureImageViews, samplers, samplerImageLayout, 0, 1, -1, -1)) {
      printf("Could not create descriptor set!\n");
      return output;
    }
  }

  // shadow descriptor sets
  {
    VkDeviceSize bufferSize = sizeof(ShadowUBO);

    output._ubos[Material::SHADOW_INDEX].resize(numCopies);

    for (size_t i = 0; i < numCopies; i++) {
      bufferutil::createBuffer(vmaAllocator, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, output._ubos[Material::SHADOW_INDEX][i]);
    }

    if (!internalCreateDescriptorSets(
          device, output._descriptorSetLayouts[Material::SHADOW_INDEX], output._descriptorSets[Material::SHADOW_INDEX],
          numCopies, descriptorPool, output._ubos[Material::SHADOW_INDEX], bufferSize,
          {}, {},
          {}, {}, samplerImageLayout, 0, -1, -1, -1)) {
      printf("Could not create descriptor set!\n");
      return output;
    }
  }

  return output;
}

Material MaterialFactory::createPPColorInvMaterial(
    VkDevice device, VkFormat colorFormat, VkFormat depthFormat, std::size_t numCopies,
    VkDescriptorPool& descriptorPool, VmaAllocator& vmaAllocator,
    VkImageView* imageView, VkSampler* sampler, VkImageLayout samplerImageLayout)
{
  return internalCreatePP(
    device, colorFormat, depthFormat,
    numCopies, descriptorPool, vmaAllocator,
    imageView, sampler, samplerImageLayout,
    "pp_vert.spv", "pp_color_inv_frag.spv");
}

Material MaterialFactory::createPPFlipMaterial(
    VkDevice device, VkFormat colorFormat, VkFormat depthFormat, std::size_t numCopies,
    VkDescriptorPool& descriptorPool, VmaAllocator& vmaAllocator,
    VkImageView* imageView, VkSampler* sampler, VkImageLayout samplerImageLayout)
{
  return internalCreatePP(
    device, colorFormat, depthFormat,
    numCopies, descriptorPool, vmaAllocator,
    imageView, sampler, samplerImageLayout,
    "pp_vert.spv", "pp_flip_frag.spv");
}

Material MaterialFactory::createPPBlurMaterial(
    VkDevice device, VkFormat colorFormat, VkFormat depthFormat, std::size_t numCopies,
    VkDescriptorPool& descriptorPool, VmaAllocator& vmaAllocator,
    VkImageView* imageView, VkSampler* sampler, VkImageLayout samplerImageLayout)
{
  return internalCreatePP(
    device, colorFormat, depthFormat,
    numCopies, descriptorPool, vmaAllocator,
    imageView, sampler, samplerImageLayout,
    "pp_vert.spv", "pp_blur_frag.spv");
}

Material MaterialFactory::createPPFxaaMaterial(
  VkDevice device, VkFormat colorFormat, VkFormat depthFormat, std::size_t numCopies,
  VkDescriptorPool& descriptorPool, VmaAllocator& vmaAllocator,
  VkImageView* imageView, VkSampler* sampler, VkImageLayout samplerImageLayout)
{
  return internalCreatePP(
    device, colorFormat, depthFormat,
    numCopies, descriptorPool, vmaAllocator,
    imageView, sampler, samplerImageLayout,
    "pp_vert.spv", "pp_fxaa_frag.spv");
}

}