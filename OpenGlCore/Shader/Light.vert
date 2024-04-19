  #version 330 core
layout (location = 0) in vec3 aPos;
out vec3 verTexColor;
uniform mat4 model;
uniform mat4 camMatrix;
uniform vec3 Scale;
void main()
{
verTexColor=aPos;
   gl_Position =camMatrix*model*vec4(aPos*Scale,1);
}