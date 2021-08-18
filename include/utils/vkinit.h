//
// namespace for initialising vulkan structures
//

#ifndef VKINIT_H
#define VKINIT_H

// inspired from: https://github.com/SaschaWillems/Vulkan/blob/master/base/VulkanInitializers.hpp

#include <vulkan/vulkan_core.h> // vulkan core structs &c

#include <common/types.h>

namespace vkinit {

    //-----------------------------------------------------------------------------------------------------------//
    //-PIPELINE CREATE STRUCTS-----------------------------------------------------------------------------------//
    //-----------------------------------------------------------------------------------------------------------//

    VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo(
        UI32 bindingCount,
        VkVertexInputBindingDescription* pVertexBindingDescriptions,
        UI32 attributesCount,
        VkVertexInputAttributeDescription* pVertexAttributesDescriptions,
        VkPipelineVertexInputStateCreateFlags flags = 0);

    VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo(
        VkShaderStageFlagBits stage,
        VkShaderModule& shader,
        const char* name);

    VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo(
        VkPrimitiveTopology topology,
        VkBool32 restartEnabled,
        VkPipelineInputAssemblyStateCreateFlags flags = 0);

    VkPipelineRasterizationStateCreateInfo pipelineRasterStateCreateInfo(
        VkPolygonMode polyMode,
        VkCullModeFlags cullMode,
        VkFrontFace frontFace,
        VkPipelineRasterizationStateCreateFlags flags = 0,
        F32 lineWidth = 1.0f);

    VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo(
        UI32 attachmentCount,
        const VkPipelineColorBlendAttachmentState* pAttachments);

    VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo(
        VkBool32 depthTestEnable,
        VkBool32 depthWriteEnable,
        VkCompareOp depthCompareOp);

    VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo(
        UI32 viewportCount,
        VkViewport* pViewports,
        UI32 scissorCount,
        VkRect2D* pScissors,
        VkPipelineViewportStateCreateFlags flags = 0);

    VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo(
        VkSampleCountFlagBits rasterizationSamples,
        VkPipelineMultisampleStateCreateFlags flags = 0);

    VkPipelineDynamicStateCreateInfo  pipelineDynamicStateCreateInfo(
        VkDynamicState* pDynamicStates,
        UI32 dynamicStateCount,
        VkPipelineDynamicStateCreateFlags flags = 0);

    VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState(
        VkColorComponentFlags mask,
        VkBool32 blendEnable);

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo(
        UI32 layoutCount,
        VkDescriptorSetLayout* layouts,
        VkPipelineLayoutCreateFlags flags = 0);

    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo(
        VkPipelineLayout layout,
        VkRenderPass renderPass,
        UI32 subpass,
        VkPipelineCreateFlags flags = 0);

    //-----------------------------------------------------------------------------------------------------------//
    //-DESCRIPTOR SET STRUCTS------------------------------------------------------------------------------------//
    //-----------------------------------------------------------------------------------------------------------//

    VkDescriptorSetLayoutBinding descriptorSetLayoutBinding(
        UI32 binding,
        VkDescriptorType type,
        VkPipelineStageFlags flags = 0);

    VkDescriptorSetAllocateInfo descriptorSetAllocInfo(
        VkDescriptorPool pool,
        UI32 count,
        VkDescriptorSetLayout* pDescSetLayouts);

    VkWriteDescriptorSet writeDescriptorSet(
        VkDescriptorSet dst,
        UI32 binding,
        VkDescriptorType type,
        VkDescriptorBufferInfo* pBufferInfo);

    VkWriteDescriptorSet writeDescriptorSet(
        VkDescriptorSet dst,
        UI32 binding,
        VkDescriptorType type,
        VkDescriptorImageInfo* pImageInfo);

    //-----------------------------------------------------------------------------------------------------------//
    //-COMMAND BUFFER STRUCTS------------------------------------------------------------------------------------//
    //-----------------------------------------------------------------------------------------------------------//
    
    VkCommandBufferBeginInfo commandBufferBeginInfo(
        VkCommandBufferResetFlags flags = 0);
 
    //-----------------------------------------------------------------------------------------------------------//
    //-IMAGE STRUCTS---------------------------------------------------------------------------------------------//
    //-----------------------------------------------------------------------------------------------------------//

    VkImageViewCreateInfo imageViewCreateInfo(VkImage image, VkImageViewType type, VkFormat format,
        VkComponentMapping componentMapping, VkImageSubresourceRange subresourceRange);

    VkSamplerCreateInfo samplerCreateInfo(F32 maxAnisotropy = 1.0f);
}


#endif // !VKINIT_H