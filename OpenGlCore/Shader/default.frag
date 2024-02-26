#version 330 core
out vec4 FragColor;
in vec3 color;
in vec2 texcord;
uniform sampler2D tex0;
void main()
{
   FragColor = texture(tex0,texcord);
   //FragColor =vec4(color,1);
}