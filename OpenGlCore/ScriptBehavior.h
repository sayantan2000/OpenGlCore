#ifndef SCRIPT_BEHAVIOR_H
#define SCRIPT_BEHAVIOR_H

class Scene;
class Input;
struct GameObject;

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
// stores the behavior's registered name). Override the hooks you need.
class ScriptBehavior
{
public:
	virtual ~ScriptBehavior() = default;
	virtual void OnStart(ScriptContext&) {}            // once, when Play begins
	virtual void OnUpdate(ScriptContext&) {}           // every frame while playing
};

#endif
