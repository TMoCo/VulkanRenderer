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
	NUM_POOLS
};

class Renderer {
	//-Render pass attachment------------------------------------------------------------------------------------//    
	class Attachment {
	public:
		void cleanup(VulkanContext* context);

		Image _image;
		VkFormat _format;
		VkImageView _view;
	};

public:
	void init(GLFWwindow* window);
	void cleanup();
	void recreateSwapchain();

	inline F32 aspectRatio() { return _swapChain._aspectRatio; }

private:
	void createCommandPool(VkCommandPool* commandPool, VkCommandPoolCreateFlags flags);
	void createSyncObjects();
	void createFrambuffers();
	void createAttachment(Attachment& attachment, VkImageUsageFlagBits usage, VkExtent2D extent, VkFormat format);

	// TODO: move render pass outside of swap chain (merge with other renderpasses)
	//-Render passes---------------------------------------------------------------------------------------------//    
	void createFwdRenderPass();
	void createGuiRenderPass();


public:
	// the vulkan context
	VulkanContext _context;

	// command pools and buffers
	std::array<VkCommandPool, NUM_POOLS> _commandPools;

	// swap chain
	SwapChain _swapChain;

	// frame buffers
	std::vector<VkFramebuffer> _framebuffers;
	std::vector<VkFramebuffer> _guiFramebuffers;

	// gbuffer
	GBuffer _gbuffer;
	Attachment _depth;

	// main render pass
	// TODO: Merge render passes
	VkRenderPass _renderPass;
	VkRenderPass _guiRenderPass;

	// synchronisation
	std::vector<VkSemaphore> _offScreenSemaphores;
	std::vector<VkSemaphore> _imageAvailableSemaphores; // 1 semaphore per frame, GPU-GPU sync
	std::vector<VkSemaphore> _renderFinishedSemaphores;

	std::vector<VkFence> _inFlightFences; // 1 fence per frame, CPU-GPU sync
	std::vector<VkFence> _imagesInFlight;
};


#endif // !RENDERER_H

