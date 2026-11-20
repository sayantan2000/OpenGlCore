#version 330 core
// Procedural HDR sky used as the IBL environment source (no external .hdr asset needed).
out vec4 FragColor;
in vec3 localPos;

void main()
{
	vec3 dir = normalize(localPos);

	// Sky gradient: horizon -> zenith, with a darker ground below the horizon.
	float t = clamp(dir.y * 0.5 + 0.5, 0.0, 1.0);
	vec3 horizon = vec3(0.70, 0.80, 0.92);
	vec3 zenith  = vec3(0.12, 0.32, 0.72);
	vec3 ground  = vec3(0.22, 0.20, 0.18);

	vec3 sky = mix(horizon, zenith, pow(t, 0.6));
	if (dir.y < 0.0)
		sky = mix(horizon, ground, clamp(-dir.y * 2.0, 0.0, 1.0));

	// Sun: bright HDR disc plus a soft glow.
	vec3 sunDir = normalize(vec3(0.3, 0.55, 0.4));
	float s = max(dot(dir, sunDir), 0.0);
	vec3 sun = vec3(1.0, 0.96, 0.88) * pow(s, 600.0) * 30.0;
	sun += vec3(1.0, 0.9, 0.75) * pow(s, 25.0) * 1.5;

	FragColor = vec4(sky + sun, 1.0);
}
