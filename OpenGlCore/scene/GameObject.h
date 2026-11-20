#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include <string>
#include <optional>
#include <cstdint>
#include <map>
#include <glm/glm.hpp>
#include "Transform.h"
#include "PbrMaterial.h"

namespace ogl {

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
	// Serializable source descriptor, resolved on load: when kind == Model it is a model file
	// path (AssetManager); when kind == Mesh it is a primitive name (PrimitiveLibrary), e.g.
	// "Cube". Empty = not resolvable from assets (e.g. a one-off runtime mesh).
	std::string sourceId;
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

// A script behavior attached to an object. Stores only the registered behavior *name* (so a
// GameObject stays copyable/serializable); the live `ScriptBehavior` instance is owned by the
// ScriptSystem during Play mode and resolved by object id.
struct ScriptComponent
{
	std::string behaviorName;          // key into the ScriptRegistry ("" = none)
	std::map<std::string, float> params; // per-object tunables (see ScriptBehavior::DeclareParams)
	bool enabled = true;
};

// Physics material (friction / bounciness). Plain data; applied to a collider's PhysX shape.
struct PhysicsMaterialDesc
{
	float staticFriction = 0.5f;
	float dynamicFriction = 0.5f;
	float restitution = 0.1f;
};

// Rigid-body component. Static = immovable; Dynamic = simulated; Kinematic = moved by script
// but pushes dynamics. Values are applied to the PhysX actor when Play starts. Plain data so a
// GameObject stays copyable/serializable (the live PhysX actor lives in the PhysicsScene).
struct RigidBodyComponent
{
	enum class BodyType { Static, Dynamic, Kinematic };
	BodyType type = BodyType::Dynamic;
	float mass = 1.0f;
	bool useGravity = true;
	float linearDamping = 0.05f;
	float angularDamping = 0.05f;
	glm::vec3 initialLinearVelocity = glm::vec3(0.0f);
	glm::vec3 initialAngularVelocity = glm::vec3(0.0f);
	bool freezePosX = false, freezePosY = false, freezePosZ = false;
	bool freezeRotX = false, freezeRotY = false, freezeRotZ = false;
	bool enabled = true;
};

// Collider component. Primitive shapes use PhysX built-in geometry; mesh shapes are cooked
// from the renderer geometry. `center` is a local offset from the object's origin.
struct ColliderComponent
{
	enum class Shape { Box, Sphere, Capsule, Plane, ConvexMesh, TriangleMesh };
	Shape shape = Shape::Box;
	glm::vec3 center = glm::vec3(0.0f);
	glm::vec3 halfExtents = glm::vec3(0.5f); // box
	float radius = 0.5f;                      // sphere / capsule
	float halfHeight = 0.5f;                  // capsule (cylinder half-height)
	bool isTrigger = false;
	PhysicsMaterialDesc material;
	bool enabled = true;
};

// A scene object: a stable id, a name, an active flag, a transform, and optional
// components. Kept deliberately simple (optional components, not a generic ECS) per the
// project's scope. `id` is assigned by the Scene and stays constant for the object's
// lifetime so editor commands (undo/redo) can refer to it across index changes.
struct GameObject
{
	std::uint32_t id = 0;
	std::uint32_t parentId = 0;   // 0 = no parent (root); world matrix = parentWorld * local
	std::string name = "GameObject";
	bool active = true;

	Transform transform;          // local (relative to parent)
	std::optional<MeshRenderer> meshRenderer;
	std::optional<LightComponent> light;
	std::optional<ScriptComponent> script;
	std::optional<RigidBodyComponent> rigidBody;
	std::optional<ColliderComponent> collider;

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


} // namespace ogl
#endif
