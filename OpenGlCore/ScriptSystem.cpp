#include "ScriptSystem.h"
#include "ScriptBehavior.h"
#include "ScriptRegistry.h"
#include "Scene.h"
#include "GameObject.h"

ScriptSystem::ScriptSystem() = default;
ScriptSystem::~ScriptSystem() = default; // ScriptBehavior complete here -> unique_ptr can delete

void ScriptSystem::Begin(Scene& scene, Input& input, float deltaTime)
{
	instances.clear();
	for (auto& up : scene.Objects())
	{
		GameObject* go = up.get();
		if (!go->script || !go->script->enabled || go->script->behaviorName.empty()) continue;
		auto behavior = ScriptRegistry::Create(go->script->behaviorName);
		if (!behavior) continue;
		ScriptContext ctx{ *go, scene, input, deltaTime };
		behavior->OnStart(ctx);
		instances.emplace(go->id, std::move(behavior));
	}
	running = true;
}

void ScriptSystem::Update(Scene& scene, Input& input, float deltaTime)
{
	if (!running) return;
	for (auto& [id, behavior] : instances)
	{
		GameObject* go = scene.FindById(id);
		if (!go || !go->active || !go->script || !go->script->enabled) continue;
		ScriptContext ctx{ *go, scene, input, deltaTime };
		behavior->OnUpdate(ctx);
	}
}

void ScriptSystem::End()
{
	instances.clear();
	running = false;
}
