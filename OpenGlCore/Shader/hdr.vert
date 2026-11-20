#version 330 core
// Fullscreen triangle generated from gl_VertexID (no VBO needed); draw with 3 vertices.
out vec2 UV;
void main()
{
	vec2 pos = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
	UV = pos;
	gl_Position = vec4(pos * 2.0 - 1.0, 0.0, 1.0);
}
