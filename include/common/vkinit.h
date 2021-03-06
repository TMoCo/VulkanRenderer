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

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo(
        UI32 maxSets,
        UI32 poolSizeCount,
        VkDescriptorPoolSize* pPoolSizes,
        VkDescriptorPoolCreateFlags flags = 0);

    VkDescriptorSetLayoutBinding descriptorSetLayoutBinding(
        UI32 binding,
        VkDescriptorType type,
        VkPipelineStageFlags flags = 0);

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo(
        VkDescriptorSetLayoutBinding* bindings,
        UI32 count,
        VkDescriptorSetLayoutCreateFlags flags = 0);

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
    
    VkCommandPoolCreateInfo commandPoolCreateInfo(
        UI32 queueFamilyIndex, 
        VkCommandPoolCreateFlags flags);

    VkCommandBufferBeginInfo commandBufferBeginInfo(
        VkCommandBufferResetFlags flags = 0);

    VkCommandBufferAllocateInfo commandBufferAllocateInfo(
        VkCommandPool commandPool,
        VkCommandBufferLevel level,
        UI32 commandBufferCount);

    VkSubmitInfo submitInfo(
        VkPipelineStageFlags* waitStages,
        UI32 waitSemaphoreCount,
        VkSemaphore* pWaitSemaphores,
        UI32 signalSemaphoreCount,
        VkSemaphore* pSignalSemaphores,
        UI32 commandBufferCount,
        VkCommandBuffer* pCommandBuffers);

    //-----------------------------------------------------------------------------------------------------------//
    //-FRAME BUFFER STRUCTS--------------------------------------------------------------------------------------//
    //-----------------------------------------------------------------------------------------------------------//

    VkFramebufferCreateInfo framebufferCreateInfo(
        VkRenderPass renderPass,
        UI32 attachmentCount,
        VkImageView* pAttachments,
        VkExtent2D extent,
        UI32 layers);

    //-----------------------------------------------------------------------------------------------------------//
    //-IMAGE STRUCTS---------------------------------------------------------------------------------------------//
    //-----------------------------------------------------------------------------------------------------------//

    VkImageCreateInfo imageCreateInfo(
        VkFormat format,
        VkExtent3D extent,
        UI32 mipLevels,
        UI32 arrayLayers,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkFlags = 0
    );

    VkImageViewCreateInfo imageViewCreateInfo(
        VkImage image, 
        VkImageViewType type, 
        VkFormat format,
        VkComponentMapping componentMapping, 
        VkImageSubresourceRange subresourceRange);

    VkSamplerCreateInfo samplerCreateInfo(
        F32 maxAnisotropy = 1.0f);

    //-----------------------------------------------------------------------------------------------------------//
    //-BUFFER STRUCTS--------------------------------------------------------------------------------------------//
    //-----------------------------------------------------------------------------------------------------------//

    VkBufferCreateInfo bufferCreateInfo(
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkSharingMode mode = VK_SHARING_MODE_EXCLUSIVE,
        VkBufferCreateFlags flags = 0);

    //-----------------------------------------------------------------------------------------------------------//
    //-MEMORY STRUCTS--------------------------------------------------------------------------------------------//
    //-----------------------------------------------------------------------------------------------------------//

    VkMemoryAllocateInfo memoryAllocateInfo(
        VkDeviceSize size,
        UI32 memoryTypeIndex
    );

    //-----------------------------------------------------------------------------------------------------------//
    //-RENDER PASS STRUCTS---------------------------------------------------------------------------------------//
    //-----------------------------------------------------------------------------------------------------------//

    VkRenderPassBeginInfo renderPassBeginInfo(
        VkRenderPass renderPass,
        VkFramebuffer frameBuffer,
        VkExtent2D extent,
        UI32 clearValueCount,
        VkClearValue* pClearValues);

    VkRenderPassCreateInfo renderPassCreateInfo(
        VkRenderPassCreateFlags flags = 0);

    //-----------------------------------------------------------------------------------------------------------//
    //-SYNCHRONISATION STRUCTS-----------------------------------------------------------------------------------//
    //-----------------------------------------------------------------------------------------------------------//

    VkSemaphoreCreateInfo semaphoreCreateInfo(
        VkSemaphoreCreateFlags flags = 0);

    VkFenceCreateInfo fenceCreateInfo(
        VkFenceCreateFlags flags = 0);
}


#endif // !VKINIT_H