#version 330 core
out vec4 FragColor;
in vec3 verTexColor;
uniform vec4 LightColor;
uniform float _Time;
void main()
{
   // Use the declared output instead of the deprecated gl_FragColor (not valid in core).
   FragColor = LightColor;
}