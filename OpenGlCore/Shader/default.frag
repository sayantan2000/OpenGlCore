#version 330 core
out vec4 FragColor;
in vec3 color;
in vec2 texcord;
uniform sampler2D tex0;
vec4 Mul;
void main()
{
Mul=vec4(.8,.74,.7,1.00);
   FragColor = texture(tex0,texcord)*Mul;
   //FragColor =Mul;
}