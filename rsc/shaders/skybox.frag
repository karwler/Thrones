#version 330 core

uniform samplerCube skyMap;

in vec3 fragUV;

layout (location = 0) out vec4 fragColor;
layout (location = 1) out vec4 brightColor;

void main() {
	fragColor = texture(skyMap, fragUV);
	brightColor = vec4(0.0, 0.0, 0.0, 1.0);
}
