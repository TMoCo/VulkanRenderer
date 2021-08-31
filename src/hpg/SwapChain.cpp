//
// Definition of the SwapChain class
//

#include <app/AppConstants.h>

#include <scene/Model.h>

#include <common/vkinit.h>

#include <hpg/SwapChain.h>// include the class declaration
#include <hpg/Shader.h>   // include the shader struct 
#include <hpg/Image.h>    // image view create

// exceptions
#include <iostream>
#include <stdexcept>


bool SwapChain::create(VulkanContext& context) {
    bool hasNewImageCount = false;
    // query support
    {
        context.querySwapChainSupport();

        _surfaceFormat = chooseSwapSurfaceFormat(context._swapChainSupportDetails.formats);
        _extent = chooseSwapExtent(context._window, context._swapChainSupportDetails.capabilities);

        VkPresentModeKHR presentMode = chooseSwapPresentMode(context._swapChainSupportDetails.presentModes);

        UI32 newImageCount = context._swapChainSupportDetails.capabilities.minImageCount + 1; // + 1 to avoid waiting
        if (context._swapChainSupportDetails.capabilities.maxImageCount > 0 &&
            newImageCount > context._swapChainSupportDetails.capabilities.maxImageCount) {
            newImageCount = context._swapChainSupportDetails.capabilities.maxImageCount;
        }

        if (newImageCount != _imageCount) {
            hasNewImageCount = true; // means we need to recreate a lot of vulkan structures
        }
        
        _imageCount = newImageCount;
        _images.resize(_imageCount);
        _imageViews.resize(_imageCount);

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = context.surface; // glfw window
        createInfo.minImageCount = _imageCount;
        createInfo.imageFormat = _surfaceFormat.format;
        createInfo.imageColorSpace = _surfaceFormat.colorSpace;
        createInfo.imageExtent = _extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        // how to handle the sc images across multiple queue families (in case graphics queue is different to presentation queue)
        utils::QueueFamilyIndices indices = utils::QueueFamilyIndices::findQueueFamilies(context.physicalDevice, 
            context.surface);
        uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

        if (indices.graphicsFamily != indices.presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT; // image owned by one queue family, ownership must be transferred explicilty
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // images can be used accross queue families with no explicit transfer
        }

        // a certain transform to apply to the image
        createInfo.preTransform = context._swapChainSupportDetails.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode; // determined earlier
        createInfo.clipped = VK_TRUE; // ignore obscured pixels
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(context.device, &createInfo, nullptr, &_swapChain) != VK_SUCCESS) {
            throw std::runtime_error("failed to create swap chain!");
        }

        // specify desired num of images, then get pointers
        vkGetSwapchainImagesKHR(context.device, _swapChain, &_imageCount, nullptr);
        vkGetSwapchainImagesKHR(context.device, _swapChain, &_imageCount, _images.data());
    }

    // compute aspect ratio
    _aspectRatio = (F32)_extent.width / (F32)_extent.height;

    {
        // then create the image views for the images created
        for (UI32 i = 0; i < _imageCount; i++) {
            VkImageViewCreateInfo imageViewCreateInfo = vkinit::imageViewCreateInfo(_images[i],
                VK_IMAGE_VIEW_TYPE_2D, _surfaceFormat.format, {}, { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
            _imageViews[i] = Image::createImageView(&context, imageViewCreateInfo);
        }
    }

    return hasNewImageCount;
}

void SwapChain::cleanup(VkDevice device) {
    for (UI32 i = 0; i < _imageCount; i++) {
        vkDestroyImageView(device, _imageViews[i], nullptr);
    }
    vkDestroySwapchainKHR(device, _swapChain, nullptr);
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

VkExtent2D SwapChain::chooseSwapExtent(GLFWwindow* window, const VkSurfaceCapabilitiesKHR& capabilities) {
    // swap extent is the resolution of the swap chain images, almost alwawys = to window res we're drawing pixels 
    // in match resolution by setting width and height in currentExtent member of VkSurfaceCapabilitiesKHR struct.
    /*************************************************************************************************************/
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    }
    else {
        // get the dimensions of the window
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        // prepare the struct with the height and width of the window
        VkExtent2D actualExtent = { static_cast<UI32>(width), static_cast<UI32>(height) };

        // clamp the values between allowed min and max extents by the surface
        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}