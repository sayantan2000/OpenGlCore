#include "ScriptSystem.h"
#include "ScriptBehavior.h"
#include "ScriptRegistry.h"
#include "Scene.h"
#include "GameObject.h"
#include "Physics.h"
#include "EventBus.h"

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
		ScriptContext ctx{ *go, scene, input, deltaTime, physics, eventBus };
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
		ScriptContext ctx{ *go, scene, input, deltaTime, physics, eventBus };
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
		ScriptContext ctx{ *go, scene, input, deltaTime, physics, eventBus };
		if (trigger) it->second->OnTrigger(ctx, other);
		else it->second->OnCollision(ctx, other);
	};
	for (const ContactEvent& e : physics->Events())
	{
		switch (e.kind)
		{
		case ContactEvent::Kind::CollisionEnter:
			fire(e.a, e.b, false); fire(e.b, e.a, false);
			if (eventBus) eventBus->Publish(CollisionBeginEvent{ e.a, e.b });
			break;
		case ContactEvent::Kind::CollisionExit:
			if (eventBus) eventBus->Publish(CollisionEndEvent{ e.a, e.b });
			break;
		case ContactEvent::Kind::TriggerEnter:
			fire(e.a, e.b, true); fire(e.b, e.a, true);
			if (eventBus) eventBus->Publish(TriggerBeginEvent{ e.a, e.b });
			break;
		case ContactEvent::Kind::TriggerExit:
			if (eventBus) eventBus->Publish(TriggerEndEvent{ e.a, e.b });
			break;
		}
	}
}

void ScriptSystem::End()
{
	instances.clear();
	running = false;
}

} // namespace ogl