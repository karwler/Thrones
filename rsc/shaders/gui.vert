#version 330 core

const vec2 vertices[4] = vec2[](
	vec2(0.0, 0.0),
	vec2(1.0, 0.0),
	vec2(0.0, 1.0),
	vec2(1.0, 1.0)
);

uniform vec2 pview;

layout(location = 0) in vec4 rect;
layout(location = 1) in vec4 uvrc;
layout(location = 11) in vec4 diffuse;
layout(location = 14) in uint texid;

noperspective out vec2 fragUV;
flat out vec4 fragDiffuse;
flat out uint fragTexid;

void main() {
	vec2 loc = vec2(vertices[gl_VertexID].x * rect[2] + rect.x, vertices[gl_VertexID].y * rect[3] + rect.y);
	fragUV = vec2(vertices[gl_VertexID].x * uvrc[2] + uvrc.x, vertices[gl_VertexID].y * uvrc[3] + uvrc.y);
	fragDiffuse = diffuse;
	fragTexid = texid;
	gl_Position = vec4((loc.x - pview.x) / pview.x, -(loc.y - pview.y) / pview.y, 0.0, 1.0);
}
