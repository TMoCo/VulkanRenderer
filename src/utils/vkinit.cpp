#include <utils/vkinit.h>

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

    //-----------------------------------------------------------------------------------------------------------//
    //-IMAGE STRUCTS---------------------------------------------------------------------------------------------//
    //-----------------------------------------------------------------------------------------------------------//

    VkImageViewCreateInfo imageViewCreateInfo(VkImage image, VkImageViewType type, VkFormat format,
        VkComponentMapping componentMapping, VkImageSubresourceRange subresourceRange) {
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

        // how to interpolate texels that are magnified or minified
        samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
        samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
        // addressing mode
        samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerCreateInfo.addressModeV = samplerCreateInfo.addressModeU;
        samplerCreateInfo.addressModeW = samplerCreateInfo.addressModeW;
        // VK_SAMPLER_ADDRESS_MODE_REPEAT: Repeat the texture when going beyond the image dimensions.
        // VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT: Like repeat, but inverts the coordinates to mirror the image when going beyond the dimensions.
        // VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE : Take the color of the edge closest to the coordinate beyond the image dimensions.
        // VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE : Like clamp to edge, but instead uses the edge opposite to the closest edge.
        // VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER : Return a solid color when sampling beyond the dimensions of the image

        // use unless performance is a concern
        samplerCreateInfo.anisotropyEnable = VK_TRUE;
        // limits texel samples that used to calculate final colours
        samplerCreateInfo.maxAnisotropy = maxAnisotropy;
        samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        // which coordinate system we want to use to address texels!
        samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

        // if comparison enabled, texels will be compared to a value and result is used in filtering (useful for shadow maps)
        samplerCreateInfo.compareEnable = VK_FALSE;
        samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;

        // mipmapping fields
        samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerCreateInfo.mipLodBias = 0.0f;
        samplerCreateInfo.minLod = 0.0f;
        samplerCreateInfo.maxLod = 0.0f;

        return samplerCreateInfo;
    }
}