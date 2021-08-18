//
// Renderer class
//

#ifndef RENDERER_H
#define RENDERER_H

#include <hpg/VulkanContext.h>

#include <array>

enum kCmdPools {
	RENDER,
	GUI,
	POOLNUM
};

class Renderer {
public:
	void init(GLFWwindow* window);
	void cleanup();

private:
	void createCommandPool(VkCommandPool* commandPool, VkCommandPoolCreateFlags flags);

public:
	// the vulkan context
	VulkanContext _context;

	// command pools and buffers
	std::array<VkCommandPool, POOLNUM> _commandPools;

	// swap chain

	// frame buffer

	// gbuffer

	// main render pass

	// synchronisation

};


#endif // !RENDERER_H

