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

layout (location = 0) in vec2 inUV;

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
float normalisedDistributionTRGGX(vec3 normal, vec3 halfway, float roughness) { 
	// for better results, square roughness before passing to function
	float alpha2 = roughness * roughness; 
	float NdotH = max(dot(normal, halfway), 0.0f);

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

float geometrySmith(vec3 normal, vec3 view, vec3 inLight, float roughness) {
	float NdotV = max(dot(normal, view), 0.0f);
	float NdotL = max(dot(normal, inLight), 0.0f);

	float gsggx1 = geometrySchlickGGX(NdotV, roughness);
	float gsggx2 = geometrySchlickGGX(NdotL, roughness);

	return gsggx1 * gsggx2;
}


void main() 
{   
	// values from gbuffer attachments
	vec4 fragPos = subpassLoad(samplerPosition); // no need to convert to world space thanks to image format
	vec4 cameraCoord = ubo.cameraMVP * vec4(fragPos.xyz, 1.0f); // fragment position in camera space
	vec4 shadowCoord = ubo.depthMVP * vec4(fragPos.xyz, 1.0f); // fragment position in light's space
	vec4 normal = subpassLoad(samplerNormal); // already normalised
	vec4 albedo = subpassLoad(samplerAlbedo);
	vec4 metallicRoughness = subpassLoad(samplerMetallicRoughness);
	vec2 uv = vec2(albedo.w, metallicRoughness.w);
	
	float shadow = 1.0f - computeShadow(shadowCoord);

	float ao = metallicRoughness.r;
	float metallic = metallicRoughness.b;
	float roughness = metallicRoughness.g;

	// is fragment skybox? encoded in normal's w component
	if (normal.w == 0.0f) {
		outColor = vec4(albedo.rgb, 1.0f);
		return;
	}

	switch(int(ubo.viewPos.w)) {
		// scene composition
		case 0: {
			// first check if in shadow
			if (shadow > 0.5) {
				outColor = vec4(vec3(0.0f), 1.0f);			
				break;
			}

			// Rendering equation
			
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
					toLight = normalize(toLight);

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
					float NDF = normalisedDistributionTRGGX(normal.xyz, halfway, roughness);

					// geometry term
					float G = geometrySmith(normal.xyz, viewToFrag, toLight, roughness); // multiply roughness here?

					// denominator for normalisation
					float denom = 4 * max(dot(normal.xyz, viewToFrag), 0.0f) * max(dot(normal.xyz, toLight), 0.0f);

					vec3 specular = NDF * G * F / max(denom, 0.001f);

					// fresnel value directly corresponds to the specularity of the surface
					vec3 Ks = F;
					vec3 Kd = vec3(1.0f) - Ks;
					Kd *= 1.0f - metallic; // nullify diffuse component for metallic surfaces

					// add contribution of the light
					Lo += (Kd * albedo.rgb / PI + specular) * radiance * max(dot(normal.xyz, toLight), 0.0f);

					/*
					// diffuse
					float normalDotToLight = max(0.0f, dot(normal.xyz, toLight));
					vec3 diffuse = ubo.lights[i].color * albedo.rgb * normalDotToLight * attenuation;

					// specular
					vec3 r = reflect(-toLight, normal.xyz);
					float normalDotReflect = max(0.0f, dot(r, viewToFrag));
					vec3 specular = ubo.lights[i].color * albedo.a * pow(normalDotReflect, 3.0f) * attenuation;

					fragcolor += ( 1.0f - computeShadow(shadowCoord)) * (diffuse + specular);
					*/
				}
			}
			/*
			vec3 color = ambient * albedo.rgb + Lo;
			color = color / (color + vec3(1.0f));
			//outColor = ( 1.0f - computeShadow(shadowCoord)) * vec4(Lo, 1.0f);
			outColor = vec4(color, 1.0f);
			color = pow(color, vec3(1.0f/2.0f));
			*/
			vec3 color = Lo;
			color = color / (color + vec3(1.0f));
			color = pow(color, vec3(1.0f/2.2f));
			outColor = vec4(color, 1.0f);			
			break;
		}
		// position
		case 1: 
			outColor = vec4(fragPos.xyz, 1.0f);
			break;
		// normal
		case 2:
			outColor = vec4(normal.xyz, 1.0f);
			break;
		// albedo
		case 3:
			outColor = vec4(albedo.rgb, 1.0f);
			break;
		// depth
		case 4:
			// encoded scene depth in alpha channel of position texture in previous render pass
			outColor = vec4(fragPos.www, 1.0f);
			break;
		// shadowmap value
		case 5:
			// outColor = vec4(linZ(texture(samplerShadowMap, inUV).r, NEAR, FAR));
			outColor = vec4(vec3(shadow), 1.0f);
			break;
		// position projected in light space
		case 6:
			outColor = vec4(shadowCoord.xy, 0.0f, 1.0f); // display the NDC coordinates for light
			break;
		// position projected in camera space
		case 7:
			outColor = vec4(cameraCoord.xy, 0.0f, 1.0f); // display the NDC coordinates for camera
			break;
		// depth in light space
		case 8:
			// color the scene according to the shadow map's depth, apply perspective division for correct values
			outColor = vec4(vec3(linZ(shadowCoord.z / shadowCoord.w , NEAR, FAR)), 1.0f); 
			break;
		// roughness
		case 9:
			outColor = vec4(vec3(roughness), 1.0f); 
			break;
		// metallic
		case 10:
			outColor = vec4(vec3(metallic), 1.0f); 
			break;
		// occlusion
		case 11:
			outColor = vec4(vec3(ao), 1.0f); 
			break;
		case 12:
			outColor = vec4(uv, 0.0f, 1.0f);
			break;
	}
}