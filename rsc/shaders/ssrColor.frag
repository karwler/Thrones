#version 330 core

const int size = 6;
const float separation = 2.0;

uniform sampler2D colorMap;
uniform sampler2D ssrMap;

noperspective in vec2 fragUV;

out vec4 fragColor;

void main() {
	vec2 texSize = vec2(textureSize(ssrMap, 0));
	vec4 uv = texture(ssrMap, fragUV);
	if (uv.b <= 0.0) {
		uv = vec4(0.0);
		float count = 0.0;

		for (int i = -size; i <= size; ++i)
			for (int j = -size; j <= size; ++j) {
				uv += texture(ssrMap, (vec2(i, j) * separation + gl_FragCoord.xy) / texSize);
				count += 1.0;
			}
		uv.xyz /= count;
	}

	if (uv.b <= 0.0) {
		fragColor = vec4(0.0);
		return;
	}
	vec4 color = texture(colorMap, uv.xy);
	float alpha = clamp(uv.b, 0.0, 1.0);
	fragColor = vec4(mix(vec3(0.0), color.rgb, alpha), alpha);
}
