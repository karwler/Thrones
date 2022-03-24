#version 330 core

uniform vec2 pview;

layout (location = 0) in vec2 vpos;
layout (location = 1) in vec4 rect;
layout (location = 2) in vec4 uvrc;
layout (location = 3) in float zloc;
layout (location = 11) in vec4 diffuse;
layout (location = 13) in uint texid;

out vec2 fragUV;
flat out vec4 fragDiffuse;
flat out uint fragTexid;

void main() {
	vec2 loc = vec2(vpos.x * rect[2] + rect.x, vpos.y * rect[3] + rect.y);
	fragUV = vec2(vpos.x * uvrc[2] + uvrc.x, vpos.y * uvrc[3] + uvrc.y);
	fragDiffuse = diffuse;
	fragTexid = texid;
	gl_Position = vec4((loc.x - pview.x) / pview.x, -(loc.y - pview.y) / pview.y, zloc, 1.0);
}
