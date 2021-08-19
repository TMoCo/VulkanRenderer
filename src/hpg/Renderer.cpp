#include <hpg/Renderer.h>

#include <utils/vkinit.h>

void Renderer::init(GLFWwindow* window) {
	// setup vulkan context
	_context.init(window);

    createCommandPool(&_commandPools[RENDER], VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    createCommandPool(&_commandPools[GUI], VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    _swapChain.init(&_context);

    // gbuffer attachments
    createAttachment(_gbuffer[POSITION], 0x14, _swapChain.extent(), VK_FORMAT_R16G16B16A16_SFLOAT);
    createAttachment(_gbuffer[NORMAL], 0x14, _swapChain.extent(), VK_FORMAT_R16G16B16A16_SFLOAT);
    createAttachment(_gbuffer[ALBEDO], 0x14, _swapChain.extent(), VK_FORMAT_R8G8B8A8_SRGB);
    createAttachment(_gbuffer[DEPTH], 0x24, _swapChain.extent(), utils::findDepthFormat(_context.physicalDevice));

    createFwdRenderPass();
    createGuiRenderPass();
    createOffscreenRenderPass();

    createFramebuffers();

    createSyncObjects();

    // build render pass
}

void Renderer::cleanup() {
    for (UI32 i = 0; i < _swapChain.imageCount(); i++) {
        vkDestroySemaphore(_context.device, _renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(_context.device, _imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(_context.device, _offScreenSemaphores[i], nullptr);
        vkDestroyFence(_context.device, _inFlightFences[i], nullptr);
    }

    for (size_t i = 0; i < _swapChain.imageCount(); i++) {
        vkDestroyFramebuffer(_context.device, _framebuffers[i], nullptr);
        vkDestroyFramebuffer(_context.device, _guiFramebuffers[i], nullptr);
    }

    for (auto& attachment : _gbuffer) {
        attachment.cleanup(&_context);
    }

    // destroy the render passes
    vkDestroyRenderPass(_context.device, _renderPass, nullptr);
    vkDestroyRenderPass(_context.device, _guiRenderPass, nullptr);

    _swapChain.cleanup();

    vkDestroyCommandPool(_context.device, _commandPools[RENDER], nullptr);
    vkDestroyCommandPool(_context.device, _commandPools[GUI], nullptr);

    _context.cleanup();
}

void Renderer::recreateSwapchain() {
    // destroy previous swapchain and dependencies 
    for (size_t i = 0; i < _swapChain.imageCount(); i++) {
        vkDestroyFramebuffer(_context.device, _framebuffers[i], nullptr);
        vkDestroyFramebuffer(_context.device, _guiFramebuffers[i], nullptr);
    }

    for (auto& attachment : _gbuffer) {
        attachment.cleanup(&_context);
    }
    
    _swapChain.cleanup();

    // recreate swapchain and dependencies

    _swapChain.init(&_context);

    createAttachment(_gbuffer[POSITION], 0x14, _swapChain.extent(), VK_FORMAT_R16G16B16A16_SFLOAT);
    createAttachment(_gbuffer[NORMAL], 0x14, _swapChain.extent(), VK_FORMAT_R16G16B16A16_SFLOAT);
    createAttachment(_gbuffer[ALBEDO], 0x14, _swapChain.extent(), VK_FORMAT_R8G8B8A8_SRGB);
    createAttachment(_gbuffer[DEPTH], 0x24, _swapChain.extent(), utils::findDepthFormat(_context.physicalDevice));

    createFramebuffers();
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
    _offScreenSemaphores.resize(_swapChain.imageCount());

    _inFlightFences.resize(_swapChain.imageCount());
    _imagesInFlight.resize(_swapChain.imageCount(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphoreCreateInfo();

    VkFenceCreateInfo fenceCreateInfo = vkinit::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);

    for (UI32 i = 0; i < _swapChain.imageCount(); i++) {
        if (vkCreateSemaphore(_context.device, &semaphoreCreateInfo, nullptr, &_imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(_context.device, &semaphoreCreateInfo, nullptr, &_renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(_context.device, &semaphoreCreateInfo, nullptr, &_offScreenSemaphores[i]) != VK_SUCCESS) {
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

    for (UI32 i = 0; i < _swapChain.imageCount(); i++) {
        // image framebuffer
        framebufferCreateInfo = vkinit::framebufferCreateInfo(_renderPass,
            1, &_swapChain._imageViews[i], _swapChain.extent(), 1); // only need colour attachment
      
        if (vkCreateFramebuffer(_context.device, &framebufferCreateInfo, nullptr, &_framebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }

        // gui framebuffer
        framebufferCreateInfo = vkinit::framebufferCreateInfo(_guiRenderPass, 
            1, &_swapChain._imageViews[i], _swapChain.extent(), 1);

        if (vkCreateFramebuffer(_context.device, &framebufferCreateInfo, nullptr, &_guiFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }

    // TODO: once render passes have been merged, remove extra framebuffer
    std::array<VkImageView, NUM_ATTACHMENTS> gbufferViews{};
    for (UI32 i = 0; i < NUM_ATTACHMENTS; i++) {
        gbufferViews[i] = _gbuffer[i]._view;
    }

    framebufferCreateInfo = vkinit::framebufferCreateInfo(_offscreenRenderPass, 
        NUM_ATTACHMENTS, gbufferViews.data(), _swapChain.extent(), 1);

    if (vkCreateFramebuffer(_context.device, &framebufferCreateInfo, nullptr, &_offscreenFramebuffer) != VK_SUCCESS) {
        throw std::runtime_error("Could not create GBuffer's frame buffer");
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

void Renderer::createFwdRenderPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = _swapChain.format(); // sc image format
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // specify a depth attachment to the render pass
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = utils::findDepthFormat(_context.physicalDevice); // same format as depth image
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    /**************************************************************************************************************
    // A single render pass consists of multiple subpasses, which are subsequent rendering operations depending on
    // content of framebuffers on previous passes (eg post processing). Grouping subpasses into a single render
    // pass lets Vulkan optimise every subpass references 1 or more attachments.
    **************************************************************************************************************/

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // which layout during a subpass

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // the subpass
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // explicit a graphics subpass 
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = nullptr; // &depthAttachmentRef;

    /**************************************************************************************************************
    // subpass dependencies control the image layout transitions. They specify memory and execution of dependencies
    // between subpasses there are implicit subpasses right before and after the render pass
    // There are two built-in dependencies that take care of the transition at the start of the render pass and at
    // the end, but the former does not occur at the right time as it assumes that the transition occurs at the
    // start of the pipeline, but we haven't acquired the image yet there are two ways to deal with the problem:
    // - change waitStages of the imageAvailableSemaphore (in drawframe) to VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
    // this ensures that the render pass does not start until image is available.
    // - make the render pass wait for the VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT stage.
    **************************************************************************************************************/

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    /*
    std::array<VkSubpassDependency, 2> dependencies; // see struct definition for more details
    dependencies[0] = { VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_MEMORY_READ_BIT,
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0 };

    dependencies[1] = { 0, VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        VK_ACCESS_MEMORY_READ_BIT, 0 };
    */

    /**************************************************************************************************************
    // Specify the operations to wait on and stages when ops occur.
    // Need to wait for swap chain to finish reading, can be accomplished by waiting on the colour attachment
    // output stage.
    // Need to make sure there are no conflicts between transitionning og the depth image and it being cleared as
    // part of its load operation.
    // The depth image is first accessed in the early fragment test pipeline stage and because we have a load
    // operation that clears, we should specify the access mask for writes.
    **************************************************************************************************************/

    // create the render pass
    std::array<VkAttachmentDescription, 1> attachments = { colorAttachment }; // , depthAttachment };
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<UI32>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass; // associated supass
    // specify the dependency
    renderPassInfo.dependencyCount = 1;// static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies = &dependency;// dependencies.data();

    // explicitly create the renderpass
    if (vkCreateRenderPass(_context.device, &renderPassInfo, nullptr, &_renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
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

void Renderer::createOffscreenRenderPass() {
    // attachment descriptions and references
    std::array<VkAttachmentDescription, NUM_ATTACHMENTS> attachmentDescriptions = {};

    VkAttachmentDescription attachmentDescription{};
    attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // color attachments
    attachmentDescription.format = _gbuffer[POSITION]._format;
    attachmentDescriptions[POSITION] = attachmentDescription;

    attachmentDescription.format = _gbuffer[NORMAL]._format;
    attachmentDescriptions[NORMAL] = attachmentDescription;

    attachmentDescription.format = _gbuffer[ALBEDO]._format;
    attachmentDescriptions[ALBEDO] = attachmentDescription;

    // depth attachment
    attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachmentDescription.format = _gbuffer[DEPTH]._format;
    attachmentDescriptions[DEPTH] = attachmentDescription;

    // attachment references
    std::vector<VkAttachmentReference> colourReferences;
    colourReferences.push_back({ POSITION, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
    colourReferences.push_back({ NORMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
    colourReferences.push_back({ ALBEDO, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });

    VkAttachmentReference depthReference = { DEPTH, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };


    VkSubpassDescription subpass{}; // create subpass
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.pColorAttachments = colourReferences.data();
    subpass.colorAttachmentCount = static_cast<uint32_t>(colourReferences.size());
    subpass.pDepthStencilAttachment = &depthReference;

    std::array<VkSubpassDependency, 2> dependencies{}; // dependencies for attachment layout transition

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
    renderPassInfo.pAttachments    = attachmentDescriptions.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses   = &subpass;
    renderPassInfo.dependencyCount = 2;
    renderPassInfo.pDependencies   = dependencies.data();

    if (vkCreateRenderPass(_context.device, &renderPassInfo, nullptr, &_offscreenRenderPass) != VK_SUCCESS) {
        throw std::runtime_error("Could not create GBuffer's render pass");
    }
}
