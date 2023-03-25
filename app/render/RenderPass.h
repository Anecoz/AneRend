#pragma once

#include "RenderContext.h"

//#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>
#include <optional>
#include <vector>

namespace render {

  class FrameGraphBuilder;
  class RenderResourceVault;

  /*
  Renderpass base class. Helps with setting up pipeline and things related to the renderpass.
  Also forces an interface to the FrameGraphBuilder.
  */
  class RenderPass
  {
  public:
    RenderPass() = default;
    virtual ~RenderPass() = default;

    // No move or copy
    RenderPass(const RenderPass&) = delete;
    RenderPass(RenderPass&&) = delete;
    RenderPass& operator=(const RenderPass&) = delete;
    RenderPass& operator=(RenderPass&&) = delete;

    // Meant to create pipeline etc as well as add resources to the vault
    // It is also possible to get already existing resources from the vault to setup descriptor sets
    virtual bool init(RenderContext* renderContext, RenderResourceVault*) = 0;

    // Register how the render pass will actually render
    virtual void registerToGraph(FrameGraphBuilder&) = 0;

    // Clean up any created resources
    virtual void cleanup(RenderContext* renderContext, RenderResourceVault*) {}

  protected:
    VkPipeline _pipeline;
    VkPipelineLayout _pipelineLayout;
    VkDescriptorSetLayout _descriptorSetLayout;

    struct GraphicsPipelineCreateParams
    {
      VkPipelineLayout pipelineLayout;

      VkDevice device = nullptr;
      VkFormat colorFormat = VK_FORMAT_B8G8R8A8_SRGB;
      VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
      std::string vertShader;
      std::string fragShader;
      int posLoc = 0;
      int colorLoc = 1;
      int normalLoc = 2;
      int uvLoc = -1;
      VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
      bool depthBiasEnable = false;
      float depthBiasConstant = 0.0f;
      float depthBiasSlope = 0.0f;
      bool depthTest = true;
      bool colorAttachment = true;
    };

    struct ComputePipelineCreateParams
    {
      VkDevice device = nullptr;
      VkPipelineLayout pipelineLayout;
      std::string shader;
    };

    struct DescriptorBindInfo
    {
      uint32_t binding;
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

      std::vector<DescriptorBindInfo> bindInfos;
    };

    struct SamplerCreateParams
    {
      RenderContext* renderContext;

      VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
      VkFilter magFilter = VK_FILTER_NEAREST;
      VkFilter minFilter = VK_FILTER_NEAREST;
      VkSamplerMipmapMode mipMapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
      float mipLodBias = 0.0f;
      float maxAnisotropy = 1.0f;
      float minLod = 0.0f;
      float maxLod = 1.0f;
      VkBorderColor borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    };

    bool buildGraphicsPipeline(GraphicsPipelineCreateParams params);

    bool buildComputePipeline(ComputePipelineCreateParams params);

    bool buildDescriptorSetLayout(DescriptorSetLayoutCreateParams params);

    bool buildPipelineLayout(PipelineLayoutCreateParams params);

    std::vector<VkDescriptorSet> buildDescriptorSets(DescriptorSetsCreateParams params);

    VkSampler createSampler(SamplerCreateParams params);
  };
}