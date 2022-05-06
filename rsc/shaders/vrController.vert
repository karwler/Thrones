#version 410 core

uniform mat4 matrix;

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 v3ColorIn;

out vec4 v4Color;

void main() {
	v4Color.xyz = v3ColorIn; v4Color.a = 1.0;
	gl_Position = matrix * position;
}
