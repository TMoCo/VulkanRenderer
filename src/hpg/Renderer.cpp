#include <hpg/Renderer.h>

#include <utils/vkinit.h>

void Renderer::init(GLFWwindow* window) {
	// setup vulkan context
	_context.init(window);

    createCommandPool(&_commandPools[RENDER], VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    createCommandPool(&_commandPools[GUI], VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
}

void Renderer::cleanup() {
    vkDestroyCommandPool(_context.device, _commandPools[RENDER], nullptr);
    vkDestroyCommandPool(_context.device, _commandPools[GUI], nullptr);

    _context.cleanup();
}

void Renderer::createCommandPool(VkCommandPool* commandPool, VkCommandPoolCreateFlags flags) {
    utils::QueueFamilyIndices queueFamilyIndices =
        utils::QueueFamilyIndices::findQueueFamilies(_context.physicalDevice, _context.surface);
    VkCommandPoolCreateInfo poolInfo = vkinit::commandPoolCreateInfo(queueFamilyIndices.graphicsFamily.value(), flags);
    if (vkCreateCommandPool(_context.device, &poolInfo, nullptr, commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
}
