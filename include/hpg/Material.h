//
// Declaration of the material class
//

#ifndef MATERIAL_H
#define MATERIAL_H

#include <hpg/Texture2D.h>
#include <hpg/ShaderEffect.h>
#include <hpg/Renderer.h>

#include <tiny_gltf.h>

#include <array>

class Material {
public:
	void createPipeline(Renderer& renderer, kDescriptorSetLayout type);
	void cleanup(VkDevice device);

	VkPipeline _pipeline;
	VkPipelineLayout _pipelineLayout;
	VkDescriptorPool _descriptorPool;
	VkDescriptorSet _descriptorSet;

	std::vector<Texture2D> _textures;
};

#endif // !MATERIAL_H
