#version 450
#extension GL_ARB_separate_shader_objects : enable

//
// Simple forward rendering vertex shader stage
// 
 
// Uniform
layout(binding = 0, std140) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
	mat4 proj;
} ubo;

// inputs specified in the vertex buffer attributes
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inTangent;
layout(location = 3) in vec2 inTexCoord;

// outputs to next shader stage
layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragTexCoord;

void main() {
	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0f);

	fragPos      = (ubo.model * vec4(inPosition, 1.0f)).xyz;
    fragNormal   = inNormal;
    fragTexCoord = inTexCoord;
}