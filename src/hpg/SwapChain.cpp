//
// Definition of the SwapChain class
//

#include <app/AppConstants.h>

#include <common/Model.h>

#include <utils/vkinit.h>

#include <hpg/SwapChain.h>// include the class declaration
#include <hpg/Shader.h>   // include the shader struct 
#include <hpg/Image.h>    // image view create

// exceptions
#include <iostream>
#include <stdexcept>


void SwapChain::createSwapChain(VulkanContext* pVkSetup, VkDescriptorSetLayout* descriptorSetLayout) {
    // update the pointer to the setup data rather than passing as argument to functions
    vkSetup = pVkSetup;
    // create the swap chain
    createSwapChain();

    // then create the image views for the images created
    imageViews.resize(images.size());
    for (size_t i = 0; i < images.size(); i++) {
        VkImageViewCreateInfo imageViewCreateInfo = vkinit::imageViewCreateInfo(images[i],
            VK_IMAGE_VIEW_TYPE_2D, imageFormat, {}, { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
        imageViews[i] = Image::createImageView(vkSetup, imageViewCreateInfo);
    }

    // then the geometry render pass 
    createRenderPass();

    // and the ImGUI render pass
    createImGuiRenderPass();

    // followed by the graphics pipeline
    createForwardPipeline(descriptorSetLayout);
}

void SwapChain::cleanupSwapChain() {
    // destroy pipeline and related data
    vkDestroyPipeline(vkSetup->device, pipeline, nullptr);
    vkDestroyPipelineLayout(vkSetup->device, pipelineLayout, nullptr);

    // destroy the render passes
    vkDestroyRenderPass(vkSetup->device, renderPass, nullptr);
    vkDestroyRenderPass(vkSetup->device, imGuiRenderPass, nullptr);

    // loop over the image views and destroy them. NB we don't destroy the images because they are implicilty created
    // and destroyed by the swap chain
    for (size_t i = 0; i < imageViews.size(); i++) {
        vkDestroyImageView(vkSetup->device, imageViews[i], nullptr);
    }

    // destroy the swap chain proper
    vkDestroySwapchainKHR(vkSetup->device, swapChain, nullptr);
}

void SwapChain::createSwapChain() {
    supportDetails = querySwapChainSupport(vkSetup->physicalDevice, vkSetup->surface); // is sc supported

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(supportDetails.formats);
    VkPresentModeKHR presentMode     = chooseSwapPresentMode(supportDetails.presentModes);
    VkExtent2D newExtent             = chooseSwapExtent(supportDetails.capabilities);

    uint32_t imageCount = supportDetails.capabilities.minImageCount + 1; // + 1 to avoid waiting
    if (supportDetails.capabilities.maxImageCount > 0 && imageCount > supportDetails.capabilities.maxImageCount) {
        imageCount = supportDetails.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface          = vkSetup->surface; // glfw window
    createInfo.minImageCount    = imageCount;
    createInfo.imageFormat      = surfaceFormat.format;
    createInfo.imageColorSpace  = surfaceFormat.colorSpace;
    createInfo.imageExtent      = newExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // how to handle the sc images across multiple queue families (in case graphics queue is different to presentation queue)
    utils::QueueFamilyIndices indices = utils::QueueFamilyIndices::findQueueFamilies(vkSetup->physicalDevice, vkSetup->surface);
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT; // image owned by one queue family, ownership must be transferred explicilty
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices   = queueFamilyIndices;
    }
    else {
        createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE; // images can be used accross queue families with no explicit transfer
    }

    // a certain transform to apply to the image
    createInfo.preTransform   = supportDetails.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode    = presentMode; // determined earlier
    createInfo.clipped        = VK_TRUE; // ignore obscured pixels
    createInfo.oldSwapchain   = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(vkSetup->device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    // get images
    vkGetSwapchainImagesKHR(vkSetup->device, swapChain, &imageCount, nullptr);
    images.resize(imageCount);
    vkGetSwapchainImagesKHR(vkSetup->device, swapChain, &imageCount, images.data());

    // save format and extent
    imageFormat = surfaceFormat.format;
    extent      = newExtent;
}

VkSurfaceFormatKHR SwapChain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    // VkSurfaceFormatKHR entry contains a format and colorSpace member
    // format is colour channels and type eg VK_FORMAT_B8G8R8A8_SRGB (8 bit uint BGRA channels, 32 bits per pixel)
    // colorSpace is the coulour space that indicates if SRGB is supported with VK_COLOR_SPACE_SRGB_NONLINEAR_KHR 
    // (used to be VK_COLORSPACE_SRGB_NONLINEAR_KHR)
    /*************************************************************************************************************/

    // loop through available formats
    for (const auto& availableFormat : availableFormats) {
        // if the correct combination of desired format and colour space exists then return the format
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }
    // if above fails, we could rank available formats based on how "good" they are for our task, settle for first element for now 
    return availableFormats[0];
}

VkPresentModeKHR SwapChain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    // presentation mode, can be one of four possible values:
    // VK_PRESENT_MODE_IMMEDIATE_KHR -> image submitted by app is sent straight to screen, may result in tearing
    // VK_PRESENT_MODE_FIFO_KHR -> swap chain is a queue where display takes an image from front when display is 
    // refreshed. Program inserts rendered images at back. 
    // If queue full, program has to wait. Most similar vsync. Moment display is refreshed is "vertical blank".
    // VK_PRESENT_MODE_FIFO_RELAXED_KHR -> Mode only differs from previous if application is late and queue empty 
    // at last vertical blank. Instead of waiting for next vertical blank. image is transferred right away when it 
    // finally arrives, may result tearing.
    // VK_PRESENT_MODE_MAILBOX_KHR -> another variation of second mode. Instead of blocking the app when queue is 
    // full, images that are already queued are replaced with newer ones. Can be used to implement triple buffering
    // which allows to avoid tearing with less latency issues than standard vsync using double buffering.   
    /*************************************************************************************************************/

    for (const auto& availablePresentMode : availablePresentModes) {
        // use triple buffering if available
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D SwapChain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    // swap extent is the resolution of the swap chain images, almost alwawys = to window res we're drawing pixels 
    // in match resolution by setting width and height in currentExtent member of VkSurfaceCapabilitiesKHR struct.
    /*************************************************************************************************************/
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    }
    else {
        // get the dimensions of the window
        int width, height;
        glfwGetFramebufferSize(vkSetup->window, &width, &height);

        // prepare the struct with the height and width of the window
        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        // clamp the values between allowed min and max extents by the surface
        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}

void SwapChain::createRenderPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format         = imageFormat; // sc image format
    colorAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // specify a depth attachment to the render pass
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format         = DepthResource::findDepthFormat(vkSetup); // same format as depth image
    depthAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    /**************************************************************************************************************
    // A single render pass consists of multiple subpasses, which are subsequent rendering operations depending on 
    // content of framebuffers on previous passes (eg post processing). Grouping subpasses into a single render 
    // pass lets Vulkan optimise every subpass references 1 or more attachments.
    **************************************************************************************************************/

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // which layout during a subpass

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // the subpass
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS; // explicit a graphics subpass 
    subpass.colorAttachmentCount    = 1;
    subpass.pColorAttachments       = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

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
    dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass    = 0;
    dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
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
    std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments    = attachments.data();
    renderPassInfo.subpassCount    = 1;
    renderPassInfo.pSubpasses      = &subpass; // associated supass
    // specify the dependency
    renderPassInfo.dependencyCount = 1;// static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies = &dependency;// dependencies.data();

    // explicitly create the renderpass
    if (vkCreateRenderPass(vkSetup->device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
}

void SwapChain::createImGuiRenderPass() {
    VkAttachmentDescription attachment = {};
    attachment.format         = imageFormat;
    attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // the initial layout is the image of scene geometry
    attachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment = {};
    color_attachment.attachment = 0;
    color_attachment.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments    = &color_attachment;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass    = 0;
    dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo info = {};
    info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    info.attachmentCount = 1;
    info.pAttachments    = &attachment;
    info.subpassCount    = 1;
    info.pSubpasses      = &subpass;
    info.dependencyCount = 1;
    info.pDependencies   = &dependency;

    if (vkCreateRenderPass(vkSetup->device, &info, nullptr, &imGuiRenderPass) != VK_SUCCESS) {
        throw std::runtime_error("Could not create Dear ImGui's render pass");
    }
}

void SwapChain::createForwardPipeline(VkDescriptorSetLayout* descriptorSetLayout) {
    VkShaderModule vertShaderModule = Shader::createShaderModule(vkSetup, Shader::readFile(FWD_VERT_SHADER));
    VkShaderModule fragShaderModule = Shader::createShaderModule(vkSetup, Shader::readFile(FWD_FRAG_SHADER));

    auto bindingDescription    = Model::getBindingDescriptions(0);
    auto attributeDescriptions = Model::getAttributeDescriptions(0);

    VkViewport viewport{ 0.0f, 0.0f, (float)extent.width, (float)extent.height, 0.0f, 1.0f };
    VkRect2D scissor{ { 0, 0 }, extent };

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = 
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {
        vkinit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule, "main"),
        vkinit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule, "main")
    };
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = 
        vkinit::pipelineVertexInputStateCreateInfo(1, &bindingDescription, 
            static_cast<uint32_t>(attributeDescriptions.size()), attributeDescriptions.data());
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = 
        vkinit::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE);
    VkPipelineViewportStateCreateInfo viewportState = 
        vkinit::pipelineViewportStateCreateInfo(1, &viewport, 1, &scissor);
    VkPipelineRasterizationStateCreateInfo rasterizer = 
        vkinit::pipelineRasterStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);    
    VkPipelineMultisampleStateCreateInfo multisampling = 
        vkinit::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);
    VkPipelineColorBlendStateCreateInfo colorBlending = 
        vkinit::pipelineColorBlendStateCreateInfo(1, &colorBlendAttachment);
    VkPipelineDepthStencilStateCreateInfo depthStencil = 
        vkinit::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS);
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = 
        vkinit::pipelineLayoutCreateInfo(1, descriptorSetLayout);

    if (vkCreatePipelineLayout(vkSetup->device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo = vkinit::graphicsPipelineCreateInfo(pipelineLayout, renderPass, 0);

    // fixed function pipeline
    pipelineInfo.pVertexInputState   = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState   = &multisampling;
    pipelineInfo.pColorBlendState    = &colorBlending;
    pipelineInfo.pDepthStencilState  = &depthStencil;
    pipelineInfo.stageCount          = static_cast<uint32_t>(shaderStages.size());
    pipelineInfo.pStages             = shaderStages.data();

    if (vkCreateGraphicsPipelines(vkSetup->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(vkSetup->device, fragShaderModule, nullptr);
    vkDestroyShaderModule(vkSetup->device, vertShaderModule, nullptr);
}

SwapChain::SupportDetails SwapChain::querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
    SwapChain::SupportDetails details;
    // query the surface capabilities and store in a VkSurfaceCapabilities struct
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities); // takes into account device and surface when determining capabilities

    // same as we have seen many times before
    uint32_t formatCount;
    // query the available formats, pass null ptr to just set the count
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    // if there are formats
    if (formatCount != 0) {
        // then resize the vector accordingly
        details.formats.resize(formatCount);
        // and set details struct fromats vector with the data pointer
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    // exact same thing as format for presentation modes
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}