  #version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 acolor;
layout (location = 2) in vec2 aTex;
layout (location = 3) in vec3 Normal;

out vec3 color;
out vec2 texcord;
out vec3 currentPos;
out vec3 normal;
uniform float scale;

uniform mat4 model;
uniform mat4 camMatrix;


mat3 normalMat;
void main()
{
	color=acolor;
	texcord=aTex;
	//rotating The Normals Asper Model Updates Its Rotations
	currentPos=vec3(model*vec4(aPos,1));
	normalMat=mat3(camMatrix*model);
	normalMat=transpose(inverse(normalMat));
	normal=normalize(Normal*normalMat);


	gl_Position =camMatrix*model*vec4(aPos*scale,1);
   
  
}