#version 130

uniform mat4 pview;
uniform mat4 model;
uniform mat3 normat;

in vec3 vertex;
in vec3 normal;
in vec2 uvloc;

out vec3 fragLoc;
out vec3 fragNormal;
out vec2 fragUV;

void main() {
	vec4 wloc = model * vec4(vertex, 1.0);
	fragLoc = vec3(wloc);
	fragNormal = normat * normal;
	fragUV = uvloc;
	gl_Position = pview * wloc;
}
