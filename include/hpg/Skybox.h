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

#include <hpg/Renderer.h>
#include <hpg/TextureCube.h>
#include <hpg/Image.h>

typedef struct {
	glm::mat4 projectionView;
} SkyboxUBO;

class Skybox {
public: 
	//-Unfiorm buffer object---------------------------------------------//    

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
	Skybox() : _onCpu(false), _onGpu(false) {}
  
	bool load(const std::string& path);
	bool uploadToGpu(Renderer& renderer);
	void draw(VkCommandBuffer cmdBuffer);

	void cleanupSkybox(VkDevice device);

public:
	//-Members-----------------------------------------------------------//

	// pixels loaded from a cube map
	ImageData _imageData;

	// descriptors
	TextureCube _cubeMap;

	Buffer _uniformBuffer;

	VkDescriptorSet _descriptorSet;

	VkPipelineLayout _pipelineLayout;
	VkPipeline _pipeline;

	Buffer _vertexBuffer;

	bool _onCpu;
	bool _onGpu;
};

#endif // !SKYBOX_H

