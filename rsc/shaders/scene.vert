#version 130

uniform mat4 pview;
uniform mat4 trans;
uniform mat4 rotscl;
in vec3 vertex;
in vec2 uvloc;
in vec3 normal;
out vec2 fragUV;
out vec3 fragLoc;
out vec3 fragNormal;

void main() {
	vec4 wloc = trans * vec4(vertex, 1);
	gl_Position = pview * wloc;

	fragUV = uvloc;
	fragLoc = vec3(wloc);
	fragNormal = vec3(rotscl * vec4(normal, 1));
}
