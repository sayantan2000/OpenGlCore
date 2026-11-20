#include "ScriptRegistry.h"
#include "ScriptBehavior.h"
#include "GameObject.h"
#include "Input.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <unordered_map>
#include <cmath>

// ---------------------------------------------------------------------------------------
// Built-in example behaviors (demonstrate transform control, time, and input).
// ---------------------------------------------------------------------------------------
namespace
{
	// Spins the object around its local Y axis.
	class Rotator : public ScriptBehavior
	{
	public:
		void DeclareParams(std::vector<ScriptParam>& out) const override
		{
			out.push_back({ "speed", 60.0f }); // degrees/second
		}
		void OnUpdate(ScriptContext& ctx) override
		{
			ctx.self.transform.rotationEuler.y += GetParam(ctx, "speed", 60.0f) * ctx.deltaTime;
		}
	};

	// Bobs up and down around the object's starting height.
	class FloatBob : public ScriptBehavior
	{
	public:
		void DeclareParams(std::vector<ScriptParam>& out) const override
		{
			out.push_back({ "amplitude", 1.0f });
			out.push_back({ "frequency", 1.0f });
		}
		void OnStart(ScriptContext& ctx) override { baseY = ctx.self.transform.position.y; t = 0.0f; }
		void OnUpdate(ScriptContext& ctx) override
		{
			t += ctx.deltaTime;
			float amp = GetParam(ctx, "amplitude", 1.0f);
			float freq = GetParam(ctx, "frequency", 1.0f);
			ctx.self.transform.position.y = baseY + std::sin(t * freq * 6.2831853f) * amp;
		}
	private:
		float baseY = 0.0f;
		float t = 0.0f;
	};

	// Moves the object on the XZ plane from keyboard axes (WASD / arrows).
	class WasdMover : public ScriptBehavior
	{
	public:
		void DeclareParams(std::vector<ScriptParam>& out) const override
		{
			out.push_back({ "speed", 4.0f });
		}
		void OnUpdate(ScriptContext& ctx) override
		{
			float speed = GetParam(ctx, "speed", 4.0f);
			float h = ctx.input.GetAxis("Horizontal");
			float v = ctx.input.GetAxis("Vertical");
			ctx.self.transform.position.x += h * speed * ctx.deltaTime;
			ctx.self.transform.position.z -= v * speed * ctx.deltaTime; // forward = -Z
		}
	};

	struct Registry
	{
		std::unordered_map<std::string, ScriptRegistry::Factory> factories;
		std::vector<std::string> names;
	};

	Registry& R()
	{
		static Registry r;
		return r;
	}
}

namespace ScriptRegistry
{
	void Register(const std::string& name, Factory factory)
	{
		if (R().factories.find(name) == R().factories.end())
			R().names.push_back(name);
		R().factories[name] = std::move(factory);
	}

	void RegisterBuiltins()
	{
		Register("Rotator", [] { return std::make_unique<Rotator>(); });
		Register("FloatBob", [] { return std::make_unique<FloatBob>(); });
		Register("WasdMover", [] { return std::make_unique<WasdMover>(); });
	}

	std::unique_ptr<ScriptBehavior> Create(const std::string& name)
	{
		auto it = R().factories.find(name);
		return it != R().factories.end() ? it->second() : nullptr;
	}

	bool Has(const std::string& name) { return R().factories.count(name) != 0; }
	const std::vector<std::string>& Names() { return R().names; }

	std::vector<ScriptParam> ParamsOf(const std::string& name)
	{
		std::vector<ScriptParam> out;
		if (auto b = Create(name)) b->DeclareParams(out);
		return out;
	}
}

// Reads a per-object script parameter (set in the Inspector), falling back to a default.
float GetParam(const ScriptContext& ctx, const char* name, float def)
{
	if (ctx.self.script)
	{
		auto it = ctx.self.script->params.find(name);
		if (it != ctx.self.script->params.end()) return it->second;
	}
	return def;
}
