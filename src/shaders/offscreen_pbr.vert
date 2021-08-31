#version 450
#extension GL_ARB_separate_shader_objects : enable

//
// Vertex shader for deferred rendering offscreen stage 
// 

// uniform
layout(binding = 0, std140) uniform UniformBufferObject {
    mat4 model;
    mat4 viewProj;
} ubo;

// inputs specified in the vertex buffer attributes
layout(location = 0) in vec4 inPositionU;
layout(location = 1) in vec4 inNormalV;
layout(location = 2) in vec4 inTangent;

// outputs
layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec4 fragTangent;
layout(location = 3) out vec2 fragTexCoord;

void main() {
	vec4 tmpPos = ubo.model * vec4(inPositionU.xyz, 1.0f);
	gl_Position = ubo.viewProj * tmpPos;
	// position
	fragPos     = tmpPos.xyz;
	// normal
    fragNormal   = normalize(mat3(ubo.model) * inNormalV.xyz);
	// tangent
	fragTangent  = vec4(normalize(ubo.model * inTangent).xyz, inTangent.w);
	// texture uv
    fragTexCoord = vec2(inPositionU.w, inNormalV.w);
}