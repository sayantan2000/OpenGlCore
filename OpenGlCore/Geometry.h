#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <vector>
#include <glad/glad.h>

// Procedural geometry helpers. Spheres use the project's standard 11-float vertex
// layout (pos3, color3, uv2, normal3) so they render with the PBR / default shaders.
namespace Geometry
{
	// UV sphere of the given radius. `segments` controls both longitude and latitude.
	void BuildSphere(std::vector<GLfloat>& vertices, std::vector<GLuint>& indices,
		int segments = 64, float radius = 1.0f);
}

#endif
