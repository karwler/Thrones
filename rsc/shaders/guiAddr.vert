#version 330 core

const vec2 vertices[4] = vec2[](
	vec2(0.0, 0.0),
	vec2(1.0, 0.0),
	vec2(0.0, 1.0),
	vec2(1.0, 1.0)
);

uniform vec2 pview;

layout(location = 0) in vec4 rect;
layout(location = 2) in uvec2 addr;

flat out uvec2 fragAddr;

void main() {
	fragAddr = addr;
	vec2 loc = vec2(vertices[gl_VertexID].x * rect[2] + rect.x, vertices[gl_VertexID].y * rect[3] + rect.y);
	gl_Position = vec4((loc.x - pview.x) / pview.x, -(loc.y - pview.y) / pview.y, 0.0, 1.0);
}
