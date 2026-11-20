#version 330 core
// Projects an equirectangular HDR image onto cube faces (direction -> lat/long UV).
out vec4 FragColor;
in vec3 localPos;
uniform sampler2D equirectangularMap;

const vec2 invAtan = vec2(0.1591, 0.3183); // 1/(2pi), 1/pi

vec2 SampleSphericalMap(vec3 v)
{
	vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
	uv *= invAtan;
	uv += 0.5;
	return uv;
}

void main()
{
	vec2 uv = SampleSphericalMap(normalize(localPos));
	// Preserve full HDR range (no clamping).
	vec3 color = texture(equirectangularMap, uv).rgb;
	FragColor = vec4(color, 1.0);
}
