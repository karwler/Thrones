#version 330 core

uniform samplerCube skyMap;

in vec3 fragUV;

out vec4 fragColor;

void main() {
	fragColor = texture(skyMap, fragUV);
}
