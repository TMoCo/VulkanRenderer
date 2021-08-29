#include <hpg/Renderer.h>
#include <hpg/Shader.h>
#include <hpg/Image.h>

#include <common/vkinit.h>

void Renderer::init(GLFWwindow* window) {
	_context.init(window);

    createCommandPool(&_commandPools[RENDER_CMD_POOL], VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    createCommandPool(&_commandPools[GUI_CMD_POOL], VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    _swapChain.init(&_context);

    // gbuffer attachments
    createAttachment(_gbuffer[GBUFFER_POSITION], 0x94, _swapChain.extent(), VK_FORMAT_R16G16B16A16_SFLOAT);
    createAttachment(_gbuffer[GBUFFER_NORMAL], 0x94, _swapChain.extent(), VK_FORMAT_R16G16B16A16_SFLOAT);
    createAttachment(_gbuffer[GBUFFER_ALBEDO], 0x94, _swapChain.extent(), VK_FORMAT_R8G8B8A8_SRGB);
    createAttachment(_gbuffer[GBUFFER_AO_METALLIC_ROUGHNESS], 0x94, _swapChain.extent(), VK_FORMAT_R8G8B8A8_UNORM);
    createAttachment(_gbuffer[GBUFFER_DEPTH], 0xa4, _swapChain.extent(), utils::findDepthFormat(_context.physicalDevice));

    // build render pass
    createRenderPass();
    createGuiRenderPass();
    createFramebuffers();

    // command buffers
    createCommandBuffers();

    // descriptor layouts
    createDescriptorPool();
    createDescriptorSetLayouts();

    // composition pipeline
    createCompositionPipeline();

    // create the uniform buffer for composition
    _compositionUniforms = Buffer::createBuffer(_context, sizeof(CompositionUBO) * _swapChain.imageCount(), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    createCompositionDescriptorSets();

    createSyncObjects();

    createColorSampler();
}

void Renderer::cleanup() {
    for (UI32 i = 0; i < _swapChain.imageCount(); i++) {
        vkDestroySemaphore(_context.device, _renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(_context.device, _imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(_context.device, _inFlightFences[i], nullptr);
    }

    for (size_t i = 0; i < _swapChain.imageCount(); i++) {
        vkDestroyFramebuffer(_context.device, _framebuffers[i], nullptr);
        vkDestroyFramebuffer(_context.device, _guiFramebuffers[i], nullptr);
    }

    for (auto& attachment : _gbuffer) {
        attachment.cleanup(_context.device);
    }

    vkDestroyDescriptorPool(_context.device, _descriptorPool, nullptr);
    //vkDestroyDescriptorSetLayout(_context.device, descriptorSetLayout, nullptr);

    vkDestroySampler(_context.device, _colorSampler, nullptr);

    // destroy the render passes
    vkDestroyRenderPass(_context.device, _guiRenderPass, nullptr);
    vkDestroyRenderPass(_context.device, _renderPass, nullptr);

    _swapChain.cleanup();

    vkDestroyCommandPool(_context.device, _commandPools[RENDER_CMD_POOL], nullptr);
    vkDestroyCommandPool(_context.device, _commandPools[GUI_CMD_POOL], nullptr);

    _context.cleanup();
}

void Renderer::recreateSwapchain() {

    vkFreeCommandBuffers(_context.device, _commandPools[RENDER_CMD_POOL],
        static_cast<UI32>(_renderCommandBuffers.size()), _renderCommandBuffers.data());
    vkFreeCommandBuffers(_context.device, _commandPools[GUI_CMD_POOL],
        static_cast<UI32>(_guiCommandBuffers.size()), _guiCommandBuffers.data());

    // destroy previous swapchain and dependencies 
    for (UI32 i = 0; i < _swapChain.imageCount(); i++) {
        vkDestroyFramebuffer(_context.device, _framebuffers[i], nullptr);
        vkDestroyFramebuffer(_context.device, _guiFramebuffers[i], nullptr);
    }

    vkDestroyRenderPass(_context.device, _guiRenderPass, nullptr);
    vkDestroyRenderPass(_context.device, _renderPass, nullptr);

    for (auto& attachment : _gbuffer) {
        attachment.cleanup(_context.device);
    }
    
    _swapChain.cleanup();

    // recreate swapchain and dependencies

    _swapChain.init(&_context);

    createAttachment(_gbuffer[GBUFFER_POSITION], 0x94, _swapChain.extent(), VK_FORMAT_R16G16B16A16_SFLOAT);
    createAttachment(_gbuffer[GBUFFER_NORMAL], 0x94, _swapChain.extent(), VK_FORMAT_R16G16B16A16_SFLOAT);
    createAttachment(_gbuffer[GBUFFER_ALBEDO], 0x94, _swapChain.extent(), VK_FORMAT_R8G8B8A8_SRGB);
    createAttachment(_gbuffer[GBUFFER_AO_METALLIC_ROUGHNESS], 0x94, _swapChain.extent(), VK_FORMAT_R16G16B16A16_SFLOAT);
    createAttachment(_gbuffer[GBUFFER_DEPTH], 0xa4, _swapChain.extent(), utils::findDepthFormat(_context.physicalDevice));

    createRenderPass();
    createGuiRenderPass();

    createFramebuffers();

    createCommandBuffers();
}

void Renderer::render() {
    // TODO: update uniform management and gui setup before moving render out of application class
}

void Renderer::createCommandPool(VkCommandPool* commandPool, VkCommandPoolCreateFlags flags) {
    utils::QueueFamilyIndices queueFamilyIndices =
        utils::QueueFamilyIndices::findQueueFamilies(_context.physicalDevice, _context.surface);
    VkCommandPoolCreateInfo poolInfo = vkinit::commandPoolCreateInfo(queueFamilyIndices.graphicsFamily.value(), flags);
    if (vkCreateCommandPool(_context.device, &poolInfo, nullptr, commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
}

void Renderer::createSyncObjects() {
    _imageAvailableSemaphores.resize(_swapChain.imageCount());
    _renderFinishedSemaphores.resize(_swapChain.imageCount());

    _inFlightFences.resize(_swapChain.imageCount());
    _imagesInFlight.resize(_swapChain.imageCount(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphoreCreateInfo();

    VkFenceCreateInfo fenceCreateInfo = vkinit::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);

    for (UI32 i = 0; i < _swapChain.imageCount(); i++) {
        if (vkCreateSemaphore(_context.device, &semaphoreCreateInfo, nullptr, &_imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(_context.device, &semaphoreCreateInfo, nullptr, &_renderFinishedSemaphores[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create semaphores!");
        }
        if (vkCreateFence(_context.device, &fenceCreateInfo, nullptr, &_inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create fences!");
        }
    }
}

void Renderer::createFramebuffers() {
    _framebuffers.resize(_swapChain.imageCount());
    _guiFramebuffers.resize(_swapChain.imageCount());

    // create array to store views of all attachments used in render pass
    VkImageView attachmentViews[kAttachments::ATTACHMENTS_MAX_ENUM] {};

    attachmentViews[GBUFFER_POSITION_ATTACHMENT] = _gbuffer[GBUFFER_POSITION]._view;
    attachmentViews[GBUFFER_NORMAL_ATTACHMENT] = _gbuffer[GBUFFER_NORMAL]._view;
    attachmentViews[GBUFFER_ALBEDO_ATTACHMENT] = _gbuffer[GBUFFER_ALBEDO]._view;
    attachmentViews[GBUFFER_AO_METALLIC_ROUGHNESS_ATTACHMENT] = _gbuffer[GBUFFER_AO_METALLIC_ROUGHNESS]._view;
    attachmentViews[GBUFFER_DEPTH_ATTACHMENT] = _gbuffer[GBUFFER_DEPTH]._view;

    VkFramebufferCreateInfo framebufferCreateInfo; 

    for (UI32 i = 0; i < _swapChain.imageCount(); i++) {
        // gui framebuffer
        framebufferCreateInfo = vkinit::framebufferCreateInfo(_guiRenderPass, 
            1, &_swapChain._imageViews[i], _swapChain.extent(), 1);

        if (vkCreateFramebuffer(_context.device, &framebufferCreateInfo, nullptr, &_guiFramebuffers[i]) != 
            VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }

        // get color attachment from swap chain
        attachmentViews[COLOR_ATTACHMENT] = _swapChain._imageViews[i];

        // final render framebuffers
        framebufferCreateInfo = vkinit::framebufferCreateInfo(_renderPass, ATTACHMENTS_MAX_ENUM,
            attachmentViews, _swapChain.extent(), 1);

        if (vkCreateFramebuffer(_context.device, &framebufferCreateInfo, nullptr, &_framebuffers[i]) != 
            VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

void Renderer::createAttachment(Attachment& attachment, VkImageUsageFlags usage, VkExtent2D extent, VkFormat format) {
    // create the image 
    {
        VkImageCreateInfo imageCreateInfo = vkinit::imageCreateInfo(format, { extent.width, extent.height, 1 }, 1, 1,
            VK_IMAGE_TILING_OPTIMAL, usage);
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;

        if (vkCreateImage(_context.device, &imageCreateInfo, nullptr, &attachment._image) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image!");
        }

        // get memory requirements for this particular image
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(_context.device, attachment._image, &memRequirements);

        VkMemoryAllocateInfo allocInfo = vkinit::memoryAllocateInfo(memRequirements.size,
            utils::findMemoryType(_context.physicalDevice, memRequirements.memoryTypeBits,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));

        // allocate memory on device
        if (vkAllocateMemory(_context.device, &allocInfo, nullptr, &attachment._memory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate image memory!");
        }

        // bind image to memory
        vkBindImageMemory(_context.device, attachment._image, attachment._memory, 0);
    }

    // create the image view
    {
        VkImageAspectFlags aspectMask = 0;
        // usage determines aspect mask
        if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
            aspectMask |= VK_IMAGE_ASPECT_COLOR_BIT;
        if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
            aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;

        if (aspectMask <= 0)
            throw std::runtime_error("Invalid aspect mask!");

        VkImageViewCreateInfo imageViewCreateInfo = vkinit::imageViewCreateInfo(attachment._image,
            VK_IMAGE_VIEW_TYPE_2D, format, {}, { aspectMask, 0, 1, 0, 1 });
        attachment._view = Image::createImageView(&_context, imageViewCreateInfo);
    }

    attachment._format = format;
}

void Renderer::createColorSampler() {
    VkSamplerCreateInfo samplerCreateInfo = 
        vkinit::samplerCreateInfo(_context.deviceProperties.limits.maxSamplerAnisotropy);
    samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCreateInfo.addressModeV = samplerCreateInfo.addressModeU;
    samplerCreateInfo.addressModeW = samplerCreateInfo.addressModeU;
    if (vkCreateSampler(_context.device, &samplerCreateInfo, nullptr, &_colorSampler)) {
        throw std::runtime_error("Could not create GBuffer colour sampler");
    }
}

void Renderer::createCommandBuffers() {
    _renderCommandBuffers.resize(_swapChain.imageCount());
    _guiCommandBuffers.resize(_swapChain.imageCount());
    VkCommandBufferAllocateInfo allocInfo = vkinit::commandBufferAllocateInfo(_commandPools[RENDER_CMD_POOL], 
        VK_COMMAND_BUFFER_LEVEL_PRIMARY, _swapChain.imageCount());
    if (vkAllocateCommandBuffers(_context.device, &allocInfo, _renderCommandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
    if (vkAllocateCommandBuffers(_context.device, &allocInfo, _guiCommandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

void Renderer::createDescriptorPool() {
    UI32 poolNum = 100;
    // TODO: MAKE DESCRIPTOR POOL SIZE MORE APPROPRIATE (in relation to scene perhaps?)
    std::vector<VkDescriptorPoolSize> poolSizes = {
        { VK_DESCRIPTOR_TYPE_SAMPLER,                poolNum },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, poolNum },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          poolNum },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          poolNum },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,   poolNum },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,   poolNum },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         poolNum },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         poolNum },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, poolNum },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, poolNum },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,       poolNum }
    };

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = vkinit::descriptorPoolCreateInfo(
        poolNum * _swapChain.imageCount(), static_cast<UI32>(poolSizes.size()), poolSizes.data(),
        VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);

    if (vkCreateDescriptorPool(_context.device, &descriptorPoolCreateInfo, nullptr, &_descriptorPool) != 
        VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

void Renderer::createDescriptorSetLayouts() {
    // hard coded descriptor set layouts for specific material types
    // subsequent derived materials will reuse the layout but with different descriptor sets

    // OFFSCREEN:

    // 0: default material (no textures, plain color)
    std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings = {
        // binding 0: vertex shader uniform buffer 
        vkinit::descriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT),
    };

    VkDescriptorSetLayoutCreateInfo descriptorSetlayoutCreateInfo = vkinit::descriptorSetLayoutCreateInfo(
        descriptorSetLayoutBindings.data(), static_cast<UI32>(descriptorSetLayoutBindings.size()));

    if (vkCreateDescriptorSetLayout(_context.device, &descriptorSetlayoutCreateInfo, nullptr, 
        &_descriptorSetLayouts[OFFSCREEN_DEFAULT_DESCRIPTOR_LAYOUT]) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }

    // 1: PBR material
    descriptorSetLayoutBindings = {
        // binding 0: vertex shader uniform buffer 
        vkinit::descriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT),
        // binding 1: fragment shader albedo texture
        vkinit::descriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
        // binding 2: fragment shader occlusion metallic roughness texture
        vkinit::descriptorSetLayoutBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) 
    };

    descriptorSetlayoutCreateInfo = vkinit::descriptorSetLayoutCreateInfo(
        descriptorSetLayoutBindings.data(), static_cast<UI32>(descriptorSetLayoutBindings.size()));

    if (vkCreateDescriptorSetLayout(_context.device, &descriptorSetlayoutCreateInfo, nullptr,
        &_descriptorSetLayouts[OFFSCREEN_PBR_DESCRIPTOR_LAYOUT]) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }

    // 2: PBR material with normal map
    descriptorSetLayoutBindings = {
        // binding 0: vertex shader uniform buffer 
        vkinit::descriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT),
        // binding 1: fragment shader albedo texture
        vkinit::descriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
        // binding 2: fragment shader occlusion metallic roughness texture
        vkinit::descriptorSetLayoutBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
        // binding 3: fragment shader normal map
        vkinit::descriptorSetLayoutBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
    };

    descriptorSetlayoutCreateInfo = vkinit::descriptorSetLayoutCreateInfo(
        descriptorSetLayoutBindings.data(), static_cast<UI32>(descriptorSetLayoutBindings.size()));

    if (vkCreateDescriptorSetLayout(_context.device, &descriptorSetlayoutCreateInfo, nullptr,
        &_descriptorSetLayouts[OFFSCREEN_PBR_NORMAL_DESCRIPTOR_LAYOUT]) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }

    // 3: skybox
    descriptorSetLayoutBindings = {
        // binding 0: vertex shader uniform buffer 
        vkinit::descriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT),
        // binding 1: fragment shader texture sampler
        vkinit::descriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
    };

    descriptorSetlayoutCreateInfo = vkinit::descriptorSetLayoutCreateInfo(
        descriptorSetLayoutBindings.data(), static_cast<UI32>(descriptorSetLayoutBindings.size()));

    if (vkCreateDescriptorSetLayout(_context.device, &descriptorSetlayoutCreateInfo, nullptr,
        &_descriptorSetLayouts[OFFSCREEN_SKYBOX_DESCRIPTOR_LAYOUT]) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }

    // 4: shadowmap
    descriptorSetLayoutBindings = {
        // binding 0: vertex shader uniform buffer 
        vkinit::descriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
    };

    descriptorSetlayoutCreateInfo = vkinit::descriptorSetLayoutCreateInfo(
        descriptorSetLayoutBindings.data(), static_cast<UI32>(descriptorSetLayoutBindings.size()));

    if (vkCreateDescriptorSetLayout(_context.device, &descriptorSetlayoutCreateInfo, nullptr,
        &_descriptorSetLayouts[OFFSCREEN_SHADOWMAP_DESCRIPTOR_LAYOUT]) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }

    // COMPOSITION:

    descriptorSetLayoutBindings = {
        // binding 0: composition fragment shader uniform buffer 
        vkinit::descriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT),
        // binding 1: position input attachment
        vkinit::descriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT),
        // binding 2: normal input attachment
        vkinit::descriptorSetLayoutBinding(2, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT),
        // binding 3: albedo input attachment
        vkinit::descriptorSetLayoutBinding(3, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT),
        // binding 4: metallic roughness input attachment
        vkinit::descriptorSetLayoutBinding(4, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT)
    };

    descriptorSetlayoutCreateInfo = vkinit::descriptorSetLayoutCreateInfo(
        descriptorSetLayoutBindings.data(), static_cast<UI32>(descriptorSetLayoutBindings.size()));

    if (vkCreateDescriptorSetLayout(_context.device, &descriptorSetlayoutCreateInfo, nullptr,
        &_descriptorSetLayouts[COMPOSITION_DESCRIPTOR_LAYOUT]) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

void Renderer::createCompositionPipeline() {
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = 
        vkinit::pipelineLayoutCreateInfo(1, &_descriptorSetLayouts[COMPOSITION_DESCRIPTOR_LAYOUT]);

    if (vkCreatePipelineLayout(_context.device, &pipelineLayoutCreateInfo, nullptr, &_compositionPipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Could not create composition pipeline layout!");
    }

    VkColorComponentFlags colBlendAttachFlag =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    // only one colour attachment 
    VkPipelineColorBlendAttachmentState colorBlendAttachment =
        vkinit::pipelineColorBlendAttachmentState(colBlendAttachFlag, VK_FALSE);

    VkShaderModule vertShaderModule, fragShaderModule;
    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateInfo =
        vkinit::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE);

    VkPipelineRasterizationStateCreateInfo rasterizerStateInfo =
        vkinit::pipelineRasterStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);

    VkPipelineColorBlendStateCreateInfo    colorBlendingStateInfo =
        vkinit::pipelineColorBlendStateCreateInfo(1, &colorBlendAttachment);

    // disable depth testing
    VkPipelineDepthStencilStateCreateInfo  depthStencilStateInfo =
        vkinit::pipelineDepthStencilStateCreateInfo(VK_FALSE, VK_FALSE, VK_COMPARE_OP_NEVER);

    VkPipelineViewportStateCreateInfo      viewportStateInfo =
        vkinit::pipelineViewportStateCreateInfo(1, nullptr, 1, nullptr);

    VkPipelineMultisampleStateCreateInfo   multisamplingStateInfo =
        vkinit::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);

    // dynamic view port for resizing
    VkDynamicState dynamicStateEnables[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = vkinit::pipelineDynamicStateCreateInfo(dynamicStateEnables, 2);
    VkPipelineVertexInputStateCreateInfo emptyVertexInputStateCreateInfo =
        vkinit::pipelineVertexInputStateCreateInfo(0, nullptr, 0, nullptr); // no vertex data input

    VkGraphicsPipelineCreateInfo pipelineCreateInfo =
        vkinit::graphicsPipelineCreateInfo(_compositionPipelineLayout, _renderPass, COMPOSITION_SUBPASS); // composition pipeline uses swapchain render pass
    pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineCreateInfo.pStages = shaderStages.data();
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateInfo;
    pipelineCreateInfo.pViewportState = &viewportStateInfo;
    pipelineCreateInfo.pRasterizationState = &rasterizerStateInfo;
    pipelineCreateInfo.pMultisampleState = &multisamplingStateInfo;
    pipelineCreateInfo.pDepthStencilState = &depthStencilStateInfo;
    pipelineCreateInfo.pColorBlendState = &colorBlendingStateInfo;
    pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
    pipelineCreateInfo.pVertexInputState = &emptyVertexInputStateCreateInfo;

#ifndef NDEBUG
    vertShaderModule = Shader::createShaderModule(&_context, Shader::readFile("composition_debug.vert.spv"));
    fragShaderModule = Shader::createShaderModule(&_context, Shader::readFile("composition_debug.frag.spv"));
#else
    vertShaderModule = Shader::createShaderModule(&_context, Shader::readFile("composition.vert.spv"));
    fragShaderModule = Shader::createShaderModule(&_context, Shader::readFile("composition.frag.spv"));
#endif

    shaderStages[0] = vkinit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule, "main");
    shaderStages[1] = vkinit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule, "main");

    if (vkCreateGraphicsPipelines(_context.device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &_compositionPipeline) != VK_SUCCESS) {
        throw std::runtime_error("Could not create deferred graphics pipeline!");
    }

    vkDestroyShaderModule(_context.device, vertShaderModule, nullptr);
    vkDestroyShaderModule(_context.device, fragShaderModule, nullptr);
}

void Renderer::createGuiRenderPass() {
    VkAttachmentDescription attachment = {};
    attachment.format = _swapChain.format();
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // the initial layout is the image of scene geometry
    attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment = {};
    color_attachment.attachment = 0;
    color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    info.attachmentCount = 1;
    info.pAttachments = &attachment;
    info.subpassCount = 1;
    info.pSubpasses = &subpass;
    info.dependencyCount = 1;
    info.pDependencies = &dependency;

    if (vkCreateRenderPass(_context.device, &info, nullptr, &_guiRenderPass) != VK_SUCCESS) {
        throw std::runtime_error("Could not create Dear ImGui's render pass");
    }
}

void Renderer::createCompositionDescriptorSets() {
    // we want one descriptor set per swap chain image
    std::vector<VkDescriptorSetLayout> _compositionDescriptorSetLayouts(_swapChain.imageCount(), 
        _descriptorSetLayouts[COMPOSITION_DESCRIPTOR_LAYOUT]);
    _compositionDescriptorSets.resize(_compositionDescriptorSetLayouts.size());

    VkDescriptorSetAllocateInfo allocInfo = vkinit::descriptorSetAllocInfo(_descriptorPool, 
        static_cast<UI32>(_compositionDescriptorSetLayouts.size()), _compositionDescriptorSetLayouts.data());

    if (vkAllocateDescriptorSets(_context.device, &allocInfo, _compositionDescriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    // image descriptors for gBuffer color attachments and shadow map
    VkDescriptorImageInfo texDescriptorPosition{};
    texDescriptorPosition.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    texDescriptorPosition.imageView = _gbuffer[GBUFFER_POSITION]._view;
    texDescriptorPosition.sampler = _colorSampler;

    VkDescriptorImageInfo texDescriptorNormal{};
    texDescriptorNormal.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    texDescriptorNormal.imageView = _gbuffer[GBUFFER_NORMAL]._view;
    texDescriptorNormal.sampler = _colorSampler;

    VkDescriptorImageInfo texDescriptorAlbedo{};
    texDescriptorAlbedo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    texDescriptorAlbedo.imageView = _gbuffer[GBUFFER_ALBEDO]._view;
    texDescriptorAlbedo.sampler = _colorSampler;

    VkDescriptorImageInfo texDescriptorMetallicRoughness{};
    texDescriptorMetallicRoughness.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    texDescriptorMetallicRoughness.imageView = _gbuffer[GBUFFER_AO_METALLIC_ROUGHNESS]._view;
    texDescriptorMetallicRoughness.sampler = _colorSampler;

    // TODO: ADD SHADOW MAPS BACK IN
    /*
    VkDescriptorImageInfo texDescriptorShadowMap{};
    texDescriptorShadowMap.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    texDescriptorShadowMap.imageView = shadowMap.imageView;
    texDescriptorShadowMap.sampler = shadowMap.depthSampler;
    */

    std::vector<VkWriteDescriptorSet> writeDescriptorSets{};

    for (UI32 i = 0; i < _swapChain.imageCount(); i++) {
        // forward rendering uniform buffer
        VkDescriptorBufferInfo compositionUboInf{};
        compositionUboInf.buffer = _compositionUniforms._vkBuffer;
        compositionUboInf.offset = sizeof(CompositionUBO) * i;
        compositionUboInf.range = sizeof(CompositionUBO);

        // composition descriptor writes
        writeDescriptorSets = {
            // binding 1: shadow map
            //vkinit::writeDescriptorSet(_compositionDescriptorSets[i], 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &texDescriptorShadowMap),
            // binding 0: composition fragment shader uniform
            vkinit::writeDescriptorSet(_compositionDescriptorSets[i], 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &compositionUboInf),
            // binding 1: position input attachment 
            vkinit::writeDescriptorSet(_compositionDescriptorSets[i], 1, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, &texDescriptorPosition),
            // binding 2: normal input attachment
            vkinit::writeDescriptorSet(_compositionDescriptorSets[i], 2, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, &texDescriptorNormal),
            // binding 3: albedo input attachment
            vkinit::writeDescriptorSet(_compositionDescriptorSets[i], 3, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, &texDescriptorAlbedo),
            // binding 4: metallic roughness input attachment
            vkinit::writeDescriptorSet(_compositionDescriptorSets[i], 4, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, &texDescriptorMetallicRoughness)
        };

        // update according to the configuration
        vkUpdateDescriptorSets(_context.device, static_cast<UI32>(writeDescriptorSets.size()), 
            writeDescriptorSets.data(), 0, nullptr);
    }
}

void Renderer::createRenderPass() {

    // attachment descriptions

    std::array<VkAttachmentDescription, ATTACHMENTS_MAX_ENUM> attachmentDescriptions = {};

    VkAttachmentDescription attachmentDescription{};
    attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    // offscreen: 3 color attachments, 1 depth attachment

    attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachmentDescription.format = _gbuffer[GBUFFER_POSITION]._format;
    attachmentDescriptions[GBUFFER_POSITION_ATTACHMENT] = attachmentDescription;

    attachmentDescription.format = _gbuffer[GBUFFER_NORMAL]._format;
    attachmentDescriptions[GBUFFER_NORMAL_ATTACHMENT] = attachmentDescription;

    attachmentDescription.format = _gbuffer[GBUFFER_ALBEDO]._format;
    attachmentDescriptions[GBUFFER_ALBEDO_ATTACHMENT] = attachmentDescription;

    attachmentDescription.format = _gbuffer[GBUFFER_AO_METALLIC_ROUGHNESS]._format;
    attachmentDescriptions[GBUFFER_AO_METALLIC_ROUGHNESS_ATTACHMENT] = attachmentDescription;

    attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachmentDescription.format = _gbuffer[GBUFFER_DEPTH]._format;
    attachmentDescriptions[GBUFFER_DEPTH_ATTACHMENT] = attachmentDescription;

    // composition: 1 color attachment

    attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachmentDescription.format = _swapChain.format();
    attachmentDescriptions[COLOR_ATTACHMENT] = attachmentDescription;

    // subpasses

    std::array<VkSubpassDescription, kSubpass::SUBPASS_MAX_ENUM> subpasses {};

    // 1: offscreen subpass

    // offscreen subpass attachment references
    std::vector<VkAttachmentReference> offScreenColorReferences = {
        { GBUFFER_POSITION_ATTACHMENT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
        { GBUFFER_NORMAL_ATTACHMENT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
        { GBUFFER_ALBEDO_ATTACHMENT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
        { GBUFFER_AO_METALLIC_ROUGHNESS_ATTACHMENT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL } };

    VkAttachmentReference offScreenDepthReference = 
        { GBUFFER_DEPTH_ATTACHMENT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };


    subpasses[OFFSCREEN_SUBPASS].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpasses[OFFSCREEN_SUBPASS].pColorAttachments = offScreenColorReferences.data();
    subpasses[OFFSCREEN_SUBPASS].colorAttachmentCount = static_cast<UI32>(offScreenColorReferences.size());
    subpasses[OFFSCREEN_SUBPASS].pDepthStencilAttachment = &offScreenDepthReference;

    // 2: composition subpass

    // composition subpass attachment references
    std::vector<VkAttachmentReference> offScreenInputReferences = {
        { GBUFFER_POSITION_ATTACHMENT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
        { GBUFFER_NORMAL_ATTACHMENT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
        { GBUFFER_ALBEDO_ATTACHMENT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
        { GBUFFER_AO_METALLIC_ROUGHNESS_ATTACHMENT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } };

    VkAttachmentReference compositionColorReference = 
        { COLOR_ATTACHMENT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

    subpasses[COMPOSITION_SUBPASS].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpasses[COMPOSITION_SUBPASS].pColorAttachments = &compositionColorReference;
    subpasses[COMPOSITION_SUBPASS].colorAttachmentCount = 1;
    subpasses[COMPOSITION_SUBPASS].pInputAttachments = offScreenInputReferences.data();
    subpasses[COMPOSITION_SUBPASS].inputAttachmentCount = static_cast<UI32>(offScreenInputReferences.size());

    // subpass dependencies

    std::array<VkSubpassDependency, 3> dependencies{};

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = OFFSCREEN_SUBPASS;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    // transition from color attachment to fragment shader read

    dependencies[1].srcSubpass = OFFSCREEN_SUBPASS;
    dependencies[1].dstSubpass = COMPOSITION_SUBPASS;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[2].srcSubpass = COMPOSITION_SUBPASS;
    dependencies[2].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[2].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[2].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo renderPassInfo = vkinit::renderPassCreateInfo();

    renderPassInfo.pAttachments = attachmentDescriptions.data();
    renderPassInfo.attachmentCount = static_cast<UI32>(attachmentDescriptions.size());

    renderPassInfo.pSubpasses = subpasses.data();
    renderPassInfo.subpassCount = static_cast<UI32>(subpasses.size());
    
    renderPassInfo.pDependencies = dependencies.data();
    renderPassInfo.dependencyCount = static_cast<UI32>(dependencies.size());

    if (vkCreateRenderPass(_context.device, &renderPassInfo, nullptr, &_renderPass) != VK_SUCCESS) {
        throw std::runtime_error("Could not create GBuffer's render pass");
    }
}