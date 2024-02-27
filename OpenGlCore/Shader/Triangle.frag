#version 330 core
out vec4 FragColor;
in vec3 verTexColor;
void main()
{
   FragColor =vec4(verTexColor,1)*vec4(3.2,2.25,6,1);
}