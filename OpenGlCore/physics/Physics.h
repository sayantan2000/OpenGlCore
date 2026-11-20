#ifndef PHYSICS_H
#define PHYSICS_H

#include <cstdint>
#include <vector>
#include <memory>
#include <glm/glm.hpp>

namespace ogl {

class Scene;

// Result of a scene query (raycast / sphere cast).
struct RaycastHit
{
	bool hit = false;
	std::uint32_t objectId = 0;     // GameObject id of what was hit (0 if none)
	glm::vec3 point = glm::vec3(0.0f);
	glm::vec3 normal = glm::vec3(0.0f);
	float distance = 0.0f;
};

// A contact or trigger event produced during a simulation step (by object id).
struct ContactEvent
{
	enum class Kind { CollisionEnter, CollisionExit, TriggerEnter, TriggerExit };
	Kind kind = Kind::CollisionEnter;
	std::uint32_t a = 0;
	std::uint32_t b = 0;
};

// A debug line (world space) for physics visualization.
struct DebugLine { glm::vec3 a, b, color; };

// Wraps the NVIDIA PhysX SDK. Lifetime: one PhysicsScene exists for the app; Begin() builds
// PhysX actors from the scene's RigidBody/Collider components at Play start, Step() advances a
// fixed-timestep simulation and writes dynamic poses back into the GameObjects, and End() tears
// the actors down (edit-time scene is untouched — Play mode restores it via its JSON snapshot).
// All PhysX types are hidden behind a pImpl so this header has no PhysX dependency.
class PhysicsScene
{
public:
	PhysicsScene();
	~PhysicsScene();
	PhysicsScene(const PhysicsScene&) = delete;
	PhysicsScene& operator=(const PhysicsScene&) = delete;

	bool Available() const;           // false if the PhysX SDK failed to initialize
	void SetGravity(const glm::vec3& g);

	void Begin(Scene& scene);         // create actors from components (Play start)
	void Step(Scene& scene, float dt);// accumulate dt, run fixed sub-steps, sync transforms
	void End();                       // release all actors
	bool IsRunning() const;

	// Per-object runtime controls (valid while running; id = GameObject id).
	void SetLinearVelocity(std::uint32_t id, const glm::vec3& v);
	glm::vec3 GetLinearVelocity(std::uint32_t id) const;
	void AddForce(std::uint32_t id, const glm::vec3& force);
	void AddImpulse(std::uint32_t id, const glm::vec3& impulse);

	// Scene queries.
	bool Raycast(const glm::vec3& origin, const glm::vec3& dir, float maxDist, RaycastHit& out) const;
	bool SphereCast(const glm::vec3& origin, float radius, const glm::vec3& dir, float maxDist, RaycastHit& out) const;
	int  OverlapSphere(const glm::vec3& center, float radius, std::vector<std::uint32_t>& outIds) const;

	// Collision/trigger events generated during the most recent Step (consumed by ScriptSystem).
	const std::vector<ContactEvent>& Events() const;

	// Fixed timestep (seconds) used for simulation sub-steps.
	void SetFixedTimestep(float dt);
	float FixedTimestep() const;

private:
	struct Impl;
	std::unique_ptr<Impl> impl;
};

// Builds collider/trigger/raycast wireframes for debug visualization from component data
// (independent of PhysX, so it also works in edit mode). Appends to `out`.
namespace PhysicsDebug
{
	void BuildColliderLines(Scene& scene, std::vector<DebugLine>& out);
}


} // namespace ogl
#endif
