///////////////////////////////////////////////////////
//
// Skybox class declaration
//
///////////////////////////////////////////////////////

//
// A skybox class for handling skybox data (essentially a type of texture associated to a small cube)
//

#ifndef SKYBOX_H
#define SKYBOX_H

#include <glm/glm.hpp>

#include <common/Model.h>

class Skybox {
public: 
	//-Unfiorm buffer object---------------------------------------------//    
	struct UBO {
		glm::mat4 projection;
		glm::mat4 view;
	};

	//-Vertices for a small cube ----------------------------------------//    
	glm::vec3 cubeVerts[36] = {
		// back
		{ -1.0f, 1.0f, -1.0f },
		{ -1.0f, -1.0f, -1.0f },
		{ 1.0f, -1.0f, -1.0f },
		{ 1.0f, -1.0f, -1.0f },
		{ 1.0f, 1.0f, -1.0f },
		{ -1.0f, 1.0f, -1.0f },

		// left
		{ -1.0f, -1.0f, 1.0f },
		{ -1.0f, -1.0f, -1.0f },
		{ -1.0f, 1.0f, -1.0f },
		{ -1.0f, 1.0f, -1.0f },
		{ -1.0f, 1.0f, 1.0f },
		{ -1.0f, -1.0f, 1.0f },

		// front
		{ 1.0f, -1.0f, -1.0f },
		{ 1.0f, -1.0f, 1.0f },
		{ 1.0f, 1.0f, 1.0f },
		{ 1.0f, 1.0f, 1.0f },
		{ 1.0f, 1.0f, -1.0f },
		{ 1.0f, -1.0f, -1.0f },

		// right 
		{ -1.0f, -1.0f, 1.0f },
		{ -1.0f, 1.0f, 1.0f },
		{ 1.0f, 1.0f, 1.0f },
		{ 1.0f, 1.0f, 1.0f },
		{ 1.0f, -1.0f, 1.0f },
		{ -1.0f, -1.0f, 1.0f },

		// top
		{ -1.0f, 1.0f, -1.0f },
		{ 1.0f, 1.0f, -1.0f },
		{ 1.0f, 1.0f, 1.0f },
		{ 1.0f, 1.0f, 1.0f },
		{ -1.0f, 1.0f, 1.0f },
		{ -1.0f, 1.0f, -1.0f },

		// bottom
		{ -1.0f, -1.0f, -1.0f },
		{ -1.0f, -1.0f, 1.0f },
		{ 1.0f, -1.0f, -1.0f },
		{ 1.0f, -1.0f, -1.0f },
		{ -1.0f, -1.0f, 1.0f },
		{ 1.0f, -1.0f, 1.0f }
	};

public:
	//-Initialisation and cleanup----------------------------------------//    
	void createSkybox(VulkanContext* pVkSetup, const VkCommandPool& commandPool);
	void cleanupSkybox();

	//-Skybox uniform object update -------------------------------------//    
	void updateSkyboxUniformBuffer(const UBO& ubo) {
		void* data;
		vkMapMemory(vkSetup->device, uniformBuffer._memory, 0, sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(vkSetup->device, uniformBuffer._memory);
	}

private:
	//-Skybox sampler creation-------------------------------------------//    
	void createSkyboxSampler();

	//-Skybox image creation---------------------------------------------//    
	void createSkyboxImage(const VkCommandPool& commandPool);

	//-Skybox image view creation----------------------------------------//    
	void createSkyboxImageView();

public:
	//-Members-----------------------------------------------------------//    
	VulkanContext* vkSetup;

	Image    skyboxImage;
	VkImageView    skyboxImageView;

	VkSampler skyboxSampler;

	Buffer vertexBuffer;
	Buffer uniformBuffer;
};

#endif // !SKYBOX_H

