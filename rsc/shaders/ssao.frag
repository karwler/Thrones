#version 330 core

const int kernelSize = 64;
const float radius = 0.5;

uniform mat4 proj;
uniform vec2 noiseScale;
uniform vec3 samples[kernelSize];
uniform sampler2D vposMap;
uniform sampler2D normMap;
uniform sampler2D noiseMap;

in vec2 fragUV;

layout (location = 0) out float fragColor;

void main() {
	vec3 fragPos = texture(vposMap, fragUV).xyz;
	vec3 normal = texture(normMap, fragUV).xyz;
	vec3 randomVec = texture(noiseMap, fragUV * noiseScale).xyz;
	vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
	mat3 tbn = mat3(tangent, cross(normal, tangent), normal);
	float occlusion = 0.0;
	for (int i = 0; i < kernelSize; ++i) {
		vec3 samplePos = fragPos + tbn * samples[i] * radius;
		vec4 offset = proj * vec4(samplePos, 1.0);
		offset.xyz = offset.xyz / offset.w * 0.5 + 0.5;

		float sampleDepth = texture(vposMap, offset.xy).z;
		if (sampleDepth >= samplePos.z + 0.025)
			occlusion += smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
	}
	fragColor = 1.0 - (occlusion / float(kernelSize));
}
