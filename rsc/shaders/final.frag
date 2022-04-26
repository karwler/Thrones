#version 330 core

uniform float gamma;
uniform sampler2D sceneMap;
uniform sampler2D bloomMap;

in vec2 fragUV;

out vec4 fragColor;

void main() {
	fragColor = vec4(pow(texture(sceneMap, fragUV).rgb + texture(bloomMap, fragUV).rgb, vec3(gamma)), 1.0);
}
