#version 330 core

uniform vec2 pview;
uniform vec4 rect;

layout (location = 0) in vec2 vpos;

out vec2 fragUV;

void main() {
	fragUV = vpos;
	gl_Position = vec4((vpos.x * rect[2] + rect.x - pview.x) / pview.x, -(vpos.y * rect[3] + rect.y - pview.y) / pview.y, 0.0, 1.0);
}
