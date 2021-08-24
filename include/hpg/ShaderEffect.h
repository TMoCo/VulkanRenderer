//
// Shader effect class declaration
//

// a shader effect contains the pipeline layout and descriptor set layout required for a pipeline
// a shader effect's data 

#ifndef SHADER_EFFECT_H
#define SHADER_EFFECT_H

#include <vulkan/vulkan_core.h>

#include <array>

enum kShaderStages {
	SHADOW,
	OFFSCREEN,
	NUM_STAGES
};


class ShaderEffect {
	VkPipelineLayout _pipelineLayout;
	std::array<VkDescriptorSetLayout, NUM_STAGES> _descriptorSetLayouts;

};




#endif // !SHADER_EFFECT_H
