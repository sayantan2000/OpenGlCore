#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include <string>
#include <optional>
#include <cstdint>
#include <glm/glm.hpp>
#include "Transform.h"
#include "PbrMaterial.h"

class Model;
class Mesh;

// A renderable component referencing shared geometry (owned elsewhere) plus a per-object
// PBR material. Exactly one of (model, mesh) is set. `localMin`/`localMax` are the
// object-space bounds of the referenced geometry, used for ray-picking and focus framing.
struct MeshRenderer
{
	enum class Geometry { Model, Mesh };
	Geometry kind = Geometry::Mesh;
	Model* model = nullptr;   // when kind == Model (drawn via DrawGeometry)
	Mesh*  mesh = nullptr;    // when kind == Mesh
	std::string meshName = "Mesh";
	PbrMaterial material;
	std::string materialName = "Material";
	bool enabled = true;

	glm::vec3 localMin = glm::vec3(-1.0f); // object-space AABB (defaults suit unit primitives)
	glm::vec3 localMax = glm::vec3( 1.0f);
};

// A light component. Directional uses `direction`; point uses the owner's transform
// position; spot uses both. Rect is an area light whose plane is oriented by `direction`
// (the emitter normal) with `width`/`height` in world units. Color is linear; intensity
// scales it.
struct LightComponent
{
	enum class Type { Directional, Point, Spot, Rect };
	Type type = Type::Point;
	glm::vec3 color = glm::vec3(1.0f);
	float intensity = 1.0f;
	glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f); // directional/spot/rect (emitter normal)
	float innerCutoffDeg = 15.0f; // spot
	float outerCutoffDeg = 25.0f; // spot
	float width = 2.0f;           // rect
	float height = 1.0f;          // rect
	bool enabled = true;
};

// A scene object: a stable id, a name, an active flag, a transform, and optional
// components. Kept deliberately simple (optional components, not a generic ECS) per the
// project's scope. `id` is assigned by the Scene and stays constant for the object's
// lifetime so editor commands (undo/redo) can refer to it across index changes.
struct GameObject
{
	std::uint32_t id = 0;
	std::string name = "GameObject";
	bool active = true;

	Transform transform;
	std::optional<MeshRenderer> meshRenderer;
	std::optional<LightComponent> light;

	// Short type label for the Inspector header.
	const char* TypeLabel() const
	{
		if (light)
		{
			switch (light->type)
			{
			case LightComponent::Type::Directional: return "Directional Light";
			case LightComponent::Type::Point:       return "Point Light";
			case LightComponent::Type::Spot:        return "Spot Light";
			case LightComponent::Type::Rect:        return "Rectangle Light";
			}
		}
		if (meshRenderer) return "Mesh Object";
		return "Empty";
	}
};

#endif
