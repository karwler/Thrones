#version 330 core

const int samples = 20;
const vec3 gridDisk[samples] = vec3[](
	vec3(1.0, 1.0, 1.0), vec3(1.0, -1.0, 1.0), vec3(-1.0, -1.0, 1.0), vec3(-1.0, 1.0, 1.0),
	vec3(1.0, 1.0, -1.0), vec3(1.0, -1.0, -1.0), vec3(-1.0, -1.0, -1.0), vec3(-1.0, 1.0, -1.0),
	vec3(1.0, 1.0, 0.0), vec3(1.0, -1.0, 0.0), vec3(-1.0, -1.0, 0.0), vec3(-1.0, 1.0, 0.0),
	vec3(1.0, 0.0, 1.0), vec3(-1.0, 0.0, 1.0), vec3(1.0, 0.0, -1.0), vec3(-1.0, 0.0, -1.0),
	vec3(0.0, 1.0, 1.0), vec3(0.0, -1.0, 1.0), vec3(0.0, -1.0, -1.0), vec3(0.0, 1.0, -1.0)
);

uniform struct {
	vec4 diffuse;
	vec3 specular;
	float shininess;
} material;

uniform struct {
	vec3 pos;
	vec3 ambient;
	vec3 diffuse;
	float linear;
	float quadratic;
} light;

uniform vec3 viewPos;
uniform float farPlane;
uniform sampler2D texsamp;
uniform samplerCube depthMap;

in vec3 fragPos;
in vec3 fragNormal;
in vec2 fragUV;

out vec4 fragColor;

float calcShadowHard() {
	vec3 fragToLight = fragPos - light.pos;
	return length(fragToLight) -  0.05 > texture(depthMap, fragToLight).r * farPlane ? 0.0 : 1.0;
}

float calcShadowSoft() {
	vec3 fragToLight = fragPos - light.pos;
	float currentDepth = length(fragToLight) - 0.15;
	float diskRadius = (1.0 + length(viewPos - fragPos) / farPlane) / 25.0;
	float shadow = 0.0;
	for (int i = 0; i < samples; i++)
		if (currentDepth > texture(depthMap, fragToLight + gridDisk[i] * diskRadius).r * farPlane)
			shadow += 1.0;
	return 1.0 - (shadow / float(samples));
}

void main() {
	vec3 normal = normalize(fragNormal);
	vec3 lightDir = light.pos - fragPos;
	float lightDist = length(lightDir);
	float attenuation = 1.0 / (1.0 + light.linear * lightDist + light.quadratic * (lightDist * lightDist));
	lightDir = normalize(lightDir);

	vec4 color = texture(texsamp, fragUV) * material.diffuse;
	vec3 ambient = color.rgb * light.ambient;
	vec3 diffuse = color.rgb * light.diffuse * max(dot(normal, lightDir), 0.0);
	vec3 specular = material.specular * pow(max(dot(normal, normalize(lightDir + normalize(viewPos - fragPos))), 0.0), material.shininess);
	float shadow = 1.0;	// appropiate function is automatically placed during startup
	fragColor = vec4((ambient + (diffuse + specular) * shadow) * attenuation, color.a);
}
