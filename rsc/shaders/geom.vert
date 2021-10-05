#version 330 core

uniform mat4 proj;
uniform mat4 view;

layout (location = 0) in vec3 vpos;
layout (location = 1) in vec3 normal;
layout (location = 4) in mat4 model;
layout (location = 11) in vec4 diffuse;
layout (location = 14) in int show;

out vec3 fragPos;
out vec3 fragNormal;
flat out float fragAlpha;
flat out int fragShow;

void main() {
	mat4 vmodel = view * model;
	vec4 vloc = vmodel * vec4(vpos, 1.0);
	fragPos = vloc.xyz;
	fragNormal = transpose(inverse(mat3(vmodel))) * normal;
	fragAlpha = diffuse.a;
	fragShow = show;
	gl_Position = proj * vloc;
}
