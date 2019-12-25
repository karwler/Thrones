#version 330 core

uniform vec3 lightPos;
uniform float farPlane;

in vec4 fragPos;

void main() {
	gl_FragDepth = length(fragPos.xyz - lightPos) / farPlane;
}
