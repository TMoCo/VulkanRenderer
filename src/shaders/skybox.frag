#version 450
#extension GL_ARB_separate_shader_objects : enable

//
// Fragment shader for deferred rendering skybox
// 

layout(binding = 1) uniform samplerCube skybox;

layout(location = 0) in vec3 inPosition;

// output to gbuffer
layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outAlbedo;
layout (location = 3) out vec4 outMetallicRoughness;

void main() {
	outPosition = vec4(inPosition, 1.0f);
	outNormal = vec4(0.0f);
	outAlbedo = vec4(inPosition, 1.0f);
	outAlbedo = texture(skybox, inPosition); // unit cube position we can use as a direction to sample cube map
	outMetallicRoughness = vec4(0.0f);
}
