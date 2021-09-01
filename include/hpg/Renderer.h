//
// Renderer class
//

#ifndef RENDERER_H
#define RENDERER_H

// #define NDEBUG

#include <hpg/VulkanContext.h>
#include <hpg/SwapChain.h>
#include <hpg/Buffer.h>

#include <array>

// struct representing a light
typedef struct {
	glm::vec4 position;
	glm::vec4 parameters; // xyz = color, w = radius
} Light;

// struct containing data for composing final image
typedef struct {
	glm::vec4 guiData;
	glm::mat4 depthMVP;
	glm::mat4 cameraMVP;
	Light lights[1];
} CompositionUBO;

typedef struct {
	glm::mat4 model;
	glm::mat4 projectionView;
} OffscreenUBO;

typedef enum {
	RENDER_CMD_POOL,
	GUI_CMD_POOL,
	CMD_POOLS_MAX_ENUM
} kCommandPools;

// for whole render pass
typedef enum {
	COLOR_ATTACHMENT,
	GBUFFER_POSITION_ATTACHMENT,
	GBUFFER_NORMAL_ATTACHMENT,
	GBUFFER_ALBEDO_ATTACHMENT,
	GBUFFER_AO_METALLIC_ROUGHNESS_ATTACHMENT,
	GBUFFER_DEPTH_ATTACHMENT,
	ATTACHMENTS_MAX_ENUM
} kAttachments;

// for indexing the elements
typedef enum {
	GBUFFER_POSITION,
	GBUFFER_NORMAL,
	GBUFFER_ALBEDO,
	GBUFFER_AO_METALLIC_ROUGHNESS,
	GBUFFER_DEPTH,
	GBUFFER_MAX_ENUM
} kGbuffer;

// indexing subpasses in main render pass
typedef enum {
	OFFSCREEN_SUBPASS,
	COMPOSITION_SUBPASS,
	SUBPASS_MAX_ENUM
} kSubpass;

// bit mask for identifying texture
typedef enum kTextureBits {
	NO_TEXTURE_BIT = 0x0,
	ALBEDO_TEXTURE_BIT = 0x1,
	OCCLUSION_METALLIC_ROUGNESS_TEXTURE_BIT = 0x2,
	NORMAL_TEXTURE_BIT = 0x4,
	EMISSIVE_TEXTURE_BIT = 0x8
} kTextureBits;
typedef UI32 kTextureMask;

// indexing descriptor set layouts
typedef enum {
	OFFSCREEN_DEFAULT_DESCRIPTOR_LAYOUT,
	OFFSCREEN_PBR_DESCRIPTOR_LAYOUT,
	OFFSCREEN_PBR_NORMAL_DESCRIPTOR_LAYOUT,
	OFFSCREEN_PBR_NORMAL_EMISSIVE_DESCRIPTOR_LAYOUT,
	OFFSCREEN_SKYBOX_DESCRIPTOR_LAYOUT,
	OFFSCREEN_SHADOWMAP_DESCRIPTOR_LAYOUT,
	COMPOSITION_DESCRIPTOR_LAYOUT,
	DESCRIPTOR_SET_LAYOUT_MAX_ENUM
} kDescriptorSetLayout;

constexpr std::array<std::pair<const char*, const char*>, DESCRIPTOR_SET_LAYOUT_MAX_ENUM> kShaders = {
	std::pair{ "offscreen_default.vert.spv", "offscreen_default.frag.spv" },
	std::pair{ "offscreen_pbr.vert.spv", "offscreen_pbr.frag.spv" },
	std::pair{ "offscreen_pbr.vert.spv", "offscreen_pbr_normal.frag.spv" },
	std::pair{ "offscreen_pbr.vert.spv", "offscreen_pbr_normal.frag.spv" },
	std::pair{ "skybox.vert.spv", "skybox.frag.spv" },
	std::pair{ "shadowmap.vert.spv", "shadowmap.frag.spv" },
	std::pair{ "composition.vert.spv", "composition.frag.spv" } };

class Renderer {
	//-Render pass attachment------------------------------------------------------------------------------------//    
	class Attachment {
	public:
		inline void cleanup(VkDevice device) {
			vkDestroyImageView(device, _view, nullptr);
			vkDestroyImage(device, _image, nullptr);
			vkFreeMemory(device, _memory, nullptr);
		}

		VkImage _image;
		VkDeviceMemory _memory;
		VkImageView _view;
		VkFormat _format;
	};

public:
	void init(GLFWwindow* window);
	void cleanup();
	void resize();
	void render();

	inline F32 aspectRatio() { return _swapChain._aspectRatio; }

private:
	void createCommandPool(VkCommandPool* commandPool, VkCommandPoolCreateFlags flags);
	void createSyncObjects();
	void createFramebuffers();
	void createAttachment(Attachment& attachment, VkImageUsageFlags usage, VkExtent2D extent, VkFormat format);
	void createColorSampler();
	void createCommandBuffers();

	void createDescriptorPool();
	void createDescriptorSetLayouts();
	void createCompositionDescriptorSets();
	void createCompositionPipeline();

	void createGuiRenderPass();
	void createRenderPass();

public:
	// the vulkan context
	VulkanContext _context;

	// command pools and buffers
	std::array<VkCommandPool, CMD_POOLS_MAX_ENUM> _commandPools;
	std::vector<VkCommandBuffer> _renderCommandBuffers;
	std::vector<VkCommandBuffer> _guiCommandBuffers;

	VkDescriptorPool _descriptorPool;
	// the base descriptor set layouts used. All descriptor sets are derived from these layouts
	std::array<VkDescriptorSetLayout, DESCRIPTOR_SET_LAYOUT_MAX_ENUM> _descriptorSetLayouts;
	// TODO: Change to push constant
	Buffer _compositionUniforms;

	// composition pipeline
	VkPipelineLayout _compositionPipelineLayout;
	VkPipeline _compositionPipeline;

	// swap chain
	SwapChain _swapChain;

	// frame buffers
	std::vector<VkFramebuffer> _framebuffers;
	std::vector<VkFramebuffer> _guiFramebuffers;

	// gbuffer
	std::array<Attachment, GBUFFER_MAX_ENUM> _gbuffer;

	// final render descriptors
	std::vector<VkDescriptorSet> _compositionDescriptorSets;

	// color sampler
	VkSampler _colorSampler;

	// main render pass
	VkRenderPass _renderPass;
	VkRenderPass _guiRenderPass;

	// synchronisation
	std::vector<VkSemaphore> _imageAvailableSemaphores; // 1 semaphore per frame, GPU-GPU sync
	std::vector<VkSemaphore> _renderFinishedSemaphores;

	std::vector<VkFence> _inFlightFences; // 1 fence per frame, CPU-GPU sync
	std::vector<VkFence> _imagesInFlight;

	UI16 currentFrame = 0;
	UI32 imageIndex   = 0;
};


#endif // !RENDERER_H

