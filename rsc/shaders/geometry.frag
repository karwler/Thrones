#version 130

struct Material {
	vec3 diffuse;
	vec3 specular;
	float shininess;
	float alpha;
};

struct Light {
	vec3 pos;
	vec3 ambient;
	vec3 diffuse;
	float linear;
	float quadratic;
};

uniform Material material;
uniform Light light;
uniform sampler2D texsamp;
uniform vec3 viewPos;

in vec3 fragLoc;
in vec3 fragNormal;
in vec2 fragUV;

out vec4 fragColor;

void main() {
	vec3 normal = normalize(fragNormal);
	vec3 lightDir = light.pos - fragLoc;
	float distance = length(lightDir);
	float attenuation = 1.0 / (1.0 + light.linear * distance + light.quadratic * (distance * distance));
	lightDir = normalize(lightDir);

	vec3 ambient = material.diffuse * light.ambient;
	vec3 diffuse = material.diffuse * max(dot(normal, lightDir), 0.0) * light.diffuse;
	vec3 specular = material.specular * pow(max(dot(normal, normalize(lightDir + normalize(viewPos - fragLoc))), 0.0), material.shininess);
	fragColor = texture2D(texsamp, fragUV) * vec4((ambient + diffuse + specular) * attenuation, material.alpha);
}
