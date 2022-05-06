#version 330 core

const vec2 vertices[4] = vec2[](
	vec2(-1.0,  1.0),
	vec2(1.0,  1.0),
	vec2(-1.0, -1.0),
	vec2(1.0, -1.0)
);

const vec2 tuvs[4] = vec2[](
	vec2(0.0, 1.0),
	vec2(1.0, 1.0),
	vec2(0.0, 0.0),
	vec2(1.0, 0.0)
);

noperspective out vec2 fragUV;

void main() {
	fragUV = tuvs[gl_VertexID];
	gl_Position = vec4(vertices[gl_VertexID], 0.0, 1.0);
}
