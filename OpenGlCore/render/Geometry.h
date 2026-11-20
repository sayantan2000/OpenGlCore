#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <vector>
#include <glad/glad.h>

namespace ogl {

// Procedural geometry helpers. All primitives use the project's standard 11-float vertex
// layout (pos3, color3, uv2, normal3) so they render with the PBR / default shaders. Each
// builder fills `vertices`/`indices` (both cleared first). Primitives are centred on the
// origin and roughly unit-sized (fit within a [-1,1] box) so a Transform scales them cleanly.
namespace Geometry
{
	// UV sphere of the given radius. `segments` controls both longitude and latitude.
	void BuildSphere(std::vector<GLfloat>& vertices, std::vector<GLuint>& indices,
		int segments = 64, float radius = 1.0f);

	// Unit cube spanning [-1,1] on each axis (per-face normals + UVs).
	void BuildCube(std::vector<GLfloat>& vertices, std::vector<GLuint>& indices);

	// Flat plane in the XZ plane spanning [-1,1], normal +Y, `subdivisions` cells per side.
	void BuildPlane(std::vector<GLfloat>& vertices, std::vector<GLuint>& indices, int subdivisions = 1);

	// Y-axis cylinder: radius `radius`, total height `height` (centred), with capped ends.
	void BuildCylinder(std::vector<GLfloat>& vertices, std::vector<GLuint>& indices,
		int segments = 32, float radius = 1.0f, float height = 2.0f);

	// Y-axis cone: base radius `radius`, total height `height` (centred), with a capped base.
	void BuildCone(std::vector<GLfloat>& vertices, std::vector<GLuint>& indices,
		int segments = 32, float radius = 1.0f, float height = 2.0f);

	// Torus around the Y axis. `ringRadius` = distance to tube centre, `tubeRadius` = tube.
	void BuildTorus(std::vector<GLfloat>& vertices, std::vector<GLuint>& indices,
		int ringSegments = 48, int tubeSegments = 24, float ringRadius = 0.75f, float tubeRadius = 0.25f);

	// Capsule along the Y axis: hemispherical caps of `radius` joined by a cylinder of
	// `cylinderHeight`; total height = cylinderHeight + 2*radius, centred on the origin.
	void BuildCapsule(std::vector<GLfloat>& vertices, std::vector<GLuint>& indices,
		int segments = 24, float radius = 0.5f, float cylinderHeight = 1.0f);
}


} // namespace ogl
#endif
