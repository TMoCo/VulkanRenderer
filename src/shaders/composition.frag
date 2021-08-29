#version 450

/*
layout (binding = 1) uniform sampler2DShadow samplerShadowMap;
*/
struct Light {
	vec4 position;
	vec3 color;
	float radius;	
};

layout(binding = 0, std140) uniform UniformBufferObject {
	vec4 viewPos;
	mat4 depthMVP;
	mat4 cameraMVP;
	Light[1] lights;
} ubo;

layout (input_attachment_index = 0, set = 0, binding = 1) uniform subpassInput samplerPosition;
layout (input_attachment_index = 1, set = 0, binding = 2) uniform subpassInput samplerNormal;
layout (input_attachment_index = 2, set = 0, binding = 3) uniform subpassInput samplerAlbedo;
layout (input_attachment_index = 3, set = 0, binding = 4) uniform subpassInput samplerAOMetallicRoughness;


layout(location = 0) out vec4 outColor;

#define NEAR 0.1f
#define FAR 40.0f

const vec2 poissonDisk[4] = vec2[](
  vec2( -0.94201624, -0.39906216 ),
  vec2( 0.94558609, -0.76890725 ),
  vec2( -0.094184101, -0.92938870 ),
  vec2( 0.34495938, 0.29387760 )
);

#define PI 3.1415927410125732421875f

// given a scene coordinate transformed by a light's MVP matrix, perform perspective division to get the NDC coordinates
// from the light's view point 
float computeShadow(vec4 shadowCoord) {
	vec3 shadowNDC = shadowCoord.xyz / shadowCoord.w; // perspective division
	shadowNDC.xy = shadowNDC.xy * 0.5f + 0.5f; // mapping from [-1,1] to [0,1] for sampling the shadow map
	float shadow = 0.0f;
	for (int i = 0; i < 4; i++) {
		//shadow += 0.25f * texture(samplerShadowMap, vec3(shadowNDC.xy + poissonDisk[i]/600.0, shadowNDC.z));
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
	return alpha2 / max(denom, 0.001f);
}

float geometrySchlickGGX(float NdotV, float roughness) {
	roughness = roughness + 1.0f;
	float k = roughness * roughness / 8.0f; // for direct lighting
	// float k = roughness / 2.0f; // for image based lighting

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
	vec3 albedo = pow(subpassLoad(samplerAlbedo).rgb, vec3(2.2f));
	vec4 aoMetallicRoughness = subpassLoad(samplerAOMetallicRoughness);
	
	//float shadow = (1.0f - computeShadow(ubo.depthMVP * vec4(fragPos.xyz, 1.0f)));

	float ao = aoMetallicRoughness.r;
	float roughness = aoMetallicRoughness.g;
	float metallic = aoMetallicRoughness.b;

	// is fragment skybox? encoded in normal's w component
	if (normal.w == 0.0f) {
		outColor = vec4(albedo.rgb, 1.0f);
		return;
	}

	// Rendering equation:			
	// integrate over all incoming sources of light
	// Lo += BRDF * radiance[i] * cosTheta

	// initialise Lo in rendering equation to 0 vector
	vec3 Lo = vec3(0.0f);

	// for dieletcric surfaces, surface reflection is 4%
	vec3 F0 = vec3(0.04f);
	F0 = mix(F0, albedo, metallic);

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
			vec3 F = fresnelSchlick(F0, clamp(dot(halfway, viewToFrag), 0.0f, 1.0f));

			// normalised distribution function term
			float NDF = normalisedDistributionTRGGX(max(dot(halfway, normal.xyz), 0.0f), roughness * roughness);

			float NdotV = max(dot(normal.xyz, viewToFrag), 0.0f);
			float NdotL = max(dot(normal.xyz, toLight), 0.0f);

			// geometry term
			float G = geometrySmith(NdotV, NdotL, roughness); // multiply roughness here?

			// divide by normalisation factor
			vec3 specular = F * NDF * G / max(4.0f * NdotV * NdotL, 0.001f);

			// fresnel value directly corresponds to the specularity of the surface
			vec3 Kd = (vec3(1.0f) - F) * (1.0f - metallic); // nullify diffuse component for metallic surfaces

			// add contribution of the light
			Lo += (Kd * albedo / PI + specular) * radiance * NdotL;
		}
	}

	vec3 color = vec3(0.03) * albedo * ao + Lo; // multiply by some ambient term
	outColor = vec4(pow (color / (color + vec3(1.0f)), vec3(1.0/2.2)), 1.0f);
}