#ifndef SCRIPT_SYSTEM_H
#define SCRIPT_SYSTEM_H

#include <cstdint>
#include <memory>
#include <unordered_map>

namespace ogl {

class Scene;
class Input;
class ScriptBehavior;
class PhysicsScene;
class EventBus;

// Owns the live script instances during Play mode. Begin() instantiates a behavior (via the
// ScriptRegistry) for every object that has an enabled ScriptComponent and calls OnStart;
// Update() ticks them each frame; End() destroys them. Instances are keyed by object id so a
// GameObject itself stays copyable/serializable (it only stores the behavior name).
class ScriptSystem
{
public:
	ScriptSystem();
	~ScriptSystem();   // out-of-line: unique_ptr<ScriptBehavior> needs the complete type here

	// Optional: the shared event bus exposed to scripts (ScriptContext::events) and used to
	// re-publish physics collision/trigger events as engine events. Set once after construction.
	void SetEventBus(EventBus* bus) { eventBus = bus; }

	void Begin(Scene& scene, Input& input, PhysicsScene* physics, float deltaTime);
	void Update(Scene& scene, Input& input, PhysicsScene* physics, float deltaTime);
	// Dispatches OnCollision/OnTrigger for the physics events produced by the last Step.
	void DispatchPhysicsEvents(Scene& scene, Input& input, PhysicsScene* physics, float deltaTime);
	void End();

	bool IsRunning() const { return running; }

private:
	std::unordered_map<std::uint32_t, std::unique_ptr<ScriptBehavior>> instances;
	EventBus* eventBus = nullptr;
	bool running = false;
};


} // namespace ogl
#endif
