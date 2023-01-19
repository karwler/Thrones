#version 330 core

uniform mat4 proj;
uniform mat4 view;

layout(location = 0) in vec3 vpos;
layout(location = 1) in vec3 normal;
layout(location = 4) in mat4 model;
layout(location = 11) in vec4 diffuse;
layout(location = 12) in uvec3 texid;
layout(location = 13) in int show;

out vec4 fragPos;
out vec3 fragNormal;
flat out float fragAlpha;
flat out uint fragMatl;
flat out int fragShow;

void main() {
	mat4 vmodel = view * model;
	vec4 vloc = vmodel * vec4(vpos, 1.0);
	fragPos = vloc;
	fragNormal = transpose(inverse(mat3(vmodel))) * normal;
	fragAlpha = diffuse.a;
	fragMatl = texid.z;
	fragShow = show;
	gl_Position = proj * vloc;
}
