#version 330 core

uniform sampler2D colorMap;

noperspective in vec2 fragUV;

layout(location = 0) out vec4 fragColor;

void main() {
	vec2 texSize = vec2(textureSize(colorMap, 0));
	vec4 result = vec4(0.0);
	for (int x = -2; x < 2; ++x)
		for (int y = -2; y < 2; ++y)
			result += texture(colorMap, fragUV + vec2(x, y) / texSize);
	fragColor = result / 16.0;
}
