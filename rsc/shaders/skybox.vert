#version 330 core

uniform mat4 pview;
uniform vec3 viewPos;

layout (location = 0) in vec3 vpos;

out vec3 fragUV;

void main() {
	fragUV = vpos;
	gl_Position = vec4(pview * vec4(vpos + viewPos, 1.0)).xyww;
}
