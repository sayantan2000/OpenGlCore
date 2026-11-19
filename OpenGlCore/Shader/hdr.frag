#version 330 core
out vec4 FragColor;
in vec2 UV;

uniform sampler2D u_hdr;   // HDR scene color (linear)
uniform float u_exposure;

// Narkowicz ACES filmic tonemap approximation.
vec3 ACESFilm(vec3 x)
{
	const float a = 2.51;
	const float b = 0.03;
	const float c = 2.43;
	const float d = 0.59;
	const float e = 0.14;
	return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main()
{
	vec3 hdr = texture(u_hdr, UV).rgb;
	vec3 mapped = ACESFilm(hdr * u_exposure);
	mapped = pow(mapped, vec3(1.0 / 2.2)); // linear -> sRGB
	FragColor = vec4(mapped, 1.0);
}
