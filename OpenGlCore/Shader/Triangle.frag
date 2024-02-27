#version 330 core
out vec4 FragColor;
in vec3 verTexColor;
void main()
{
   FragColor =vec4(verTexColor,1)*1.2;
}