#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include <string>
#include <optional>
#include <glm/glm.hpp>
#include "Transform.h"
#include "PbrMaterial.h"

class Model;
class Mesh;

// A renderable component referencing shared geometry (owned elsewhere) plus a per-object
// PBR material. Exactly one of (model, mesh) is set.
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
};

// A light component. Directional uses `direction`; point uses the owner's transform
// position; spot uses both. Color is linear; intensity scales it.
struct LightComponent
{
	enum class Type { Directional, Point, Spot };
	Type type = Type::Point;
	glm::vec3 color = glm::vec3(1.0f);
	float intensity = 1.0f;
	glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f); // directional/spot
	float innerCutoffDeg = 15.0f; // spot
	float outerCutoffDeg = 25.0f; // spot
	bool enabled = true;
};

// A scene object: a name, an active flag, a transform, and optional components. Kept
// deliberately simple (optional components, not a generic ECS) per the project's scope.
struct GameObject
{
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
			}
		}
		if (meshRenderer) return "Mesh Object";
		return "Empty";
	}
};

#endif
