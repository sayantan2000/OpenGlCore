  #version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 acolor;
layout (location = 2) in vec2 aTex;

out vec3 color;
out vec2 texcord;
uniform float scale;


uniform mat4 camMatrix;
void main()
{
   gl_Position =camMatrix*vec4(aPos*scale,1);
   color=acolor;
   texcord=aTex;
}