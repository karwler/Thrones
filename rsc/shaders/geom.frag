#version 330 core

in vec3 fragPos;
in vec3 fragNormal;
flat in float fragAlpha;
flat in int fragShow;

layout (location = 0) out vec3 outPos;
layout (location = 1) out vec3 outNormal;

void main() {
	if (bool(fragShow) && fragAlpha > 0.2) {
		outPos = fragPos;
		outNormal = fragNormal;
	} else
		discard;
}
