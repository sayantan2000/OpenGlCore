 #version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 acolor;
layout (location = 2) in vec2 aTex;
layout (location = 3) in vec3 Normal;

//structure to store Position,Normal,Texcord,Color to pass the fragmentshader
  struct Data{
	vec3 Position;
	vec3 Normal;
	vec4 color;
	vec2 Texcord0;
};
out mat4 MVP;
uniform float scale;
uniform mat4 model;
uniform mat4 camMatrix;

out Data data;
void main()
{
	MVP=camMatrix*model;
	data.color=vec4(acolor,1);
	data.Texcord0=aTex;
	//rotating The Normals Asper Model Updates Its Rotations
	data.Position=vec3(model*vec4(aPos,1));
	// Correct normal matrix = inverse-transpose of the model's upper-left 3x3.
	// Handles non-uniform scale correctly (the old transpose(mat3(model)) did not,
	// and multiplied the vector on the wrong side).
	mat3 normalMatrix = transpose(inverse(mat3(model)));
	data.Normal=normalize(normalMatrix * Normal);


	gl_Position =MVP*vec4(aPos*scale,1);
   
  
}