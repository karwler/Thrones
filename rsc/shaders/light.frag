#version 330 core

const int samples = 20;
const vec3 gridDisk[samples] = vec3[](
    vec3(1.0, 1.0, 1.0), vec3(1.0, -1.0, 1.0), vec3(-1.0, -1.0, 1.0), vec3(-1.0, 1.0, 1.0),
    vec3(1.0, 1.0, -1.0), vec3(1.0, -1.0, -1.0), vec3(-1.0, -1.0, -1.0), vec3(-1.0, 1.0, -1.0),
    vec3(1.0, 1.0, 0.0), vec3(1.0, -1.0, 0.0), vec3(-1.0, -1.0, 0.0), vec3(-1.0, 1.0, 0.0),
    vec3(1.0, 0.0, 1.0), vec3(-1.0, 0.0, 1.0), vec3(1.0, 0.0, -1.0), vec3(-1.0, 0.0, -1.0),
    vec3(0.0, 1.0, 1.0), vec3(0.0, -1.0, 1.0), vec3(0.0, -1.0, -1.0), vec3(0.0, 1.0, -1.0)
);

uniform vec3 lightPos;
uniform vec3 lightAmbient;
uniform vec3 lightDiffuse;
uniform float lightLinear;
uniform float lightQuadratic;

uniform vec2 screenSize;
uniform vec3 viewPos;
uniform float farPlane;
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
flat in vec4 fragSpecShine;
flat in int fragTexid;
flat in int fragShow;

out vec4 fragColor;

float calcShadowHard() {
	vec3 fragToLight = fragPos - lightPos;
	return length(fragToLight) - 0.05 > texture(depthMap, fragToLight).r * farPlane ? 0.0 : 1.0;
}

float calcShadowSoft() {
	vec3 fragToLight = fragPos - lightPos;
	float currentDepth = length(fragToLight) - 0.15;
	float diskRadius = (1.0 + length(viewPos - fragPos) / farPlane) / 25.0;
	float shadow = 0.0;
	for (int i = 0; i < samples; ++i)
		if (currentDepth > texture(depthMap, fragToLight + gridDisk[i] * diskRadius).r * farPlane)
			shadow += 1.0;
	return 1.0 - (shadow / float(samples));
}

float getSsao() {
	return texture(ssaoMap, gl_FragCoord.xy / screenSize).r;
}

void main() {
	if (!bool(fragShow)) {
		discard;
		return;
	}

	vec3 normal = normalize(texture(normaMap, vec3(fragUV, fragTexid)).xyz * 2.0 - 1.0);
	vec3 lightDir = tanLightPos - tanFragPos;
	float lightDist = length(lightDir);
	float attenuation = 1.0 + lightLinear * lightDist + lightQuadratic * (lightDist * lightDist);
	vec3 viewDir = normalize(tanViewPos - tanFragPos);
	lightDir = normalize(lightDir);

	vec4 color = texture(colorMap, vec3(fragUV, fragTexid)) * fragDiffuse;
	vec3 ambient = color.rgb * lightAmbient;
	vec3 diffuse = color.rgb * lightDiffuse * max(dot(normal, lightDir), 0.0);
	vec3 specular = fragSpecShine.rgb * pow(max(dot(normal, normalize(lightDir + viewDir)), 0.0), fragSpecShine.a);
	float shadow = 1.0;	// appropiate function is automatically placed during startup
	float ssao = 1.0;	// ^
	fragColor = vec4((ambient + (diffuse + specular) * shadow) / attenuation * ssao, color.a);
}
