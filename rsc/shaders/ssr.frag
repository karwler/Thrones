#version 330 core

struct Material {
	float reflective;
};

const float maxDistance = 6.0;
const float resolution = 0.3;
const float thickness = 0.5;

uniform mat4 proj;
uniform Material materials[19];

uniform sampler2D vposMap;
uniform sampler2D normMap;
uniform usampler2D matlMap;

noperspective in vec2 fragUV;

out vec4 fragColor;

void main() {
	vec4 positionFrom = texture(vposMap, fragUV);
	if (positionFrom.w <= 0.0 || materials[texture(matlMap, fragUV).r].reflective <= 0.0) {
		fragColor = vec4(0.0);
		return;
	}

	vec2 texSize = vec2(textureSize(vposMap, 0));
	vec3 unitPositionFrom = normalize(positionFrom.xyz);
	vec3 normal = normalize(texture(normMap, fragUV).xyz);
	vec3 pivot = normalize(reflect(unitPositionFrom, normal));
	vec4 positionTo = positionFrom;
	vec4 startView = vec4(positionFrom.xyz + (pivot * 0.0), 1.0);
	vec4 endView = vec4(positionFrom.xyz + (pivot * maxDistance), 1.0);

	vec4 startFrag = proj * startView;
	startFrag.xyz /= startFrag.w;
	startFrag.xy = (startFrag.xy * 0.5 + 0.5) * texSize;

	vec4 endFrag = proj * endView;
	endFrag.xyz /= endFrag.w;
	endFrag.xy = (endFrag.xy * 0.5 + 0.5) * texSize;

	vec2 frag = startFrag.xy;
	vec4 uv = vec4(frag / texSize, 0.0, 0.0);

	float deltaX = endFrag.x - startFrag.x;
	float deltaY = endFrag.y - startFrag.y;
	float useX = abs(deltaX) >= abs(deltaY) ? 1.0 : 0.0;
	float delta = mix(abs(deltaY), abs(deltaX), useX) * clamp(resolution, 0.0, 1.0);
	vec2 increment = vec2(deltaX, deltaY) / max(delta, 0.001);
	float search0 = 0.0;
	float search1 = 0.0;
	int hit0 = 0;
	int hit1 = 0;
	float viewDistance = startView.y;
	float depth = thickness;

	for (int i = 0; i < int(delta); ++i) {
		frag += increment;
		uv.xy = frag / texSize;
		positionTo = texture(vposMap, uv.xy);
		search1 = mix((frag.y - startFrag.y) / deltaY, (frag.x - startFrag.x) / deltaX, useX);
		search1 = clamp(search1, 0.0, 1.0);
		viewDistance = (startView.y * endView.y) / mix(endView.y, startView.y, search1);
		depth = viewDistance - positionTo.y;

		if (depth > 0.0 && depth < thickness) {
			hit0 = 1;
			break;
		}
		search0 = search1;
	}

	int steps = 5;
	search1 = search0 + (search1 - search0) * 0.5;
	steps *= hit0;

	for (int i = 0; i < steps; ++i) {
		frag = mix(startFrag.xy, endFrag.xy, search1);
		uv.xy = frag / texSize;
		positionTo = texture(vposMap, uv.xy);
		viewDistance = (startView.y * endView.y) / mix(endView.y, startView.y, search1);
		depth = viewDistance - positionTo.y;

		if (depth > 0.0 && depth < thickness) {
			hit1 = 1;
			search1 = search0 + (search1 - search0) * 0.5;
		} else {
			float temp = search1;
			search1 = search1 + (search1 - search0) * 0.5;
			search0 = temp;
		}
	}

	float visibility = float(hit1) * positionTo.w * (1.0 - max(dot(-unitPositionFrom, pivot), 0.0)) * (1.0 - clamp(depth / thickness, 0.0, 1.0)) * (1.0 - clamp(length(positionTo - positionFrom) / maxDistance, 0.0, 1.0)) * (uv.x < 0.0 || uv.x > 1.0 ? 0.0 : 1.0) * (uv.y < 0.0 || uv.y > 1.0 ? 0.0 : 1.0);
	uv.ba = vec2(clamp(visibility, 0.0, 1.0));
	fragColor = uv;
}
