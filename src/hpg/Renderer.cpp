#include <hpg/Renderer.h>

#include <utils/vkinit.h>

void Renderer::init(GLFWwindow* window) {
	_context.init(window);

    createCommandPool(&_commandPools[RENDER], VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    createCommandPool(&_commandPools[GUI], VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    _swapChain.init(&_context);

    // gbuffer attachments
    createAttachment(_gbuffer[POSITION], 0x94, _swapChain.extent(), VK_FORMAT_R16G16B16A16_SFLOAT);
    createAttachment(_gbuffer[NORMAL], 0x94, _swapChain.extent(), VK_FORMAT_R16G16B16A16_SFLOAT);
    createAttachment(_gbuffer[ALBEDO], 0x94, _swapChain.extent(), VK_FORMAT_R8G8B8A8_SRGB);
    createAttachment(_gbuffer[DEPTH], 0xa4, _swapChain.extent(), utils::findDepthFormat(_context.physicalDevice));

    // build render pass
    createRenderPass();
    createGuiRenderPass();

    createFramebuffers();

    createCommandBuffers();

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
        attachment.cleanup(&_context);
    }

    vkDestroySampler(_context.device, _colorSampler, nullptr);

    // destroy the render passes
    vkDestroyRenderPass(_context.device, _guiRenderPass, nullptr);
    vkDestroyRenderPass(_context.device, _renderPass, nullptr);

    _swapChain.cleanup();

    vkDestroyCommandPool(_context.device, _commandPools[RENDER], nullptr);
    vkDestroyCommandPool(_context.device, _commandPools[GUI], nullptr);

    _context.cleanup();
}

void Renderer::recreateSwapchain() {

    vkFreeCommandBuffers(_context.device, _commandPools[RENDER], 
        static_cast<UI32>(_renderCommandBuffers.size()), _renderCommandBuffers.data());
    vkFreeCommandBuffers(_context.device, _commandPools[RENDER],
        static_cast<UI32>(_guiCommandBuffers.size()), _guiCommandBuffers.data());

    // destroy previous swapchain and dependencies 
    for (UI32 i = 0; i < _swapChain.imageCount(); i++) {
        vkDestroyFramebuffer(_context.device, _framebuffers[i], nullptr);
        vkDestroyFramebuffer(_context.device, _guiFramebuffers[i], nullptr);
    }

    vkDestroyRenderPass(_context.device, _guiRenderPass, nullptr);
    vkDestroyRenderPass(_context.device, _renderPass, nullptr);

    for (auto& attachment : _gbuffer) {
        attachment.cleanup(&_context);
    }
    
    _swapChain.cleanup();

    // recreate swapchain and dependencies

    _swapChain.init(&_context);

    createAttachment(_gbuffer[POSITION], 0x94, _swapChain.extent(), VK_FORMAT_R16G16B16A16_SFLOAT);
    createAttachment(_gbuffer[NORMAL], 0x94, _swapChain.extent(), VK_FORMAT_R16G16B16A16_SFLOAT);
    createAttachment(_gbuffer[ALBEDO], 0x94, _swapChain.extent(), VK_FORMAT_R8G8B8A8_SRGB);
    createAttachment(_gbuffer[DEPTH], 0xa4, _swapChain.extent(), utils::findDepthFormat(_context.physicalDevice));

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

    VkFramebufferCreateInfo framebufferCreateInfo; 

    // TODO: once render passes have been merged, remove extra framebuffer
    std::array<VkImageView, kGbuffer::NUM_GBUFFER_ATTACHMENTS> gbufferViews{};
    for (UI32 i = 0; i < kGbuffer::NUM_GBUFFER_ATTACHMENTS; i++) {
        gbufferViews[i] = _gbuffer[i]._view;
    }

    VkImageView attachmentViews[kAttachments::NUM_ATTACHMENTS] {};

    attachmentViews[kAttachments::GBUFFER_POSITION] = _gbuffer[kGbuffer::POSITION]._view;
    attachmentViews[kAttachments::GBUFFER_NORMAL]   = _gbuffer[kGbuffer::NORMAL]._view;
    attachmentViews[kAttachments::GBUFFER_ALBEDO]   = _gbuffer[kGbuffer::ALBEDO]._view;
    attachmentViews[kAttachments::GBUFFER_DEPTH]    = _gbuffer[kGbuffer::DEPTH]._view;

    for (UI32 i = 0; i < _swapChain.imageCount(); i++) {
        // gui framebuffer
        framebufferCreateInfo = vkinit::framebufferCreateInfo(_guiRenderPass, 
            1, &_swapChain._imageViews[i], _swapChain.extent(), 1);

        if (vkCreateFramebuffer(_context.device, &framebufferCreateInfo, nullptr, &_guiFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }

        // get color attachment from swap chain
        attachmentViews[kAttachments::COLOR] = _swapChain._imageViews[i];

        // final render framebuffers
        framebufferCreateInfo = vkinit::framebufferCreateInfo(_renderPass, kAttachments::NUM_ATTACHMENTS,
            attachmentViews, _swapChain.extent(), 1);

        if (vkCreateFramebuffer(_context.device, &framebufferCreateInfo, nullptr, &_framebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

void Renderer::createAttachment(Renderer::Attachment& attachment, VkImageUsageFlags usage, VkExtent2D extent, VkFormat format) {
    // create the image 
    Image::ImageCreateInfo info{};
    info.extent = extent;
    info.format = format;
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.usage = usage;
    info.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    info.pImage = &attachment._image;

    Image::createImage(&_context, _commandPools[RENDER], info);

    // create the image view
    VkImageAspectFlags aspectMask = 0;
    // usage determines aspect mask
    if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
        aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
        aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    if (aspectMask <= 0)
        throw std::runtime_error("Invalid aspect mask!");

    VkImageViewCreateInfo imageViewCreateInfo = vkinit::imageViewCreateInfo(attachment._image._vkImage,
        VK_IMAGE_VIEW_TYPE_2D, format, {}, { aspectMask, 0, 1, 0, 1 });
    attachment._view = Image::createImageView(&_context, imageViewCreateInfo);
}

void Renderer::createColorSampler() {
    VkSamplerCreateInfo samplerCreateInfo = vkinit::samplerCreateInfo();
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
    VkCommandBufferAllocateInfo allocInfo = vkinit::commandBufferAllocateInfo(_commandPools[RENDER], 
        VK_COMMAND_BUFFER_LEVEL_PRIMARY, _swapChain.imageCount());
    if (vkAllocateCommandBuffers(_context.device, &allocInfo, _renderCommandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
    if (vkAllocateCommandBuffers(_context.device, &allocInfo, _guiCommandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
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

void Renderer::createRenderPass() {

    // attachment descriptions

    std::array<VkAttachmentDescription, NUM_ATTACHMENTS> attachmentDescriptions = {};

    VkAttachmentDescription attachmentDescription{};
    attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    // offscreen: 3 color attachments, 1 depth attachment

    attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachmentDescription.format = _gbuffer[kGbuffer::POSITION]._image._format;
    attachmentDescriptions[kAttachments::GBUFFER_POSITION] = attachmentDescription;

    attachmentDescription.format = _gbuffer[kGbuffer::NORMAL]._image._format;
    attachmentDescriptions[kAttachments::GBUFFER_NORMAL] = attachmentDescription;

    attachmentDescription.format = _gbuffer[kGbuffer::ALBEDO]._image._format;
    attachmentDescriptions[kAttachments::GBUFFER_ALBEDO] = attachmentDescription;

    attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachmentDescription.format = _gbuffer[kGbuffer::DEPTH]._image._format;
    attachmentDescriptions[kAttachments::GBUFFER_DEPTH] = attachmentDescription;

    // composition: 1 color attachment

    attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachmentDescription.format = _swapChain.format();
    attachmentDescriptions[kAttachments::COLOR] = attachmentDescription;

    // subpasses

    std::array<VkSubpassDescription, kSubpasses::NUM_SUBPASSES> subpasses {};

    // 1: offscreen subpass

    // offscreen subpass attachment references
    std::vector<VkAttachmentReference> offScreenColorReferences = {
        { kAttachments::GBUFFER_POSITION, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
        { kAttachments::GBUFFER_NORMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
        { kAttachments::GBUFFER_ALBEDO, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL } };

    VkAttachmentReference offScreenDepthReference = 
        { kAttachments::GBUFFER_DEPTH, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };


    subpasses[kSubpasses::OFFSCREEN].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpasses[kSubpasses::OFFSCREEN].pColorAttachments = offScreenColorReferences.data();
    subpasses[kSubpasses::OFFSCREEN].colorAttachmentCount = static_cast<UI32>(offScreenColorReferences.size());
    subpasses[kSubpasses::OFFSCREEN].pDepthStencilAttachment = &offScreenDepthReference;

    // 2: composition subpass

    // composition subpass attachment references
    std::vector<VkAttachmentReference> offScreenInputReferences = {
        { kAttachments::GBUFFER_POSITION, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
        { kAttachments::GBUFFER_NORMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
        { kAttachments::GBUFFER_ALBEDO, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } };

    VkAttachmentReference compositionColorReference = 
        { kAttachments::COLOR, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

    subpasses[kSubpasses::COMPOSITION].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpasses[kSubpasses::COMPOSITION].pColorAttachments = &compositionColorReference;
    subpasses[kSubpasses::COMPOSITION].colorAttachmentCount = 1;
    subpasses[kSubpasses::COMPOSITION].pInputAttachments = offScreenInputReferences.data();
    subpasses[kSubpasses::COMPOSITION].inputAttachmentCount = static_cast<UI32>(offScreenInputReferences.size());

    // subpass dependencies

    std::array<VkSubpassDependency, 3> dependencies{};

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = kSubpasses::OFFSCREEN;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    // transition from color attachment to fragment shader read

    dependencies[1].srcSubpass = kSubpasses::OFFSCREEN;
    dependencies[1].dstSubpass = kSubpasses::COMPOSITION;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[2].srcSubpass = kSubpasses::COMPOSITION;
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