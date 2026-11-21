#include "ScriptRegistry.h"
#include "ScriptBehavior.h"
#include "GameObject.h"
#include "Input.h"
#include "Physics.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <unordered_map>
#include <cmath>

namespace ogl {

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

	// Drives a CharacterControllerComponent via the PhysX character controller: WASD/left-stick
	// movement on the XZ plane, gravity integration, jump (Jump action), and ground detection.
	// This is the correct way to move a character — a swept kinematic capsule, not a dynamic body
	// pushed by forces. Tuning comes from the CharacterControllerComponent (falls back to params).
	class CharacterController : public ScriptBehavior
	{
	public:
		void DeclareParams(std::vector<ScriptParam>& out) const override
		{
			out.push_back({ "moveSpeed", 4.0f });
			out.push_back({ "runSpeed", 7.0f });
			out.push_back({ "jumpHeight", 1.4f });
			out.push_back({ "gravity", -20.0f });
		}
		void OnStart(ScriptContext&) override { vertVel = 0.0f; }
		void OnUpdate(ScriptContext& ctx) override
		{
			if (!ctx.physics || !ctx.physics->HasCharacter(ctx.self.id)) return;

			const CharacterControllerComponent* cc = ctx.self.characterController ? &*ctx.self.characterController : nullptr;
			float moveSpeed = cc ? cc->moveSpeed : GetParam(ctx, "moveSpeed", 4.0f);
			float runSpeed  = cc ? cc->runSpeed  : GetParam(ctx, "runSpeed", 7.0f);
			float jumpH     = cc ? cc->jumpHeight: GetParam(ctx, "jumpHeight", 1.4f);
			float gravity   = cc ? cc->gravity   : GetParam(ctx, "gravity", -20.0f);

			float h = ctx.input.GetAxis("Horizontal");
			float v = ctx.input.GetAxis("Vertical");
			bool running = ctx.input.GetAction("Sprint");
			float speed = running ? runSpeed : moveSpeed;

			glm::vec3 planar(h, 0.0f, -v); // forward = -Z
			if (glm::length(planar) > 1.0f) planar = glm::normalize(planar);

			bool grounded = ctx.physics->IsCharacterGrounded(ctx.self.id);
			if (grounded && vertVel < 0.0f) vertVel = -2.0f; // keep lightly pinned to the ground
			if (grounded && ctx.input.GetActionDown("Jump"))
				vertVel = std::sqrt(glm::max(2.0f * -gravity * jumpH, 0.0f)); // v = sqrt(2*g*h)
			vertVel += gravity * ctx.deltaTime;

			glm::vec3 disp = planar * speed * ctx.deltaTime + glm::vec3(0.0f, vertVel * ctx.deltaTime, 0.0f);
			ctx.physics->MoveCharacter(ctx.self.id, disp, ctx.deltaTime);
		}
	private:
		float vertVel = 0.0f;
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
		Register("CharacterController", [] { return std::make_unique<CharacterController>(); });
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

} // namespace ogl