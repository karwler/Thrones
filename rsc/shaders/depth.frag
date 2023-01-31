#version 330 core

const float lightRange = 140.0;

uniform vec3 lightPos;

in vec3 fragPos;
flat in float fragAlpha;
flat in int fragShow;

void main() {
	if (bool(fragShow) && fragAlpha > 0.2)
		gl_FragDepth = length(fragPos - lightPos) / lightRange;
	else
		discard;
}
