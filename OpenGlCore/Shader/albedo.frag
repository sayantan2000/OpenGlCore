#version 330 core

// Lit solid-color shader for loaded models: uses a uniform `albedo` base color
// instead of a texture. Shares default.vert (same Data struct / uniforms).
struct Data{
	vec3 Position;
	vec3 Normal;
	vec4 color;
	vec2 Texcord0;
};
out vec4 FragColor;
uniform vec3 albedo;          // solid base color (used when hasTexture == 0)
uniform bool hasTexture;      // 1 -> sample modelTex, 0 -> use albedo
uniform sampler2D modelTex;   // diffuse texture (unit 0)
uniform vec3 lightPos;
uniform vec4 lightcolor;
uniform vec3 CamPos;
in Data data;

void main()
{
	float ambient = 0.2;

	vec3 Norm = normalize(data.Normal);
	vec3 lightDir = normalize(lightPos - data.Position);
	float diffuse = max(dot(lightDir, Norm), 0.0);

	float specularLight = 0.50;
	vec3 viewDirection = normalize(CamPos - data.Position);
	vec3 reflectionDirection = reflect(-lightDir, Norm);
	float specAmount = pow(max(dot(viewDirection, reflectionDirection), 0.0), 8.0);
	float specular = specAmount * specularLight;

	vec3 base = hasTexture ? texture(modelTex, data.Texcord0).rgb : albedo;
	FragColor = vec4(base, 1.0) * lightcolor * (diffuse + ambient + specular);
}
