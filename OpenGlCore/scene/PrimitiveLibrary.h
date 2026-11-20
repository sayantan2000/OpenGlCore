#ifndef PRIMITIVE_LIBRARY_H
#define PRIMITIVE_LIBRARY_H

#include <array>
#include <string>
#include <glm/glm.hpp>

namespace ogl {

class Mesh;

// Built-in primitive mesh shapes the editor's "Create > Primitive" menu can instantiate.
enum class PrimitiveType { Cube, Sphere, Plane, Cylinder, Cone, Torus, Capsule, Count };

// One shared primitive: its mesh (owned by Application) plus object-space AABB and a label.
struct PrimitiveDef
{
	Mesh* mesh = nullptr;
	glm::vec3 localMin = glm::vec3(-1.0f);
	glm::vec3 localMax = glm::vec3(1.0f);
	const char* name = "Primitive";
};

// A registry of the built-in primitives, indexed by PrimitiveType. Application fills it once
// (creating the GL meshes) and hands a const pointer to the Editor, which only references
// the shared meshes when creating GameObjects (GL ownership stays in Application).
struct PrimitiveLibrary
{
	std::array<PrimitiveDef, (size_t)PrimitiveType::Count> defs;

	const PrimitiveDef& Get(PrimitiveType t) const { return defs[(size_t)t]; }
	PrimitiveDef& Get(PrimitiveType t) { return defs[(size_t)t]; }

	// Looks up a primitive by its display name (e.g. "Cube"); nullptr if not found.
	const PrimitiveDef* FindByName(const std::string& name) const
	{
		for (const PrimitiveDef& d : defs)
			if (d.mesh && d.name && name == d.name) return &d;
		return nullptr;
	}
};


} // namespace ogl
#endif
