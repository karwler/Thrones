#version 130

struct Material {
	vec3 diffuse;
	vec3 emission;
	vec3 specular;
	float shininess;
	float alpha;
};

struct Light {
	vec3 pos;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float linear;
	float quadratic;
};

uniform Material material;
uniform Light light;
uniform sampler2D texsamp;
uniform vec3 viewPos;
in vec2 fragUV;
in vec3 fragLoc;
in vec3 fragNormal;

void main() {
	vec3 normal = normalize(fragNormal);
	vec3 lightDir = light.pos - fragLoc;
	float distance = length(lightDir);
	float attenuation = 1 / (1 + light.linear * distance + light.quadratic * (distance * distance));
	lightDir = normalize(lightDir);

	vec3 ambient = material.diffuse * light.ambient;
	vec3 diffuse = material.diffuse * max(dot(normal, lightDir), 0) * light.diffuse;
	vec3 specular = material.specular * pow(max(dot(normalize(viewPos - fragLoc), reflect(-lightDir, normal)), 0.0), material.shininess) * light.specular;
	gl_FragColor = texture2D(texsamp, fragUV) * vec4(material.emission + (ambient + diffuse + specular) * attenuation, material.alpha);
}
