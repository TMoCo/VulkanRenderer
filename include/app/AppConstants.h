//
// A convenience header file for application constants like app name
//

#ifndef APP_CONSTANTS_H
#define APP_CONSTANTS_H

#include <stdint.h> // uint32_t
#include <vector> // vector container
#include <string> // string class
#include <optional> // optional wrapper
#include <ios> // streamsize

#include <glm/glm.hpp>

//
// App constants
//

// constants for window dimensions
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

// strings for the vulkan instance
const std::string APP_NAME    = "Deferred Rendering";
const std::string ENGINE_NAME = "No Engine";

// max size for reading a line 
const std::streamsize MAX_SIZE = 1048;

// paths to the model
const std::string MODEL_PATH = "C:\\Users\\Tommy\\Documents\\Graphics\\Gltf\\Suzanne\\Suzanne.gltf";

// path to the skybox
const std::string SKYBOX_PATH = "C:\\Users\\Tommy\\Documents\\Graphics\\CubeMaps\\sky\\";

// path to shaders
const std::string SHADER_DIR = "C:\\Users\\Tommy\\Documents\\Graphics\\VulkanGraphics\\src\\shaders\\";


namespace Axes {
	// world axes
	const glm::vec3 WORLD_RIGHT = glm::vec3(1.0f, 0.0f, 0.0f);
	const glm::vec3 WORLD_UP = glm::vec3(0.0f, 1.0f, 0.0f);
	const glm::vec3 WORLD_FRONT = glm::vec3(0.0f, 0.0f, 1.0f);

	const glm::vec3 X = glm::vec3(1.0f, 0.0f, 0.0f);
	const glm::vec3 Y = glm::vec3(0.0f, 1.0f, 0.0f);
	const glm::vec3 Z = glm::vec3(0.0f, 0.0f, 1.0f);

	// default directions
	const glm::vec3 RIGHT = glm::vec3(1.0f, 0.0f, 0.0f);
	const glm::vec3 LEFT = glm::vec3(-1.0f, 0.0f, 0.0f);
	const glm::vec3 UP = glm::vec3(0.0f, 1.0f, 0.0f);
	const glm::vec3 DOWN = glm::vec3(0.0f, -1.0f, 0.0f);
	const glm::vec3 FRONT = glm::vec3(0.0f, 0.0f, -1.0f);
	const glm::vec3 BACK = glm::vec3(0.0f, 0.0f, 1.0f);
}

#endif // !APP_CONSTANTS_H