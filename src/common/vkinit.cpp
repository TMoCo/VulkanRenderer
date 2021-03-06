#include <common/vkinit.h>

namespace vkinit {

    //-----------------------------------------------------------------------------------------------------------//
    //-PIPELINE CREATE STRUCTS-----------------------------------------------------------------------------------//
    //-----------------------------------------------------------------------------------------------------------//

    VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo (
        UI32 bindingCount,
        VkVertexInputBindingDescription* pVertexBindingDescriptions,
        UI32 attributesCount,
        VkVertexInputAttributeDescription* pVertexAttributesDescriptions,
        VkPipelineVertexInputStateCreateFlags flags) {
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.flags = flags;
        vertexInputInfo.vertexBindingDescriptionCount = bindingCount;
        vertexInputInfo.pVertexBindingDescriptions = pVertexBindingDescriptions;
        vertexInputInfo.vertexAttributeDescriptionCount = attributesCount;
        vertexInputInfo.pVertexAttributeDescriptions = pVertexAttributesDescriptions;
        return vertexInputInfo;
    }

    VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo (
        VkShaderStageFlagBits stage,
        VkShaderModule& shader,
        const char* name) {
        VkPipelineShaderStageCreateInfo shaderStageInfo{};
        shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageInfo.stage = stage;
        shaderStageInfo.module = shader;
        shaderStageInfo.pName = name;
        return shaderStageInfo;
    }

    VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo (
        VkPrimitiveTopology topology,
        VkBool32 restartEnabled,
        VkPipelineInputAssemblyStateCreateFlags flags) {
        VkPipelineInputAssemblyStateCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        info.topology = topology;
        info.flags = flags;
        info.primitiveRestartEnable = restartEnabled;
        return info;
    }

    VkPipelineRasterizationStateCreateInfo pipelineRasterStateCreateInfo (
        VkPolygonMode polyMode,
        VkCullModeFlags cullMode,
        VkFrontFace frontFace,
        VkPipelineRasterizationStateCreateFlags flags,
        F32 lineWidth) {
        VkPipelineRasterizationStateCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        info.polygonMode = polyMode;
        info.cullMode = cullMode;
        info.frontFace = frontFace;
        info.lineWidth = lineWidth;
        info.flags = flags;
        return info;
    }

    VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo (
        UI32 attachmentCount,
        const VkPipelineColorBlendAttachmentState* pAttachments) {
        VkPipelineColorBlendStateCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        info.attachmentCount = attachmentCount;
        info.pAttachments = pAttachments;
        return info;
    }

    VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo (
        VkBool32 depthTestEnable,
        VkBool32 depthWriteEnable,
        VkCompareOp depthCompareOp) {
        VkPipelineDepthStencilStateCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        info.depthTestEnable = depthTestEnable;
        info.depthWriteEnable = depthWriteEnable;
        info.depthCompareOp = depthCompareOp;
        // info.back.compareOp = VK_COMPARE_OP_ALWAYS;
        return info;
    }

    VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo (
        UI32 viewportCount,
        VkViewport* pViewports,
        UI32 scissorCount,
        VkRect2D* pScissors,
        VkPipelineViewportStateCreateFlags flags) {
        VkPipelineViewportStateCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        info.viewportCount = viewportCount;
        info.pViewports = pViewports;
        info.scissorCount = scissorCount;
        info.pScissors = pScissors;
        info.flags = flags;
        return info;
    }

    VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo (
        VkSampleCountFlagBits rasterizationSamples,
        VkPipelineMultisampleStateCreateFlags flags) {
        VkPipelineMultisampleStateCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        info.rasterizationSamples = rasterizationSamples;
        info.minSampleShading = 1.0f;
        info.flags = flags;
        return info;
    }

    VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo (
        VkDynamicState* pDynamicStates,
        UI32 dynamicStateCount,
        VkPipelineDynamicStateCreateFlags flags)
    {
        VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo{};
        pipelineDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        pipelineDynamicStateCreateInfo.pDynamicStates = pDynamicStates;
        pipelineDynamicStateCreateInfo.dynamicStateCount = dynamicStateCount;
        pipelineDynamicStateCreateInfo.flags = flags;
        return pipelineDynamicStateCreateInfo;
    }

    VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState (
        VkColorComponentFlags mask,
        VkBool32 blendEnable) {
        VkPipelineColorBlendAttachmentState blendAttachmentState{};
        blendAttachmentState.colorWriteMask = mask;
        blendAttachmentState.blendEnable = blendEnable;
        return blendAttachmentState;
    }

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo (
        UI32 layoutCount,
        VkDescriptorSetLayout* layouts,
        VkPipelineLayoutCreateFlags flags) {
        VkPipelineLayoutCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        info.setLayoutCount = layoutCount;
        info.pSetLayouts = layouts;
        info.flags = flags;
        return info;
    }

    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo (
        VkPipelineLayout layout,
        VkRenderPass renderPass,
        UI32 subpass,
        VkPipelineCreateFlags flags) {
        VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo{};
        graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        graphicsPipelineCreateInfo.layout = layout;
        graphicsPipelineCreateInfo.renderPass = renderPass;
        graphicsPipelineCreateInfo.subpass = subpass;
        graphicsPipelineCreateInfo.flags = flags;
        graphicsPipelineCreateInfo.basePipelineIndex = -1;
        graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
        return graphicsPipelineCreateInfo;
    }

    //-----------------------------------------------------------------------------------------------------------//
    //-DESCRIPTOR SET STRUCTS------------------------------------------------------------------------------------//
    //-----------------------------------------------------------------------------------------------------------//

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo(
        UI32 maxSets,
        UI32 poolSizeCount,
        VkDescriptorPoolSize* pPoolSizes,
        VkDescriptorPoolCreateFlags flags) {
        VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
        descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolCreateInfo.maxSets = maxSets;
        descriptorPoolCreateInfo.poolSizeCount = poolSizeCount;
        descriptorPoolCreateInfo.pPoolSizes = pPoolSizes;
        descriptorPoolCreateInfo.flags = flags;
        return descriptorPoolCreateInfo;
    }

    VkDescriptorSetLayoutBinding descriptorSetLayoutBinding(
        UI32 binding,
        VkDescriptorType type,
        VkPipelineStageFlags flags) {
        VkDescriptorSetLayoutBinding descSetLayoutBinding{};
        descSetLayoutBinding.descriptorType = type;
        descSetLayoutBinding.descriptorCount = 1; // change manually later if needed
        descSetLayoutBinding.binding = binding;
        descSetLayoutBinding.stageFlags = flags;
        return descSetLayoutBinding;
    }

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo(
        VkDescriptorSetLayoutBinding* bindings,
        UI32 count, 
        VkDescriptorSetLayoutCreateFlags flags) {
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
        descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutCreateInfo.bindingCount = count;
        descriptorSetLayoutCreateInfo.pBindings = bindings;
        descriptorSetLayoutCreateInfo.flags = flags;
        return descriptorSetLayoutCreateInfo;
    }

    VkDescriptorSetAllocateInfo descriptorSetAllocInfo(
        VkDescriptorPool pool,
        UI32 count,
        VkDescriptorSetLayout* pDescSetLayouts) {
        VkDescriptorSetAllocateInfo descSetAllocInfo{};
        descSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descSetAllocInfo.descriptorPool = pool;
        descSetAllocInfo.descriptorSetCount = count;
        descSetAllocInfo.pSetLayouts = pDescSetLayouts;
        return descSetAllocInfo;
    }

    VkWriteDescriptorSet writeDescriptorSet(
        VkDescriptorSet dst,
        UI32 binding,
        VkDescriptorType type,
        VkDescriptorBufferInfo* pBufferInfo) {
        VkWriteDescriptorSet writeDescriptorSet{};
        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.descriptorType = type;
        writeDescriptorSet.dstSet = dst;
        writeDescriptorSet.dstBinding = binding;
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.pBufferInfo = pBufferInfo;
        return writeDescriptorSet;
    }

    VkWriteDescriptorSet writeDescriptorSet(
        VkDescriptorSet dst,
        UI32 binding,
        VkDescriptorType type,
        VkDescriptorImageInfo* pImageInfo) {
        VkWriteDescriptorSet writeDescriptorSet{};
        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.descriptorType = type;
        writeDescriptorSet.dstSet = dst;
        writeDescriptorSet.dstBinding = binding;
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.pImageInfo = pImageInfo;
        return writeDescriptorSet;
    }

    //-----------------------------------------------------------------------------------------------------------//
    //-COMMAND BUFFER STRUCTS------------------------------------------------------------------------------------//
    //-----------------------------------------------------------------------------------------------------------//

    VkCommandPoolCreateInfo commandPoolCreateInfo(
        UI32 queueFamilyIndex,
        VkCommandPoolCreateFlags flags) {
        VkCommandPoolCreateInfo commandPoolCreateInfo{};
        commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;
        commandPoolCreateInfo.flags = flags;
        return commandPoolCreateInfo;
    }

    VkCommandBufferBeginInfo commandBufferBeginInfo(
        VkCommandBufferResetFlags flags) {
        VkCommandBufferBeginInfo commandBufferBegin{};
        commandBufferBegin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBufferBegin.flags = flags;
        return commandBufferBegin;
    }

    VkCommandBufferAllocateInfo commandBufferAllocateInfo(
        VkCommandPool commandPool,
        VkCommandBufferLevel level,
        UI32 commandBufferCount) {
        VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.commandPool = commandPool;
        commandBufferAllocateInfo.level = level;
        commandBufferAllocateInfo.commandBufferCount = commandBufferCount;
        return commandBufferAllocateInfo;
    }

    VkSubmitInfo submitInfo(
        VkPipelineStageFlags* waitStages,
        UI32 waitSemaphoreCount,
        VkSemaphore* pWaitSemaphores,
        UI32 signalSemaphoreCount,
        VkSemaphore* pSignalSemaphores,
        UI32 commandBufferCount,
        VkCommandBuffer* pCommandBuffers) {
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.waitSemaphoreCount = waitSemaphoreCount;
        submitInfo.pWaitSemaphores = pWaitSemaphores;
        submitInfo.signalSemaphoreCount = signalSemaphoreCount;
        submitInfo.pSignalSemaphores = pSignalSemaphores;
        submitInfo.commandBufferCount = commandBufferCount;
        submitInfo.pCommandBuffers = pCommandBuffers;
        return submitInfo;
    }

    //-----------------------------------------------------------------------------------------------------------//
    //-FRAME BUFFER STRUCTS--------------------------------------------------------------------------------------//
    //-----------------------------------------------------------------------------------------------------------//

    VkFramebufferCreateInfo framebufferCreateInfo (
        VkRenderPass renderPass,
        UI32 attachmentCount,
        VkImageView* pAttachments,
        VkExtent2D extent,
        UI32 layers) {
        VkFramebufferCreateInfo framebufferCreateInfo{};
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.renderPass = renderPass; 
        framebufferCreateInfo.attachmentCount = attachmentCount;
        framebufferCreateInfo.pAttachments    = pAttachments; 
        framebufferCreateInfo.width  = extent.width; 
        framebufferCreateInfo.height = extent.height;
        framebufferCreateInfo.layers = layers;
        return framebufferCreateInfo;
    }

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
        VkFlags flags) {
        VkImageCreateInfo imageCreateInfo{};
        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateInfo.extent = extent;
        imageCreateInfo.mipLevels = mipLevels;
        imageCreateInfo.arrayLayers = arrayLayers;
        imageCreateInfo.format = format;
        imageCreateInfo.tiling = tiling;
        imageCreateInfo.usage = usage;
        imageCreateInfo.flags = flags;
        return imageCreateInfo;
    }

    VkImageViewCreateInfo imageViewCreateInfo(VkImage image, 
        VkImageViewType type, 
        VkFormat format,
        VkComponentMapping componentMapping, 
        VkImageSubresourceRange subresourceRange) {
        VkImageViewCreateInfo viewCreateInfo{};
        viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCreateInfo.image = image;
        viewCreateInfo.viewType = type; // image as 1/2/3D textures and cube maps
        viewCreateInfo.format = format;
        viewCreateInfo.components = componentMapping;
        viewCreateInfo.subresourceRange = subresourceRange;
        return viewCreateInfo;
    }

    VkSamplerCreateInfo samplerCreateInfo(F32 maxAnisotropy) {
        VkSamplerCreateInfo samplerCreateInfo{};
        samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
        samplerCreateInfo.minFilter = samplerCreateInfo.magFilter;
        samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerCreateInfo.addressModeV = samplerCreateInfo.addressModeU;
        samplerCreateInfo.addressModeW = samplerCreateInfo.addressModeW;
        samplerCreateInfo.borderColor  = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerCreateInfo.anisotropyEnable = VK_TRUE; // use unless performance is a concern
        samplerCreateInfo.maxAnisotropy    = maxAnisotropy; // limits texel samples used to calculate final colours
        samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
        samplerCreateInfo.compareEnable = VK_FALSE;
        samplerCreateInfo.compareOp    = VK_COMPARE_OP_ALWAYS;
        samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerCreateInfo.mipLodBias = 0.0f;
        samplerCreateInfo.minLod = 0.0f;
        samplerCreateInfo.maxLod = 0.0f;
        return samplerCreateInfo;
    }

    //-----------------------------------------------------------------------------------------------------------//
    //-BUFFER STRUCTS--------------------------------------------------------------------------------------------//
    //-----------------------------------------------------------------------------------------------------------//

    VkBufferCreateInfo bufferCreateInfo (
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkSharingMode mode,
        VkBufferCreateFlags flags) {
        VkBufferCreateInfo bufferCreateInfo{};
        bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCreateInfo.size  = size; // allocate a buffer of the right size in bytes
        bufferCreateInfo.usage = usage; // what the data in the buffer is used for
        bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        bufferCreateInfo.flags = flags;
        return bufferCreateInfo;
    }

    //-----------------------------------------------------------------------------------------------------------//
    //-MEMORY STRUCTS--------------------------------------------------------------------------------------------//
    //-----------------------------------------------------------------------------------------------------------//

    VkMemoryAllocateInfo memoryAllocateInfo(
        VkDeviceSize size,
        UI32 memoryTypeIndex ) {
        VkMemoryAllocateInfo memoryAllocateInfo{};
        memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memoryAllocateInfo.allocationSize = size;
        memoryAllocateInfo.memoryTypeIndex = memoryTypeIndex;
        return memoryAllocateInfo;
    }

    //-----------------------------------------------------------------------------------------------------------//
    //-RENDER PASS STRUCTS---------------------------------------------------------------------------------------//
    //-----------------------------------------------------------------------------------------------------------//

    VkRenderPassBeginInfo renderPassBeginInfo (
        VkRenderPass renderPass,
        VkFramebuffer frameBuffer,
        VkExtent2D extent,
        UI32 clearValueCount,
        VkClearValue* pClearValues) {
        VkRenderPassBeginInfo renderPassBeginInfo{};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass = renderPass;
        renderPassBeginInfo.framebuffer = frameBuffer;
        renderPassBeginInfo.renderArea.extent = extent;
        renderPassBeginInfo.clearValueCount = clearValueCount;
        renderPassBeginInfo.pClearValues = pClearValues;
        return renderPassBeginInfo;
    }

    VkRenderPassCreateInfo renderPassCreateInfo (
        VkRenderPassCreateFlags flags) {
        VkRenderPassCreateInfo renderPassCreateInfo{};
        renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassCreateInfo.flags = flags;
        return renderPassCreateInfo;
    }

    //-----------------------------------------------------------------------------------------------------------//
    //-SYNCHRONISATION STRUCTS-----------------------------------------------------------------------------------//
    //-----------------------------------------------------------------------------------------------------------//

    VkSemaphoreCreateInfo semaphoreCreateInfo (
        VkSemaphoreCreateFlags flags) {
        VkSemaphoreCreateInfo semaphoreCreateInfo{};
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphoreCreateInfo.flags = flags;
        return semaphoreCreateInfo;
    }

    VkFenceCreateInfo fenceCreateInfo (
        VkFenceCreateFlags flags) {
        VkFenceCreateInfo fenceCreateInfo{};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.flags = flags;
        return fenceCreateInfo;
    }
    
}