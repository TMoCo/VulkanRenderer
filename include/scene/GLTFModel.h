//
// A class containing a gltf model's data
//

#ifndef GLTF_MODEL_H
#define GLTF_MODEL_H

#include <hpg/Buffer.h>
#include <hpg/Renderer.h>
#include <hpg/Material.h>

#include <common/Vertex.h>

#include <glm/glm.hpp>

#include <tiny_gltf.h> // for extracting gltf data from file

constexpr VkPrimitiveTopology primitiveModes[7] = {
	VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
	VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
	VK_PRIMITIVE_TOPOLOGY_POINT_LIST, // no line loop primitive in vulkan (as far as I'm aware of)
	VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,
	VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
	VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN
};

typedef enum {
	POINTS,
	LINES,
	LINE_LOOP,
	LINE_STRIP,
	TRIANGLES,
	TRANGLE_STRIP,
	TRIANGLE_FAN
} primitiveMode;


class GLTFModel {
public:
	GLTFModel() : onCpu(false), onGpu(false) {}

	bool load(const std::string& path);

	bool uploadToGpu(Renderer& renderer);
	bool cleanup(Renderer& renderer);

	void draw(VkCommandBuffer buffer);

	// model data from tinygltf model
	tinygltf::Model _model;

	std::vector<Vertex> _vertices;
	std::vector<UI32> _indices;

	// data for rendering model
	std::vector<Material> _materials;

	Buffer _vertexBuffer;
	Buffer _indexBuffer;
	
	Buffer _uniformBuffer;

	bool onCpu;
	bool onGpu;
};

#endif // !GLTF_MODEL_H

