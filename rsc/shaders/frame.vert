#version 330 core

layout (location = 0) in vec2 vpos;
layout (location = 2) in vec2 uvloc;

out vec2 fragUV;

void main() {
	fragUV = uvloc;
	gl_Position = vec4(vpos, 0.0, 1.0);
}
