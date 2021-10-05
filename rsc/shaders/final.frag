#version 330 core

uniform sampler2D sceneMap;
uniform sampler2D bloomMap;

in vec2 fragUV;

out vec4 fragColor;

void main() {
	fragColor = vec4(texture(sceneMap, fragUV).rgb + texture(bloomMap, fragUV).rgb, 1.0);
}
