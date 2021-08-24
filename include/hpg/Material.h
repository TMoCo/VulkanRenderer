//
// Declaration of the material class
//

#ifndef MATERIAL_H
#define MATERIAL_H


#include <unordered_map>

#include <hpg/Texture.h>
#include <hpg/ShaderEffect.h>

enum kTransparency {
	OPAQUE,
	MASK,
	BLEND
};
 
struct MaterialParameters {
	glm::vec4 emissive;
};

class Material {
public:

	std::vector<VkDescriptorSet> _descriptorSets;
	std::vector<Texture> _textures;

	MaterialParameters _parameters;
};

#endif // !MATERIAL_H
