#version 330 core
// Project standard 11-float layout: pos(3) color(3, unused by PBR) uv(2) normal(3).
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aUV;
layout (location = 3) in vec3 aNormal;

uniform mat4 model;
uniform mat4 camMatrix; // projection * view

out vec3 WorldPos;
out vec3 Normal;
out vec2 UV;

void main()
{
	WorldPos = vec3(model * vec4(aPos, 1.0));
	Normal = mat3(transpose(inverse(model))) * aNormal;
	UV = aUV;
	gl_Position = camMatrix * vec4(WorldPos, 1.0);
}
