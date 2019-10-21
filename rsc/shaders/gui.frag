#version 130

uniform vec4 color;
uniform sampler2D texsamp;

in vec2 textureUV;
out vec4 fragColor;

void main() {
	fragColor = texture2D(texsamp, textureUV) * color;
}
