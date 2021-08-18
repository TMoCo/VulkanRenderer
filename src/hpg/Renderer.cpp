#include <hpg/Renderer.h>

#include <utils/vkinit.h>

void Renderer::init(GLFWwindow* window) {
	// setup vulkan context
	_context.init(window);

    createCommandPool(&_commandPools[RENDER], VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    createCommandPool(&_commandPools[GUI], VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    _swapChain.init(&_context);

    createSyncObjects();

    // build render pass
}

void Renderer::cleanup() {
    for (UI32 i = 0; i < _swapChain._imageCount; i++) {
        vkDestroySemaphore(_context.device, _renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(_context.device, _imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(_context.device, _offScreenSemaphores[i], nullptr);
        vkDestroyFence(_context.device, _inFlightFences[i], nullptr);
    }

    _swapChain.cleanup();

    vkDestroyCommandPool(_context.device, _commandPools[RENDER], nullptr);
    vkDestroyCommandPool(_context.device, _commandPools[GUI], nullptr);

    _context.cleanup();
}

void Renderer::recreateSwapchain() {
    // destroy previous swapchain and dependencies 
    _swapChain.cleanup();

    // recreate swapchain and dependencies
    _swapChain.init(&_context);
}

F32 Renderer::aspectRatio() {
    return _swapChain._aspectRatio;
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
    _imageAvailableSemaphores.resize(_swapChain._imageCount);
    _renderFinishedSemaphores.resize(_swapChain._imageCount);
    _offScreenSemaphores.resize(_swapChain._imageCount);

    _inFlightFences.resize(_swapChain._imageCount);
    _imagesInFlight.resize(_swapChain._imageCount, VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphoreCreateInfo();

    VkFenceCreateInfo fenceCreateInfo = vkinit::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);

    for (UI32 i = 0; i < _swapChain._imageCount; i++) {
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
