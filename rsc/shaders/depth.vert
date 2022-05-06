#version 330 core

uniform mat4 pvTrans[6];
uniform uint pvId;

layout(location = 0) in vec3 vpos;
layout(location = 4) in mat4 model;
layout(location = 11) in vec4 diffuse;
layout(location = 15) in int show;

out vec3 fragPos;
flat out float fragAlpha;
flat out int fragShow;

void main() {
	vec4 wloc = model * vec4(vpos, 1.0);
	fragPos = wloc.xyz;
	fragAlpha = diffuse.a;
	fragShow = show;
	gl_Position = pvTrans[pvId] * wloc;
}
