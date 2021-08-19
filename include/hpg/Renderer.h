//
// Renderer class
//

#ifndef RENDERER_H
#define RENDERER_H

#include <hpg/VulkanContext.h>
#include <hpg/SwapChain.h>

#include <array>

enum kCmdPools {
	RENDER,
	GUI,
	NUM_POOLS
};

enum kGbuffer {
	POSITION,
	NORMAL,
	ALBEDO,
	DEPTH,
	NUM_ATTACHMENTS
};

class Renderer {
	//-Render pass attachment------------------------------------------------------------------------------------//    
	class Attachment {
	public:
		inline void cleanup(VulkanContext* context) {
			vkDestroyImageView(context->device, _view, nullptr);
			_image.cleanupImage(context);
		}

		Image _image;
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
	void createFramebuffers();
	void createAttachment(Attachment& attachment, VkImageUsageFlags usage, VkExtent2D extent, VkFormat format);
	void createColorSampler();

	// TODO: merge renderpasses
	void createFwdRenderPass();
	void createGuiRenderPass();
	void createOffscreenRenderPass(); 

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
	VkFramebuffer _offscreenFramebuffer;

	// gbuffer
	std::array<Attachment, NUM_ATTACHMENTS> _gbuffer;

	// color sampler
	VkSampler _colorSampler;

	// main render pass
	// TODO: Merge render passes
	VkRenderPass _renderPass;
	VkRenderPass _guiRenderPass;
	VkRenderPass _offscreenRenderPass;

	// synchronisation
	std::vector<VkSemaphore> _offScreenSemaphores;
	std::vector<VkSemaphore> _imageAvailableSemaphores; // 1 semaphore per frame, GPU-GPU sync
	std::vector<VkSemaphore> _renderFinishedSemaphores;

	std::vector<VkFence> _inFlightFences; // 1 fence per frame, CPU-GPU sync
	std::vector<VkFence> _imagesInFlight;
};


#endif // !RENDERER_H

