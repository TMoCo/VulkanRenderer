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


// for whole render pass
enum kAttachments {
	COLOR,
	GBUFFER_POSITION,
	GBUFFER_NORMAL,
	GBUFFER_ALBEDO,
	GBUFFER_METALLIC_ROUGHNESS,
	GBUFFER_DEPTH,
	NUM_ATTACHMENTS
};

// exclusively for accessing the gbuffer elements
enum kGbuffer {
	POSITION,
	NORMAL,
	ALBEDO,
	METALLIC_ROUGHNESS,
	DEPTH,
	NUM_GBUFFER_ATTACHMENTS
};

// indexing subpasses in main render pass
enum kSubpasses {
	OFFSCREEN,
	COMPOSITION,
	NUM_SUBPASSES
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
	void render();

	inline F32 aspectRatio() { return _swapChain._aspectRatio; }

private:
	void createCommandPool(VkCommandPool* commandPool, VkCommandPoolCreateFlags flags);
	void createSyncObjects();
	void createFramebuffers();
	void createAttachment(Attachment& attachment, VkImageUsageFlags usage, VkExtent2D extent, VkFormat format);
	void createColorSampler();
	void createCommandBuffers();

	// TODO: merge renderpasses
	void createGuiRenderPass();
	void createRenderPass();

public:
	// the vulkan context
	VulkanContext _context;

	// command pools and buffers
	std::array<VkCommandPool, NUM_POOLS> _commandPools;
	std::vector<VkCommandBuffer> _renderCommandBuffers;
	std::vector<VkCommandBuffer> _guiCommandBuffers;

	// swap chain
	SwapChain _swapChain;

	// frame buffers
	std::vector<VkFramebuffer> _framebuffers;
	std::vector<VkFramebuffer> _guiFramebuffers;
	// gbuffer
	std::array<Attachment, NUM_GBUFFER_ATTACHMENTS> _gbuffer;

	// color sampler
	VkSampler _colorSampler;

	// main render pass
	// TODO: Merge render passes
	VkRenderPass _renderPass;
	VkRenderPass _guiRenderPass;

	// synchronisation
	std::vector<VkSemaphore> _imageAvailableSemaphores; // 1 semaphore per frame, GPU-GPU sync
	std::vector<VkSemaphore> _renderFinishedSemaphores;

	std::vector<VkFence> _inFlightFences; // 1 fence per frame, CPU-GPU sync
	std::vector<VkFence> _imagesInFlight;

	UI16 currentFrame = 0;
	UI32 imageIndex = 0;
};


#endif // !RENDERER_H

