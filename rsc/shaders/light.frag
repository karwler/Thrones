#version 330 core

struct Material {
	float ao;
	float metallic;
	float roughness;
};

const float pi = 3.14159265359;
const int samples = 20;
const vec3 gridDisk[samples] = vec3[](
	vec3(1.0, 1.0, 1.0), vec3(1.0, -1.0, 1.0), vec3(-1.0, -1.0, 1.0), vec3(-1.0, 1.0, 1.0),
	vec3(1.0, 1.0, -1.0), vec3(1.0, -1.0, -1.0), vec3(-1.0, -1.0, -1.0), vec3(-1.0, 1.0, -1.0),
	vec3(1.0, 1.0, 0.0), vec3(1.0, -1.0, 0.0), vec3(-1.0, -1.0, 0.0), vec3(-1.0, 1.0, 0.0),
	vec3(1.0, 0.0, 1.0), vec3(-1.0, 0.0, 1.0), vec3(1.0, 0.0, -1.0), vec3(-1.0, 0.0, -1.0),
	vec3(0.0, 1.0, 1.0), vec3(0.0, -1.0, 1.0), vec3(0.0, -1.0, -1.0), vec3(0.0, 1.0, -1.0)
);

const float lightRange = 140.0;
const vec3 lightColor = vec3(1.0, 0.98, 0.92) * 6.0;
const float lightLinear = 4.5 / lightRange;
const float lightQuadratic = 75.0 / (lightRange * lightRange);

uniform int optShadow;
uniform bool optSsao;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform Material materials[19];
uniform sampler2DArray colorMap;
uniform sampler2DArray normaMap;
uniform samplerCube depthMap;
uniform sampler2D ssaoMap;

in vec3 fragPos;
in vec2 fragUV;
in vec3 tanLightPos;
in vec3 tanViewPos;
in vec3 tanFragPos;
flat in vec4 fragDiffuse;
flat in uvec3 fragTexid;
flat in int fragShow;

out vec4 fragColor;

float distribution(vec3 normal, vec3 h, float roughness) {
	float a = roughness*roughness;
	float a2 = a*a;
	float NdotH = max(dot(normal, h), 0.0);
	float NdotH2 = NdotH*NdotH;

	float nom   = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = pi * denom * denom;

	return nom / denom;
}

float geometrySchlick(float ndotv, float roughness) {
	float r = (roughness + 1.0);
	float k = (r * r) / 8.0;
	return ndotv / (ndotv * (1.0 - k) + k);
}

float geometrySmith(vec3 normal, vec3 viewDir, vec3 lightDir, float roughness) {
	float ndotv = max(dot(normal, viewDir), 0.0);
	float ndotl = max(dot(normal, lightDir), 0.0);
	return geometrySchlick(ndotl, roughness) * geometrySchlick(ndotv, roughness);
}

vec3 fresnelSchlick(float cosTheta, vec3 f0) {
	return f0 + (1.0 - f0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float calcShadowHard() {
	vec3 fragToLight = fragPos - lightPos;
	return length(fragToLight) - 0.05 > texture(depthMap, fragToLight).r * lightRange ? 0.0 : 1.0;
}

float calcShadowSoft() {
	vec3 fragToLight = fragPos - lightPos;
	float currentDepth = length(fragToLight) - 0.15;
	float diskRadius = (1.0 + length(viewPos - fragPos) / lightRange) / 25.0;
	float shadow = 0.0;
	for (int i = 0; i < samples; ++i)
		if (currentDepth > texture(depthMap, fragToLight + gridDisk[i] * diskRadius).r * lightRange)
			shadow += 1.0;
	return 1.0 - (shadow / float(samples));
}

void main() {
	if (!bool(fragShow)) {
		discard;
		return;
	}

	Material mtl = materials[fragTexid.z];
	vec4 color = texture(colorMap, vec3(fragUV, fragTexid.x)) * fragDiffuse;
	vec3 normal = normalize(texture(normaMap, vec3(fragUV, fragTexid.y)).xyz * 2.0 - 1.0);
	vec3 lightDir = tanLightPos - tanFragPos;
	float lightDist = length(lightDir);
	vec3 viewDir = normalize(tanViewPos - tanFragPos);
	lightDir = normalize(lightDir);

	vec3 f0 = mix(vec3(0.04), color.rgb, mtl.metallic);
	vec3 h = normalize(viewDir + lightDir);
	float attenuation = 1.0 / (1.0 + lightLinear * lightDist + lightQuadratic * (lightDist * lightDist));
	vec3 radiance = lightColor * attenuation;
	float ndf = distribution(normal, h, mtl.roughness);
	float g = geometrySmith(normal, viewDir, lightDir, mtl.roughness);
	vec3 f = fresnelSchlick(max(dot(h, viewDir), 0.0), f0);
	vec3 numerator = ndf * g * f;
	float denominator = 4.0 * max(dot(normal, viewDir), 0.0) * max(dot(normal, lightDir), 0.0) + 0.0001;
	vec3 specular = numerator / denominator;
	vec3 ks = f;
	vec3 kd = (vec3(1.0) - ks) * (1.0 - mtl.metallic);
	float ndotl = max(dot(normal, lightDir), 0.0);
	vec3 lo = (kd * color.rgb / pi + specular) * radiance * ndotl;
	vec3 ambient = vec3(0.03) * color.rgb * mtl.ao;

	float shadow = optShadow == 2 ? calcShadowSoft() : optShadow == 1 ? calcShadowHard() : 1.0;
	float ssao = optSsao ? texture(ssaoMap, gl_FragCoord.xy / vec2(textureSize(ssaoMap, 0))).r : 1.0;
	fragColor = vec4((ambient + lo * shadow) * ssao, color.a);
}
