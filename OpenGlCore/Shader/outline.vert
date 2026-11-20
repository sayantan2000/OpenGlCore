#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 3) in vec3 aNormal;   // 11-float layout: normal at location 3

uniform mat4 u_mvp;
uniform float u_thickness;   // object-space shell thickness (pre-scaled by the caller)

void main()
{
	// Extrude along the vertex normal to form a slightly larger shell around the mesh.
	vec3 p = aPos + normalize(aNormal) * u_thickness;
	gl_Position = u_mvp * vec4(p, 1.0);
}
