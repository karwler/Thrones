#version 330 core

struct Material {
	float reflect;
	float rough;
};

const vec3 toLuma = vec3(0.299, 0.587, 0.114);
const float lumaThreshold = 0.63;		// [0, 1]
const float mulReduce = 1.0 / 8.0;		// [1, 256]
const float minReduce = 1.0 / 128.0;	// [1, 512]
const float maxSpan = 8.0;				// [1, 16]

uniform bool optFxaa;
uniform bool optSsr;
uniform bool optBloom;
uniform float gamma;
uniform Material materials[19];
uniform sampler2D sceneMap;
uniform sampler2D bloomMap;
uniform sampler2D ssrCleanMap;
uniform sampler2D ssrBlurMap;
uniform usampler2D matlMap;

noperspective in vec2 fragUV;

out vec4 fragColor;

vec3 fxaa(vec3 rgbM) {
	float lumaNW = dot(textureOffset(sceneMap, fragUV, ivec2(-1, 1)).rgb, toLuma);
	float lumaNE = dot(textureOffset(sceneMap, fragUV, ivec2(1, 1)).rgb, toLuma);
	float lumaSW = dot(textureOffset(sceneMap, fragUV, ivec2(-1, -1)).rgb, toLuma);
	float lumaSE = dot(textureOffset(sceneMap, fragUV, ivec2(1, -1)).rgb, toLuma);
	float lumaM = dot(rgbM, toLuma);
	float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
	float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
	if (lumaMax - lumaMin <= lumaMax * lumaThreshold)
		return rgbM;

	vec2 samplingDirection = vec2(-((lumaNW + lumaNE) - (lumaSW + lumaSE)), ((lumaNW + lumaSW) - (lumaNE + lumaSE)));
    float samplingDirectionReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * 0.25 * mulReduce, minReduce);
	float minSamplingDirectionFactor = 1.0 / (min(abs(samplingDirection.x), abs(samplingDirection.y)) + samplingDirectionReduce);
    samplingDirection = clamp(samplingDirection * minSamplingDirectionFactor, vec2(-maxSpan), vec2(maxSpan)) / vec2(textureSize(sceneMap, 0));

	vec3 rgbSampleNeg = texture(sceneMap, fragUV + samplingDirection * (1.0 / 3.0 - 0.5)).rgb;
	vec3 rgbSamplePos = texture(sceneMap, fragUV + samplingDirection * (2.0 / 3.0 - 0.5)).rgb;
	vec3 rgbTwoTab = (rgbSamplePos + rgbSampleNeg) * 0.5;
	vec3 rgbSampleNegOuter = texture(sceneMap, fragUV - samplingDirection * 0.5).rgb;
	vec3 rgbSamplePosOuter = texture(sceneMap, fragUV + samplingDirection * 0.5).rgb;
	vec3 rgbFourTab = (rgbSamplePosOuter + rgbSampleNegOuter) * 0.25 + rgbTwoTab * 0.5;
	float lumaFourTab = dot(rgbFourTab, toLuma);
	return lumaFourTab < lumaMin || lumaFourTab > lumaMax ? rgbTwoTab : rgbFourTab;
}

void main() {
	vec4 base = texture(sceneMap, fragUV);
	vec3 color = optFxaa ? fxaa(base.rgb) : base.rgb;
	if (optSsr) {
		uint mi = texture(matlMap, fragUV).r;
		color += mix(texture(ssrCleanMap, fragUV).rgb, texture(ssrBlurMap, fragUV).rgb, materials[mi].rough) * materials[mi].reflect;
	}
	if (optBloom)
		color += texture(bloomMap, fragUV).rgb;
	fragColor = vec4(pow(color, vec3(gamma)), base.a);
}
