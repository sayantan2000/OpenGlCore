#version 330 core

//structure Hold Position,Normal,Texcord,Color
struct Data{
	vec3 Position;
	vec3 Normal;
	vec4 color;
	vec2 Texcord0;
};
out vec4 FragColor;
uniform sampler2D tex0;
uniform vec3 lightPos;
uniform vec4 lightcolor;
uniform vec3 CamPos;
in Data data;


vec4 AmbientAndDiffuseLighting();

void main()
{

    FragColor = AmbientAndDiffuseLighting();
}


vec4 AmbientAndDiffuseLighting(){

//ambient Lighting 
	float ambient=0.2;


	//Diffuse Lighting
	vec3 Norm=normalize(data.Normal);
	vec3 lighdir=normalize(lightPos-data.Position);
	float diffuse=max(dot(lighdir,data.Normal),0.0);


	//specularLighting
	float specularLight = 0.50f;
	vec3 viewDirection = normalize(CamPos - data.Position);
	vec3 reflectionDirection = reflect(-lighdir, data.Normal);
	float specAmount = pow(max(dot(viewDirection, reflectionDirection), 0.0f), 8);
	float specular = specAmount * specularLight;
	

	return texture(tex0,data.Texcord0)*lightcolor*(diffuse+ambient+specular);

}
