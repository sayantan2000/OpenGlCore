#version 330 core
out vec4 FragColor;
in vec3 color;
in vec2 texcord;
uniform sampler2D tex0;
vec4 Mul;
uniform vec3 lightPos;
uniform vec4 lightcolor;
in  vec3 currentPos;
in vec3 normal;
void main()
{
	vec3 Norm=normalize(normal);
	vec3 lighdir=normalize(lightPos-currentPos);
	float diffuse=max(dot(lighdir,normal),0.2);
    FragColor = texture(tex0,texcord)*(diffuse+0.2);
   //FragColor =Mul;
}