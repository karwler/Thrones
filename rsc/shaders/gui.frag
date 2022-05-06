#version 330 core

uniform sampler2DArray colorMap;

noperspective in vec2 fragUV;
flat in vec4 fragDiffuse;
flat in uint fragTexid;

out vec4 fragColor;

void main() {
	fragColor = texture(colorMap, vec3(fragUV, fragTexid)) * fragDiffuse;
}
