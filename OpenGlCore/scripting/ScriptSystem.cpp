#include "ScriptSystem.h"
#include "ScriptBehavior.h"
#include "ScriptRegistry.h"
#include "Scene.h"
#include "GameObject.h"
#include "Physics.h"

namespace ogl {

ScriptSystem::ScriptSystem() = default;
ScriptSystem::~ScriptSystem() = default; // ScriptBehavior complete here -> unique_ptr can delete

void ScriptSystem::Begin(Scene& scene, Input& input, PhysicsScene* physics, float deltaTime)
{
	instances.clear();
	for (auto& up : scene.Objects())
	{
		GameObject* go = up.get();
		if (!go->script || !go->script->enabled || go->script->behaviorName.empty()) continue;
		auto behavior = ScriptRegistry::Create(go->script->behaviorName);
		if (!behavior) continue;
		ScriptContext ctx{ *go, scene, input, deltaTime, physics };
		behavior->OnStart(ctx);
		instances.emplace(go->id, std::move(behavior));
	}
	running = true;
}

void ScriptSystem::Update(Scene& scene, Input& input, PhysicsScene* physics, float deltaTime)
{
	if (!running) return;
	for (auto& [id, behavior] : instances)
	{
		GameObject* go = scene.FindById(id);
		if (!go || !go->active || !go->script || !go->script->enabled) continue;
		ScriptContext ctx{ *go, scene, input, deltaTime, physics };
		behavior->OnUpdate(ctx);
	}
}

void ScriptSystem::DispatchPhysicsEvents(Scene& scene, Input& input, PhysicsScene* physics, float deltaTime)
{
	if (!running || !physics) return;
	auto fire = [&](std::uint32_t id, std::uint32_t other, bool trigger)
	{
		auto it = instances.find(id);
		if (it == instances.end()) return;
		GameObject* go = scene.FindById(id);
		if (!go) return;
		ScriptContext ctx{ *go, scene, input, deltaTime, physics };
		if (trigger) it->second->OnTrigger(ctx, other);
		else it->second->OnCollision(ctx, other);
	};
	for (const ContactEvent& e : physics->Events())
	{
		if (e.kind == ContactEvent::Kind::CollisionEnter) { fire(e.a, e.b, false); fire(e.b, e.a, false); }
		else if (e.kind == ContactEvent::Kind::TriggerEnter) { fire(e.a, e.b, true); fire(e.b, e.a, true); }
	}
}

void ScriptSystem::End()
{
	instances.clear();
	running = false;
}

} // namespace ogl