#version 130

uniform vec4 color;
uniform sampler2D texsamp;
in vec2 textureUV;

void main() {
	gl_FragColor = texture2D(texsamp, textureUV) * color;
}
