#version 330 core

uniform sampler2D colorMap;

noperspective in vec2 fragUV;

out vec4 fragColor;

void main() {
	vec4 color = texture(colorMap, fragUV);
	fragColor = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722)) < 0.9 ? vec4(0.0, 0.0, 0.0, color.a) : color;
}
