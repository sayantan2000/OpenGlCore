#ifndef SCRIPT_BEHAVIOR_H
#define SCRIPT_BEHAVIOR_H

#include <string>
#include <vector>

class Scene;
class Input;
struct GameObject;

// A tunable parameter a behavior exposes to the editor (name + default value). Per-object
// values live in the GameObject's ScriptComponent.params map and are read via GetParam().
struct ScriptParam
{
	std::string name;
	float defaultValue = 0.0f;
};

// What a running script can see/touch each frame. Deliberately small: the object it's on, the
// scene (to find/spawn others later), and input. Extend as the engine grows.
struct ScriptContext
{
	GameObject& self;
	Scene& scene;
	Input& input;
	float deltaTime;
};

// Base class for a gameplay behavior. Instances live only during Play mode (owned by the
// ScriptSystem, keyed by object id), so a GameObject stays copyable/serializable (it only
// stores the behavior's registered name + a params map). Override the hooks you need.
class ScriptBehavior
{
public:
	virtual ~ScriptBehavior() = default;
	virtual void OnStart(ScriptContext&) {}            // once, when Play begins
	virtual void OnUpdate(ScriptContext&) {}           // every frame while playing

	// Parameters this behavior exposes in the Inspector (name + default). Override to declare.
	virtual void DeclareParams(std::vector<ScriptParam>&) const {}
};

// Reads a per-object script parameter by name (falls back to `def` if unset). Defined in
// ScriptRegistry.cpp (needs the GameObject layout).
float GetParam(const ScriptContext& ctx, const char* name, float def);

#endif
