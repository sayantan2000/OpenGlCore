#version 330 core
// Procedural infinite grid: a full-screen triangle whose fragments reconstruct the world
// position on the y=0 plane. No vertex buffer needed (uses gl_VertexID).
out vec3 vNear;   // world-space point on the near plane for this pixel
out vec3 vFar;    // world-space point on the far plane

uniform mat4 u_invViewProj;

vec3 Unproject(vec2 ndc, float z)
{
	vec4 p = u_invViewProj * vec4(ndc, z, 1.0);
	return p.xyz / p.w;
}

void main()
{
	// Full-screen triangle: (-1,-1), (3,-1), (-1,3)
	vec2 pos = vec2(gl_VertexID == 1 ? 3.0 : -1.0, gl_VertexID == 2 ? 3.0 : -1.0);
	vNear = Unproject(pos, -1.0); // GL NDC near = -1
	vFar  = Unproject(pos,  1.0); // GL NDC far  = +1
	gl_Position = vec4(pos, 0.0, 1.0);
}
