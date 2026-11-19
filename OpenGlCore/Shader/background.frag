#version 330 core
out vec4 FragColor;
in vec3 localPos;
uniform samplerCube environmentMap;
void main()
{
	// Output linear HDR; the HDR post pass tonemaps + gamma-corrects.
	vec3 color = texture(environmentMap, localPos).rgb;
	FragColor = vec4(color, 1.0);
}
