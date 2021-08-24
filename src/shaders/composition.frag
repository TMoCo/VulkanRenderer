#version 450

layout (binding = 1) uniform sampler2DShadow samplerShadowMap;
layout (input_attachment_index = 0, set = 0, binding = 4) uniform subpassInput samplerPosition;
layout (input_attachment_index = 1, set = 0, binding = 5) uniform subpassInput samplerNormal;
layout (input_attachment_index = 2, set = 0, binding = 6) uniform subpassInput samplerAlbedo;
layout (input_attachment_index = 3, set = 0, binding = 7) uniform subpassInput samplerMetallicRoughness;

struct Light {
	vec4 position;
	vec3 color;
	float radius;	
};

layout(binding = 3, std140) uniform UniformBufferObject {
	vec4 viewPos;
	mat4 depthMVP;
	mat4 cameraMVP;
	Light[1] lights;
} ubo;

// layout (location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

const vec3 ambient = vec3(0.2f, 0.2f, 0.2f);

#define NEAR 0.1f
#define FAR 40.0f

#define PI 3.1415927410125732421875f

const vec2 poissonDisk[4] = vec2[](
  vec2( -0.94201624, -0.39906216 ),
  vec2( 0.94558609, -0.76890725 ),
  vec2( -0.094184101, -0.92938870 ),
  vec2( 0.34495938, 0.29387760 )
);

// compute linear depth, taken from:
// https://learnopengl.com/Advanced-OpenGL/Depth-testing
// (formula has been factorised)
float linZ (float z, float n, float f) {
	return n / (f - f*z + n*z);
}

// given a scene coordinate transformed by a light's MVP matrix, perform perspective division to get the NDC coordinates
// from the light's view point 
float computeShadow(vec4 shadowCoord) {
	vec3 shadowNDC = shadowCoord.xyz / shadowCoord.w; // perspective division
	shadowNDC.xy = shadowNDC.xy * 0.5f + 0.5f; // mapping from [-1,1] to [0,1] for sampling the shadow map
	float shadow = 0.0f;
	for (int i = 0; i < 4; i++) {
		shadow += 0.25f * texture(samplerShadowMap, vec3(shadowNDC.xy + poissonDisk[i]/600.0, shadowNDC.z));
	}
	return shadow; // if fragment depth greater than depth to occluder, then fragment is in shadow
}

// costheta = dot(n,h)
vec3 fresnelSchlick(vec3 F0, float cosTheta) {
	return F0 + (1.0f - F0) * pow(max(1.0f - cosTheta, 0.0f), 5.0f);
}

// normalised distribution function (Trowbridge-Reitz GGX)
float normalisedDistributionTRGGX(float NdotH, float roughness) { 
	// for better results, square roughness before passing to function
	float alpha2 = roughness * roughness; 
	float denom = NdotH * NdotH * (alpha2 - 1.0f) + 1.0f;
	denom = denom * denom * PI;
	return alpha2 / denom;
}

float geometrySchlickGGX(float NdotV, float roughness) {
	float alpha = roughness + 1.0f;
	float k = alpha * alpha / 8.0f; // for direct lighting
	// float k = roughness * roughness / 2.0f; // for image based lighting

	return NdotV / (NdotV * (1.0f - k) + k);
}

float geometrySmith(float NdotV, float NdotL, float roughness) {
	return geometrySchlickGGX(NdotV, roughness) *  geometrySchlickGGX(NdotL, roughness);
}


void main() 
{   
	// values from gbuffer attachments
	vec4 fragPos = subpassLoad(samplerPosition); // no need to convert to world space thanks to image format
	vec4 normal = subpassLoad(samplerNormal); // already normalised
	vec4 albedo = subpassLoad(samplerAlbedo);
	vec4 metallicRoughness = subpassLoad(samplerMetallicRoughness);
	
	float shadow = (1.0f - computeShadow(ubo.depthMVP * vec4(fragPos.xyz, 1.0f)));

	float ao = metallicRoughness.r;
	float metallic = metallicRoughness.b;
	float roughness = metallicRoughness.g;

	// is fragment skybox? encoded in normal's w component
	if (normal.w == 0.0f) {
		outColor = vec4(albedo.rgb, 1.0f);
		return;
	}
	// first check if in shadow
	if (shadow > 0.5) {
		outColor = vec4(vec3(0.0f), 1.0f);
		return;
	}

	// Rendering equation:			
	// integrate over all incoming sources of light
	// Lo += BRDF * radiance[i] * cosTheta

	// initialise Lo in rendering equation to 0 vector
	vec3 Lo = vec3(0.0f);

	// for dieletcric surfaces, surface reflection is 4%
	vec3 F0 = vec3(0.04f);
	F0 = mix(F0, albedo.rgb, metallic);

	// direction to frag from viewer
	vec3 viewToFrag = normalize(ubo.viewPos.xyz - fragPos.xyz);

	for (int i = 0; i < 1; i++) {
		// vector to light
		vec3 toLight = ubo.lights[i].position.xyz - fragPos.xyz;
		// distance from fragment to light
		float distToLight = length(toLight);

		// test if fragment is in light's radius
		if (distToLight < ubo.lights[i].radius) {

			// direction from fragment to light
			toLight = toLight / distToLight;

			// halfway direction
			vec3 halfway = normalize(viewToFrag + toLight);

			// compute radiance -------------------------------------------------------------
					
			// light attenuation
			float attenuation = 1.0f / (distToLight * distToLight);

			vec3 radiance = ubo.lights[i].color * attenuation;

			// compute BRDF -----------------------------------------------------------------
			// using the cook torrance specular BRDF 
								
			// fresnel term
			vec3 F = fresnelSchlick(F0, max(dot(halfway, viewToFrag), 0.0f));

			// normalised distribution function term
			float NDF = normalisedDistributionTRGGX(max(dot(normal.xyz, halfway), 0.0f), roughness);

			float NdotV = max(dot(normal.xyz, viewToFrag), 0.0f);
			float NdotL = max(dot(normal.xyz, toLight), 0.0f);

			// geometry term
			float G = geometrySmith(NdotV, NdotL, roughness); // multiply roughness here?

			// divide by normalisation factor
			vec3 specular = NDF * G * F / max(4 * NdotV * NdotL, 0.001f);

			// fresnel value directly corresponds to the specularity of the surface
			vec3 Kd = (vec3(1.0f) - F) * (1.0f - metallic); // nullify diffuse component for metallic surfaces

			// add contribution of the light
			Lo += (Kd * albedo.rgb / PI + specular) * radiance * NdotL;
		}
	}

	vec3 color = vec3(0.03) * albedo.rgb * ao + Lo; // multiply by some ambient term
	outColor = vec4(pow (color / (color + vec3(1.0f)), vec3(0.45454545f)), 1.0f);

	// (1.0f - computeShadow(ubo.depthMVP * vec4(fragPos.xyz, 1.0f)))
}