#version 330 core

in vec4 fragPos;
in vec3 fragNormal;
flat in float fragAlpha;
flat in vec2 fragReflRough;
flat in int fragShow;

layout(location = 0) out vec4 outPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outMatl;

void main() {
	if (bool(fragShow) && fragAlpha > 0.2) {
		outPos = fragPos;
		outNormal = normalize(fragNormal);
		outMatl = fragReflRough;
	} else
		discard;
}
