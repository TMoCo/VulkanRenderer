//
// A class containing a gltf model's data
//

#ifndef GLTF_MODEL_H
#define GLTF_MODEL_H

#include <tiny_gltf.h> // for extracting gltf data from file

#include <glm/glm.hpp>

class GLTFModel {
public:
	void load(const std::string& path);

	tinygltf::Model model;
};

#endif // !GLTF_MODEL_H

