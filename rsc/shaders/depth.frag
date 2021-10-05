#version 330 core

uniform vec3 lightPos;
uniform float farPlane;

in vec3 fragPos;
flat in float fragAlpha;
flat in int fragShow;

void main() {
	if (bool(fragShow) && fragAlpha > 0.2)
		gl_FragDepth = length(fragPos - lightPos) / farPlane;
	else
		discard;
}
