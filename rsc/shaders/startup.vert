#version 330 core

const vec2 vertices[4] = vec2[](
	vec2(0.0, 0.0),
	vec2(1.0, 0.0),
	vec2(0.0, 1.0),
	vec2(1.0, 1.0)
);

uniform vec2 pview;
uniform vec4 rect;

noperspective out vec2 fragUV;

void main() {
	fragUV = vertices[gl_VertexID];
	gl_Position = vec4((vertices[gl_VertexID].x * rect[2] + rect.x - pview.x) / pview.x, -(vertices[gl_VertexID].y * rect[3] + rect.y - pview.y) / pview.y, 0.0, 1.0);
}
