#version 330 core
out vec4 FragColor;
in vec3 verTexColor;
uniform vec4 LightColor;
uniform float _Time;
void main()
{
   gl_FragColor =LightColor;
}