#version 330 core

uniform vec2 pview;
uniform vec4 rect;
uniform vec4 uvrc;
uniform float zloc;

layout (location = 0) in vec2 vpos;
layout (location = 2) in vec2 uvloc;

out vec2 fragUV;

void main() {
	vec2 loc = vec2(vpos.x * rect[2] + rect.x, vpos.y * rect[3] + rect.y);
	fragUV = vec2(uvloc.x * uvrc[2] + uvrc.x, uvloc.y * uvrc[3] + uvrc.y);
	gl_Position = vec4((loc.x - pview.x) / pview.x, -(loc.y - pview.y) / pview.y, zloc, 1.0);
}
