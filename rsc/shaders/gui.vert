#version 130

uniform vec2 pview;
uniform vec4 rect;
uniform vec4 uvrc;
uniform float zloc;

in vec2 vertex;
in vec2 uvloc;
out vec2 textureUV;

void main() {
	vec2 loc = vec2(vertex.x * rect[2] + rect.x, vertex.y * rect[3] + rect.y);
	textureUV = vec2(uvloc.x * uvrc[2] + uvrc.x, uvloc.y * uvrc[3] + uvrc.y);
	gl_Position = vec4((loc.x - pview.x) / pview.x, -(loc.y - pview.y) / pview.y, zloc, 1);
}
