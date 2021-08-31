#version 450

//
// fragment shader for deferred rendering offscreen stage 
//

// textures
layout (binding = 1) uniform sampler2D albedoSampler;
layout (binding = 2) uniform sampler2D metallicRoughnessSampler;
layout (binding = 3) uniform sampler2D normalSampler;

// input from previous stage
// outputs
layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec4 fragTangent;
layout(location = 3) in vec2 fragTexCoord;

layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outAlbedo;
layout (location = 3) out vec4 outMetallicRoughness;
// ADD output of metallicRoughness

const float far = 20.0f;
const float near = 0.1;

// compute linear depth
// https://learnopengl.com/Advanced-OpenGL/Depth-testing
float linearize_Z(float z , float zNear , float zFar){
	return (2 * zNear * zFar) / (zFar + zNear - (z * 2.0f  - 1.0f) * (zFar -zNear)) ;
}


void main() 
{
	// 1: position
	outPosition = vec4(fragPos, linearize_Z(gl_FragCoord.z, near, far) / far);

	// 2: normal
	// https://github.com/KhronosGroup/glTF/blob/master/specification/2.0/README.md#tangent-space-definition
	mat3 TBN = mat3(fragNormal, cross(fragNormal, fragTangent.xyz) * fragTangent.w, fragNormal);
	vec3 normal = normalize(TBN * ((texture(normalSampler, fragTexCoord).rgb) * 2.0f - vec3(1.0f)));
	normal.y *= -1; // vulkan inverted y
	outNormal   = vec4(normal, 1.0f);

	// 3: albedo
	outAlbedo   = vec4(texture(albedoSampler, fragTexCoord).rgb, fragTexCoord.x);

	// 4: ao metallic roughness
	outMetallicRoughness = vec4(texture(metallicRoughnessSampler, fragTexCoord).rgb, fragTexCoord.y);
}