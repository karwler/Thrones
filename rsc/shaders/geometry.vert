#version 330 core

uniform mat4 pview;
uniform mat4 model;
uniform mat3 normat;

layout (location = 0) in vec3 vpos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uvloc;

out vec3 fragPos;
out vec3 fragNormal;
out vec2 fragUV;

void main() {
	vec4 wloc = model * vec4(vpos, 1.0);
	fragPos = wloc.xyz;
	fragNormal = normat * normal;
	fragUV = uvloc;
	gl_Position = pview * wloc;
}
