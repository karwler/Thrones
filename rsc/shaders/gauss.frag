#version 330 core

const float weight[5] = float[]( 0.2270270270, 0.1945945946, 0.1216216216, 0.0540540541, 0.0162162162 );

uniform sampler2D colorMap;
uniform bool horizontal;

noperspective in vec2 fragUV;

out vec4 fragColor;

void main() {
	vec2 tsize = vec2(textureSize(colorMap, 0));
	vec3 result = texture(colorMap, fragUV).rgb * weight[0];
	if (horizontal) {
		for(int i = 1; i < 5; ++i) {
			float xofs = float(i) / tsize.x;
			result += texture(colorMap, fragUV + vec2(xofs, 0.0)).rgb * weight[i];
			result += texture(colorMap, fragUV - vec2(xofs, 0.0)).rgb * weight[i];
		}
	} else {
		for(int i = 1; i < 5; ++i) {
			float yofs = float(i) / tsize.y;
			result += texture(colorMap, fragUV + vec2(0.0, yofs)).rgb * weight[i];
			result += texture(colorMap, fragUV - vec2(0.0, yofs)).rgb * weight[i];
		}
	}
	fragColor = vec4(result, 1.0);
}
