#version 330 core

uniform sampler2D colorMap;

noperspective in vec2 fragUV;

out vec4 fragColor;

void main() {
	fragColor = texture(colorMap, fragUV);
}
