#pragma once

#include <vulkan/vulkan.h>

#include "ShaderBindingTable.h"

#include <string>
#include <vector>

namespace render {

class RenderContext;

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
  int tangentLoc = -1;
  int jointLoc = -1;
  int jointWeightLoc = -1;
  VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
  bool depthBiasEnable = false;
  float depthBiasConstant = 0.0f;
  float depthBiasSlope = 0.0f;
  bool depthTest = true;
  bool colorAttachment = true;
  bool vertexLess = false;
  VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
  uint32_t viewMask = 0;
};

struct ComputePipelineCreateParams
{
  VkDevice device = nullptr;
  std::string shader;
};

struct RayTracingPipelineCreateParams
{
  VkDevice device = nullptr;
  RenderContext* rc;
  std::string raygenShader;
  std::string missShader;
  std::string closestHitShader;
  uint32_t maxRecursionDepth = 1;
};

struct DescriptorBindInfo
{
  bool bindless = false;

  // This binding has to be the _last_ binding of the descriptors if we're bindless
  uint32_t binding;
  uint32_t descriptorCount = 1;
  uint32_t dstArrayElement = 0;
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

  VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  VkFilter magFilter = VK_FILTER_LINEAR;
  VkFilter minFilter = VK_FILTER_LINEAR;
  VkSamplerMipmapMode mipMapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  float mipLodBias = 0.0f;
  float maxAnisotropy = 8.0f;
  float minLod = 0.0f;
  float maxLod = 10.0f;
  VkBorderColor borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
  bool useMaxFilter = false;
};

bool buildPipelineLayout(PipelineLayoutCreateParams params, VkPipelineLayout& outPipelineLayout);
bool buildDescriptorSetLayout(DescriptorSetLayoutCreateParams params, VkDescriptorSetLayout& outDescriptorSetLayout, VkPipelineLayout& outPipelineLayout);
std::vector<VkDescriptorSet> buildDescriptorSets(DescriptorSetsCreateParams params);
VkSampler createSampler(SamplerCreateParams params);
bool buildComputePipeline(ComputePipelineCreateParams params, VkPipelineLayout& pipelineLayout, VkPipeline& outPipeline);
bool buildGraphicsPipeline(GraphicsPipelineCreateParams param, VkPipelineLayout& pipelineLayout, VkPipeline& outPipeline);
bool buildRayTracingPipeline(RayTracingPipelineCreateParams param, VkPipelineLayout& pipelineLayout, VkPipeline& outPipeline, ShaderBindingTable& sbtOut);

}