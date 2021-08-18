//
// Renderer class
//

#ifndef RENDERER_H
#define RENDERER_H

#include <hpg/VulkanContext.h>
#include <hpg/SwapChain.h>
#include <hpg/FrameBuffer.h>
#include <hpg/GBuffer.h>

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
	void recreateSwapchain();
	F32 aspectRatio();

private:
	void createCommandPool(VkCommandPool* commandPool, VkCommandPoolCreateFlags flags);
	void createSyncObjects();


public:
	// the vulkan context
	VulkanContext _context;

	// command pools and buffers
	std::array<VkCommandPool, POOLNUM> _commandPools;

	// swap chain
	SwapChain _swapChain;

	// frame buffer
	FrameBuffer _frameBuffer;

	// gbuffer
	GBuffer _gBuffer;

	// main render pass

	// synchronisation
	std::vector<VkSemaphore> _offScreenSemaphores;
	std::vector<VkSemaphore> _imageAvailableSemaphores; // 1 semaphore per frame, GPU-GPU sync
	std::vector<VkSemaphore> _renderFinishedSemaphores;

	std::vector<VkFence> _inFlightFences; // 1 fence per frame, CPU-GPU sync
	std::vector<VkFence> _imagesInFlight;
};


#endif // !RENDERER_H

