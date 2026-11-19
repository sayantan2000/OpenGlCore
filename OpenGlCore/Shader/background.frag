#version 330 core
out vec4 FragColor;
in vec3 localPos;
uniform samplerCube environmentMap;
uniform float u_bgIntensity; // skybox brightness, independent of scene exposure
void main()
{
	// Output linear HDR; the HDR post pass tonemaps + gamma-corrects.
	vec3 color = texture(environmentMap, localPos).rgb * u_bgIntensity;
	FragColor = vec4(color, 1.0);
}
