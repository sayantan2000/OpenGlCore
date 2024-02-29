#version 330 core
out vec4 FragColor;
in vec3 color;
in vec2 texcord;
uniform sampler2D tex0;
vec4 Mul;
uniform vec3 lightPos;
uniform vec4 lightcolor;
uniform vec3 CamPos;
in  vec3 currentPos;
in vec3 normal;

void main()
{
	

	//ambient Lighting 
	float ambient=0.2;


	//Diffuse Lighting
	vec3 Norm=normalize(normal);
	vec3 lighdir=normalize(lightPos-currentPos);
	float diffuse=max(dot(lighdir,normal),0.0);


	//specularLighting
	float specularLight = 0.50f;
	vec3 viewDirection = normalize(CamPos - currentPos);
	vec3 reflectionDirection = reflect(-lighdir, normal);
	float specAmount = pow(max(dot(viewDirection, reflectionDirection), 0.0f), 8);
	float specular = specAmount * specularLight;



    FragColor = texture(tex0,texcord)*lightcolor*(diffuse+ambient+specular);
}