#ifndef SCRIPT_REGISTRY_H
#define SCRIPT_REGISTRY_H

#include <string>
#include <vector>
#include <memory>
#include <functional>

class ScriptBehavior;

// A name -> factory registry for script behaviors, so behaviors can be referenced by string
// (serializable) and instantiated by the ScriptSystem on Play. Register custom behaviors via
// Register(); RegisterBuiltins() adds the engine's example behaviors.
namespace ScriptRegistry
{
	using Factory = std::function<std::unique_ptr<ScriptBehavior>()>;

	void Register(const std::string& name, Factory factory);
	void RegisterBuiltins();                              // Rotator, FloatBob, WasdMover
	std::unique_ptr<ScriptBehavior> Create(const std::string& name); // nullptr if unknown
	bool Has(const std::string& name);
	const std::vector<std::string>& Names();             // registration order
}

#endif
