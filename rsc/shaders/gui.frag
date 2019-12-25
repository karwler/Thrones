#version 330 core

uniform vec4 color;
uniform sampler2D texsamp;

in vec2 fragUV;

out vec4 fragColor;

void main() {
	fragColor = texture(texsamp, fragUV) * color;
}
