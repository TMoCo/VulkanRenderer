///////////////////////////////////////////////////////
// Shadow map class 
///////////////////////////////////////////////////////

#ifndef SHADOW_MAP_H
#define SHADOW_MAP_H

#include <hpg/Image.h>
#include <hpg/Renderer.h>

#include <common/types.h>

#include <scene/Model.h>

class ShadowMap {
public:
	struct UBO {
		glm::mat4 depthMVP;
	};
	//-Initialisation and cleanup--------------------------------------------------------------------------------//
	void createShadowMap(const Renderer& renderer);
	void cleanupShadowMap(VkDevice device);

	void createAttachment(const Renderer& renderer);

	void createShadowMapRenderPass();

	void createShadowMapFrameBuffer();

	void createShadowMapSampler();

	void createShadowMapPipeline(VkDescriptorSetLayout descriptorSetLayout);

	void updateShadowMapUniformBuffer(const UBO& ubo);

public:
	VulkanContext* vkSetup;

	// an image representing the depth seen from the light's perspective, to be sampled
	Image		image;
	VkFormat    format = VK_FORMAT_D16_UNORM;
	//VkFormat    format = VK_FORMAT_D32_SFLOAT;
	VkImageView imageView;
	UI32 extent = 1024;
	F32 depthBiasConstant = 1.8f;
	F32 depthBiasSlope = 2.9f;

	VkSampler depthSampler;

	VkRenderPass shadowMapRenderPass;

	VkFramebuffer shadowMapFrameBuffer;

	VkPipelineLayout layout;
	VkPipeline shadowMapPipeline;

	Buffer shadowMapUniformBuffer;
};

#endif // !SHADOW_MAP_H