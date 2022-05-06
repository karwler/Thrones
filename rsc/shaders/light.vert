#version 330 core

uniform mat4 pview;
uniform vec3 viewPos;
uniform vec3 lightPos;

layout(location = 0) in vec3 vpos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uvloc;
layout(location = 3) in vec3 tangent;
layout(location = 4) in mat4 model;
layout(location = 8) in mat3 normat;
layout(location = 11) in vec4 diffuse;
layout(location = 12) in vec4 specShine;
layout(location = 14) in uvec2 texid;
layout(location = 15) in int show;

out vec3 fragPos;
out vec2 fragUV;
out vec3 tanLightPos;
out vec3 tanViewPos;
out vec3 tanFragPos;
flat out vec4 fragDiffuse;
flat out vec4 fragSpecShine;
flat out uvec2 fragTexid;
flat out int fragShow;

void main() {
	vec4 wloc = model * vec4(vpos, 1.0);
	vec3 tang = normat * tangent;
	vec3 norm = normat * normal;
	tang = normalize(tang - dot(tang, norm) * norm);
	mat3 tbn = transpose(mat3(tang, cross(norm, tang), norm));

	fragPos = wloc.xyz;
	fragUV = uvloc;
	tanLightPos = tbn * lightPos;
	tanViewPos = tbn * viewPos;
	tanFragPos = tbn * wloc.xyz;
	fragDiffuse = diffuse;
	fragSpecShine = specShine;
	fragTexid = texid;
	fragShow = show;
	gl_Position = pview * wloc;
}
